/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseParse.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cblonde <cblonde@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/29 08:25:35 by cblonde           #+#    #+#             */
/*   Updated: 2025/04/11 18:03:51 by glaguyon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

void	Response::checkConnection(std::map<std::string,
		std::string> const &headers)
{
	std::map<std::string, std::string>::const_iterator it;

	if ((_status / 100 * 100 == 300) || _autoIndex
		|| !_conf || !_conf->getRedirection().empty())
		_headers["Connection"] = "close";
	else
	{
		it = headers.find("Connection");
		if (it != headers.end())
			_headers["Connection"] = it->second;
		else
			_headers["Connection"] = "keep-alive";
	}
}


bool	Response::checkMethod(std::string method)
{
	std::set<std::string>		&methods(_conf->getAcceptedMethods());
	std::set<std::string>::iterator	it;
	std::string			allowed;

	for (it = methods.begin(); it != methods.end(); it++)
		allowed += *it + " ";
	allowed.erase(allowed.length() - 1);
	it = methods.find(method);
	if (it == methods.end())
	{
		_headers["Allow"] = allowed;
		return (false);
	}
	return (true);
}

bool	Response::checkContentLen()
{
	size_t	maxSize = _conf->getMaxSize();

	if (maxSize == 0)
		return (true);
	if (maxSize < static_cast<size_t>(_body.size()))
	{
		createError(413);
		return (false);
	}
	return (true);
}

bool	Response::checkExtCgi(void)
{
	std::map<std::string, std::string>		cgi(_conf->getCgi());

	if (_fileName.empty())
		return (false);
	for (std::map<std::string, std::string>::iterator it = cgi.begin();
		it != cgi.end(); it++)
		if (_fileName.size() >= it->first.size()
			&& _fileName.compare(_fileName.size() - it->first.size(),
			it->first.size(), it->first) == 0)
			return (true);
	return (false);
}

void	Response::getStatFile(std::string path)
{
	struct stat res;
	int			status;
	time_t		ttime;
	struct tm	*tmTime;
	char		buffer[1024];
	size_t		size;

	ttime = time(NULL);
	tmTime = gmtime(&ttime);
	strftime(buffer, 1024, "%a, %d %b %Y %T GMT", tmTime);
	_headers["Date"] = buffer;
	if (path.empty())
	{
		_headers.erase("Last-Modified");
		return ;
	}
	status = stat(path.c_str(), &res);
	if (status)
		return ;
	#ifdef __linux__
		ttime = res.st_mtim.tv_sec;
	#else
		ttime = res.st_mtimespec.tv_sec;
	#endif
	tmTime = gmtime(&ttime);
	strftime(buffer, 1024, "%a, %d %b %Y %T GMT", tmTime);
	_headers["Last-Modified"] = buffer;
	size = res.st_size;
	_buffer.reserve(size * sizeof(unsigned char));
}

std::string	getResponseTypeStr(int stat)
{
	std::map<int,std::string> names;

	names[200] = "OK";
	names[204] = "Gone";
	names[301] = "Moved Permanently";
	names[302] = "Found";
	names[308] = "Permanent Redirect";
	names[400] = "Bad Request";
	names[403] = "Forbidden";
	names[404] = "Not Found";
	names[405] = "Method Not Allowed";
	names[410] = "Gone";
	names[413] = "Content Too Large";
	names[415] = "Unsupported Media Type";
	names[500] = "Internal Server Error";
	names[501] = "Not Implemented";

	return (names[stat]);
}

std::string	getContentError(int stat)
{
	std::map<int,std::string> content;

	content[200] = "OK";
	content[204] = "Ressource deleted";
	content[400] = "the server would not process the request due to something \
the server considered to be a client error";
	content[403] = "Insufficient permissions to a resource or action";
	content[404] = "Page not found";
	content[405] = "Target resource doesn't support this method";
	content[410] = "Target resource is no longer available";
	content[413] = "Request entity was larger than limits defined by server";
	content[415] = "The request entity has a media type which the server or resource does not support";
	content[500] = "Server encountered an unexpected condition that prevented \
it from fulfilling the request";
	content[501] = "Server has not implemented the method\n";

	return (content[stat]);
}

void	Response::handleCgiHeader(std::string &str)
{
	size_t								index;
	std::string							line;
	std::pair<std::string, std::string> tmp;
	std::stringstream					ss(str);

	while (getline(ss, line))
	{
		index = line.find_first_of(":");
		if (index == std::string::npos)
			continue ;
		tmp.first = line.substr(0, index);
		tmp.second = line.substr(index + 1);
		formatHeader(tmp.first);
		trim(tmp.second);
		if (tmp.first.empty())
			continue ;
		if (tmp.first == "Set-Cookie")
			_cookies.push_back(tmp.second);
		else if (tmp.first == "Status")
		{
			char	*end;
			long	num = std::strtol(tmp.second.data(), &end, 10);
			if (end != tmp.second.data())
				_status = num;
		}
		else if (tmp.first == "Content-Type")
			_headers[tmp.first] = tmp.second;
	}
	getStatFile("");
}

void	Response::getCgiHeader(bool &finded)
{
	std::string	rawInput(_buffer.begin(), _buffer.end());
	std::string	head;
	size_t		index;
	size_t		size;

	for (size_t i = 0; i < rawInput.size(); ++i)
	{
		if (rawInput[i] == '\r')
		{
			if (i + 1 < rawInput.size() && rawInput[i + 1] == '\n')
			{
				head += '\n';
				++i;
			} else
			{
				head += '\n';
			}
		} else {
			head += rawInput[i];
		}
	}
	index = head.find("\n\n");
	if (index == std::string::npos)
		return ;
	finded = true;
	_buffer.clear();
	_buffer.insert(_buffer.begin(), head.begin() + index + 2, head.end());
	size = head.end() - (head.begin() + 2 + index);
	_buffer.resize(size);
	head.erase(head.begin() + index, head.end());
	handleCgiHeader(head);
}
