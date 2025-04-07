/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cblonde <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/26 11:15:20 by cblonde           #+#    #+#             */
/*   Updated: 2025/04/07 22:59:23 by glaguyon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "Client.hpp"

Response::Response(Requests const &req, Client  &client, Server &server)
	: _client(client),
	_server(server)
{
	std::map<std::string, std::string> const &headers = req.getHeaders();

	this->_socket = req.getFd();
	this->_body = req.getBody();
	this->_headerSent = false;
	this->_headerReady = false;
	this->_sizeSend = 0;
	this->_fileFd = -1;
	this->_cgiFd[0] = -1;
	this->_cgiFd[1] = -1;
	this->_protocol = req.getProtocol();
	this->_host = req.getHost();
	this->_path = req.getPath();
	this->_fileName = req.getFileName();
	this->_query = req.getQuery();
	this->_status = req.getError();
	this->_conf = &req.getConf();
	this->uploadPath = "";
	this->_cgi = false;
	this->_autoIndex = false;
	if (_conf != NULL)
	{
		this->uploadPath = _conf->getUploadDir();
		if (uploadPath != "" && uploadPath[uploadPath.size() -1] != '/')
			uploadPath += "/";
		this->_cgi = _conf->getCgi().empty() ? false : true;
		this->_autoIndex = _conf->getAutoindex();
	}
	initMimeTypes(_mimeTypes);
	initResponseHeaders(_headers);
	checkConnection(headers, req.getType());
	if (_status != 200)//400, 404, 301, 308, 501
	{
		if (_status / 100 * 100 == 300)
		{
			_headers["Location"] = "Location: " + urlEncode(_path);
			if (_query != "")
				_headers["Location"] += "?" + _query;
			getStatFile("");
			createResponseHeader();
			return;
		}
		createError(_status);
		return;
	}
	if (_status / 100 * 100 == 300)
	{
		std::string	redir = _conf->getRedirection();

		_headers["Location"] = "Location: "
			+ redir.substr(0, redir.find("://") + 3)
			+ urlEncode(redir.substr(redir.find("://") + 3,
				redir.find("?") - redir.find("://") - 3));
		if (_query != "")
			_headers["Location"] += "?" + _query;
		getStatFile("");
		createResponseHeader();
		return ;
	}
	if (!checkMethod(req.getType()))
		return ;
	if (!checkContentLen())
		return ;
	handleMethod(req, headers);
}

Response::~Response(void)
{
}

void	Response::handleMethod(Requests const &req,
	std::map<std::string, std::string> const &headers)
{
	std::string	method = req.getType();
	
	if (method == "POST" && _fileName == ""
		&& _conf->getAliasedPart() + _path.substr(_conf->getMount().size())
		== _conf->getName() + ((_conf->getName()[_conf->getName().size() - 1] == '/') ? "" : "/"))
	{
		if (headers.at("Content-Type").compare(0, 19, "multipart/form-data")
			|| std::string(" \t;\0", 4).find(headers.at("Content-Type")[19]) == std::string::npos)
			createError(415);
		else
			uploadFile(req.getHeaders());
	}
	else if (_cgi && checkExtCgi())
	{
		std::cout << GREEN << "/* CGI */" << RESET << std::endl;
		std::map<std::string, std::string> cgi(_conf->getCgi());
		for (std::map<std::string, std::string>::iterator it = cgi.begin();
				it != cgi.end(); it++)
		{
			if (!testAccess(it->second, EXIST)
					|| !testAccess(it->second, EXECUTABLE))
			{
				createError(400);
				return ;
			}
		}
		Cgi cgiObj(req, _server);
		int status = cgiObj.execScript();
		if (status == 200)
		{
			_cgiFd[0] = cgiObj.getChildFd();
			_cgiFd[1] = cgiObj.getParentFd();
			addFdToCluster(cgiObj.getChildFd(), (POLLIN | POLLHUP));
			addFdToCluster(cgiObj.getParentFd(), POLLOUT);
			getStatFile("");
			_headers.erase("Last-Modified");
		}
		else
			createError(status);
	}
	else if (method == "DELETE")
		deleteFile();
	else if (method == "POST")
	{
		if (headers.at("Content-Type").compare(0, 19, "multipart/form-data") == 0
			&& std::string(" \t;\0", 4).find(headers.at("Content-Type")[19]) != std::string::npos)
			createError(403);
		else
			createError(400);
	}
	else if (method == "GET")
		getFileOrIndex();
}

