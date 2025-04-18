/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Requests.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cblonde <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/21 10:24:52 by cblonde           #+#    #+#             */
/*   Updated: 2025/04/10 14:36:22 by cblonde          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTS_HPP
# define REQUESTS_HPP

# include "webserv.hpp"
# include "utils.hpp"
# include "Route.hpp"

class Client;
class Cluster;

typedef enum	e_request_type
{
	GET,
	POST,
	DELETE,
	UNIMPLEMENTED,
	UNKNOWN
} t_rqType;

class Requests
{
	private:
		int								fd;
		int								error;
		std::map<std::string,std::string>	_headers;
		std::string							_requestUri;
		std::string							_documentUri;
		std::string							_pathInfo;
		std::string							_host;
		std::string							_path;
		std::string							_protocol;
		t_rqType							_type;
		std::string							_body;
		std::map<std::string,std::string>	_mimeTypes;
		std::string							_query;
		std::string							_fileName;
		Route								*_conf;
		Client								&_client;
		Cluster								*cluster;
		/* handle in out */
		std::vector<unsigned char> 			_buffer;


		void	parse(std::string str, Cluster *c);
		void	handlePath(void);
		void	handleFile(std::string n, std::string alias);
	public:
		Requests(std::string &str, Client  &client, Cluster *c, int fd);
		Requests(Requests const &src);
		~Requests(void);
		Requests	&operator=(Requests const &rhs);
		int				getFd() const;
		std::string			getProtocol(void) const;
		std::string			getPath(void) const;
		std::map<std::string,std::string> const	&getHeaders(void) const;
		std::string			getHost(void) const;
		std::string			getType(void) const;
		std::string const		&getBody(void) const;
		std::string			getQuery(void) const;
		std::string			getFileName(void) const;
		Route				&getConf(void) const;
		std::string			getRequestUri(void) const;
		std::string			getDocumentUri(void) const;
		std::string			getPathInfo(void) const;
		std::string			getContentType(void) const;
		std::string			getClientHostName(void) const;
		std::string			getClientIpStr(void) const;
		std::string			getClientPort(void) const;
		int					getError() const;
		std::string			searchSession(std::string id) const;
		void				handleSession(std::string key,
				std::string value) const;
		void				addSession(std::pair<std::string,
				std::string> pair) const;
		
};

#endif
