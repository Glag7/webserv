/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cblonde <cblonde@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/06 07:14:01 by cblonde           #+#    #+#             */
/*   Updated: 2025/04/11 17:45:09 by glaguyon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"

Cgi::Cgi(void)
{
	return ;
}

Cgi::Cgi(Requests const &req, Server &server)
{
	initCgi(req, server);
	return ;
}

Cgi::Cgi(Cgi const &src)
{
	*this = src;
	return ;
}

Cgi::~Cgi(void)
{
	size_t	i;

	i = 0;
	if (_env)
	{
		while (_env[i])
		{
			delete [] _env[i];
			_env[i] = NULL;
			i++;
		}
		delete [] _env;
		_env = NULL;
	}
	return ;
}

Cgi	&Cgi::operator=(Cgi const &rhs)
{
	if (this != &rhs)
	{
		if (_env) {
			size_t i = 0;
			while (_env[i]) {
				delete[] _env[i];
				_env[i] = NULL;
				i++;
			}
			delete[] _env;
			_env = NULL;
		}
		_envMap = rhs._envMap;
		_pid = rhs._pid;
		_fileType = rhs._fileType;
		_status = rhs._status;
		_scriptPath = rhs._scriptPath;
		_cgiPath = rhs._cgiPath;
	}
	return (*this);
}

void	Cgi::initCgi(Requests const &req, Server &server)
{
	Route	&reqConf = req.getConf();
	std::map<std::string, std::string>::iterator cgi;
	std::map<std::string, std::string>::const_iterator head;
	std::map<std::string, std::string> reqHeaders(req.getHeaders());
	std::set<std::string>::iterator name;
	std::set<std::string>	names;
	std::string	tmp;

	names = server.getNames();
	for (head = reqHeaders.begin(); head != reqHeaders.end(); head++)
	{
		tmp = "HTTP_" + head->first;
		toUpper(tmp);
		_envMap[tmp] = head->second;
	}
	_envMap["QUERY_STRING"] = req.getQuery();
	_envMap["REQUEST_METHOD"] = req.getType();
	_envMap["CONTENT_TYPE"] = req.getContentType();
	_envMap["CONTENT_LENGTH"] = to_string(req.getBody().size());
	_envMap["SCRIPT_NAME"] = req.getFileName();
	_envMap["PATH_INFO"] = req.getPathInfo();
	_envMap["REQUEST_URI"] = req.getRequestUri();
	_envMap["DOCUMENT_URI"] = req.getDocumentUri();
	_envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	_envMap["GATEWAY_INTERFACE"] = "CGI/1.0";
	_envMap["SERVER_SOFTWARE"] = "webserv/1.0";
	_envMap["REMOTE_ADDR"] = req.getClientHostName();
	_envMap["REMOTE_PORT"] = req.getClientPort();
	_envMap["SERVER_ADDR"] = server.getIpStr();
	_envMap["SERVER_PORT"] = server.getPortStr();
	tmp = "";
	for (name = names.begin(); name != names.end(); name++)
	{
		std::set<std::string>::iterator name2 = name;

		if (++name2 == names.end())
			tmp += *name + ";";
		else
			tmp += *name + "; ";
	}
	_envMap["SERVER_NAME"] = tmp;
	_pid = -1;
	_status = -1;
	_scriptDir = req.getPath();
	_scriptPath = req.getPath() + "/" + req.getFileName();
	
	std::string	fileName = req.getFileName();
	cgi = reqConf.getCgi().begin();
	while (cgi != reqConf.getCgi().end())
	{
		if (fileName.size() >= cgi->first.size()
			&& fileName.compare(fileName.size() - cgi->first.size(),
			cgi->first.size(), cgi->first) == 0)
			break;
		++cgi;
	}
	if (cgi != reqConf.getCgi().end())
		_cgiPath = cgi->second;
	else
		_cgiPath = "";
	_env = NULL;
	_parentToChild[0] = -1;
	_parentToChild[1] = -1;
	_childToParent[0] = -1;
	_childToParent[1] = -1;
	if (pipe(_parentToChild) == -1 || pipe(_childToParent) == -1)
		closePipes();
}

char	**Cgi::createEnvArr(void)
{
	size_t		i;
	std::string	tmp;
	_env = new char *[(_envMap.size() + 1 )];
	std::map<std::string, std::string>::iterator it;

	i = 0;
	memset(static_cast<void *>(_env), 0, (_envMap.size() + 1) * sizeof(char *));
	for (it = _envMap.begin(); it != _envMap.end(); it++)
	{
		tmp = it->first + "=" + it->second;
		_env[i] = new char[tmp.size() + 1];
		strcpy(_env[i], static_cast<const char *>(tmp.data()));
		i++;
	}	
	return (_env);
}

int	Cgi::execScript(void)
{
	try
	{
		if (_parentToChild[0] == -1)
			throw std::runtime_error("bad pipe");
		_env = createEnvArr();
	}
	catch (std::exception &e)
	{
		closePipes();
		std::cerr << RED << "Error: CGI: " << e.what() << std::endl << RESET;
		return (500);
	}
	if (!testAccess(_scriptPath, EXIST))
	{
		closePipes();
		return (404);
	}
	if (!testAccess(_cgiPath, EXIST) || !testAccess(_cgiPath, EXECUTABLE))
	{
		closePipes();
		return (403);
	}
	_pid = fork();
	if (_pid == -1)
	{
		closePipes();
		return (500);
	}
	else if (!_pid)
	{
		char const *argv[3] = {_cgiPath.empty()
			? _scriptPath.data()
				: _cgiPath.data(), _scriptPath.data(), NULL};
		if (dup2(_parentToChild[0], 0) != -1 && dup2(_childToParent[1], 1) != -1)
		{
			if (!chdir(_scriptDir.data()))
			{
				closePipes();
				execve(argv[0], (char *const *)(argv), _env);
			}
		}
		closePipes();
		close(0);
		close(1);
		std::cerr << RED << "Error: Execve: Cgi: " << strerror(errno)
			<< RESET << std::endl;
		throw(500);
	}
	close(_parentToChild[0]);
	_parentToChild[0] = -1;
	close(_childToParent[1]);
	_childToParent[1] = -1;
	return (200);
}

int	&Cgi::getChildFd(void)
{
	return (_childToParent[0]);
}

int	&Cgi::getParentFd(void)
{
	return (_parentToChild[1]);
}

pid_t	Cgi::getPid()
{
	return _pid;
}

void	Cgi::closePipes()
{
	if (_parentToChild[0] != -1)
		close(_parentToChild[0]);
	if (_parentToChild[1] != -1)
		close(_parentToChild[1]);
	if (_childToParent[0] != -1)
		close(_childToParent[0]);
	if (_childToParent[1] != -1)
		close(_childToParent[1]);
	_parentToChild[0] = -1;
	_parentToChild[1] = -1;
	_childToParent[0] = -1;
	_childToParent[1] = -1;
}