void	Response::createError(int stat)
{
//TODO must not use headers or conf if error = 400
	std::string	content;
	int			fd;

	_status = stat;//idk
	std::cout << "je susi erreur\n";
	std::cout << "status " << _status << "\n";
	if (!_autoIndex || _status != 200)//XXX crappy hotfix
	//if (!_autoIndex || (_autoIndex && !_fileName.empty()))
	{
		content = _server.getErrorPage(stat);
		if (!content.empty() && _status != 500)
		{
			fd = getFile(content);
			if (fd == -1)
			{
				createError(500);
				return ;
			}
			addFdToCluster(fd, POLLIN);
			getStatFile(content);
			_fileFd = fd;
			_status = stat;
			return ;
		}
		std::cout << "wow\n";
		content = ERROR_PAGE(getResponseTypeStr(stat), getContentError(stat),
				to_string(stat));
		getStatFile("");
		_headers["Content-Type"] += "text/html; charset=UFT-8";
		_headers.erase("Last-Modified");
		_status = stat;
	}
	else
	{//TODO : pas de dir: 404, pas d'autoindex + dossier: 403
		//_status = 200;
		try
		{
		content = AutoIndex::generate(_conf->getMount(),
		_conf->getAliasedPart(), _path, _host);
		getStatFile("");
		_headers.erase("Last-Modified");
		}
		catch (int code)//may work
		{
			std::cout << "fuck autoindex exploded\n";
			_status = code;
			createError(code);
			return ;
		}
	}
	_buffer.insert(_buffer.begin(), content.begin(), content.end());
	createResponseHeader();
	return ;
}

void	Response::createResponseHeader(void)
{
	std::map<std::string,std::string>::iterator it;

	_response = _protocol + " "
		+ to_string(_status) + " "
		+ getResponseTypeStr(_status)
		+ "\r\n";
	_headers["Content-Length"] += to_string(_buffer.size());
	std::cout << _buffer.size() << " |||\n" << std::string(_buffer.begin(), _buffer.end()) << "|||\n";
	if (_status != 301 && _status != 302)//added 400 hotfix
	{
		std::cout << "salut: " << _mimeTypes[getFileType(_fileName)] << "\n";
		std::cout << getFileType(_fileName) << "\n";
		std::cout << "header: \"" << _headers["Content-Type"] << "\"\n";
		_headers["Content-Type"] += (!_cgi && (!_autoIndex || (
							_autoIndex && !_fileName.empty())))
			? _mimeTypes[getFileType(_fileName)]
			: "text/html; charset=UFT-8";
		std::cout << "header: \"" << _headers["Content-Type"] << "\"\n";
	}
	else
		_headers.erase("Content-Type");
	for (it = _headers.begin(); it != _headers.end(); it++)
		_response += (it->second + "\r\n");
	_response += "\r\n";

	_headerReady = true;
	std::cout << CYAN << "Header ready" << std::endl << _response
		<< RESET << std::endl;
	return ;
}

bool	Response::checkExtCgi(void)//should not use getfiletype
{
	std::map<std::string, std::string>		cgi(_conf->getCgi());
	std::map<std::string, std::string>::iterator	it;
	std::string					ext(getFileType(_fileName));

	if (_fileName.empty())
		return (false);
	for (it = cgi.begin(); it != cgi.end(); it++)
		if (it->first == ext)
			return (true);
	return (false);
}
