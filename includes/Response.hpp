/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cblonde <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/26 11:16:02 by cblonde           #+#    #+#             */
/*   Updated: 2025/03/26 12:29:14 by cblonde          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <webserv.hpp>
# include <Requests.hpp>
# include <AutoIndex.hpp>
# include <ErrorPages.hpp>
# include <Cgi.hpp>
# include <map>
# include <string>
# include <vector>
# include <sys/socket.h>
# include <poll.h>

struct	Upload
{
	char	*start;
	size_t	size;
	size_t	offset;
};

class Response
{
	private:
		int									_status;
		Route								*_conf;
		int									_socket;
		std::string							_protocol;
		std::string							_body;
		std::string							_path;
		std::string							_host;
		int									_port;
		std::string							_fileName;
		std::map<std::string,std::string>	_headers;
		bool								_autoIndex;
		bool								_cgi;
		std::string							_response;
		int									_resSize;
		std::map<std::string, std::string>	_mimeTypes;
		std::vector<unsigned char>			_buffer;
		int									_fileFd;
		bool								_headerSent;
		bool								_headerReady;
		int									_sizeSend;
		Response(void);
		void	handleFile(Requests const &req);
		void	isReferer(std::map<std::string, std::string> const &headers);
		void	checkConnection(std::map<std::string,
				std::string> const &headers);
		bool	checkMethod(std::string method);
		bool	checkContentLen(std::map<std::string,
				std::string> const &headers);
		void	getStatFile(void);
		void	uploadFile(std::map<std::string, std::string> const &headers);
	public:
		Response(Requests const &req);
		Response(Response const &src);
		~Response(void);
		Response &operator=(Response const &rhs);

		std::string		getResponse(void) const;
		size_t			getResSize(void) const;
		int				getFileFd(void) const;
		int				getSocket(void) const;
		void			setResponse(std::string str);
		void			setSocket(int const socket);
		void	createError(int stat);
		void	createResponseHeader(void);
		bool	handleInOut(struct pollfd &fd);
};

#endif
