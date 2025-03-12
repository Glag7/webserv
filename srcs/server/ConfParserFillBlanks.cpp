/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfParserFillBlanks.cpp                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: glaguyon <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/11 18:47:40 by glaguyon          #+#    #+#             */
/*   Updated: 2025/03/12 15:32:28 by glaguyon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfParser.hpp"
#include "Cluster.hpp"
#include "Server.hpp"
#include "Route.hpp"
#include "Conf.hpp"

void	ConfParser::fillBlanksLevel(Route &prev, std::vector<Route> &routes)
{
	for (std::vector<Route>::iterator it = routes.begin(); it < routes.end(); ++it)
	{
		if (it->getAcceptedMethods().size() == 0)
			it->getAcceptedMethods() = prev.getAcceptedMethods();
		if (it->getRoot() == "")
			it->setRoot(prev.getRoot());
		if (it->getIndex().size() == 0)
			it->getIndex() = prev.getIndex();
		if (it->isAutoindexAssigned() == false)
			it->setAutoindex(prev.getAutoindex());
		if (it->getCgi().size() == 0)
			it->getCgi() = prev.getCgi();
		if (it->getRedirection() == "")
			it->setRedirection(prev.getRedirection());
	}
	for (std::vector<Route>::iterator it = routes.begin(); it < routes.end(); ++it)
		fillBlanksLevel(*it, it->getRoutes());
}

void	ConfParser::fillBlanks()
{
	std::vector<Server>	&serverList = cluster.getServers();

	for (std::vector<Server>::iterator it = serverList.begin(); it < serverList.end(); ++it)
	{
		std::vector<Route>	&routes = it->getRoutes();

		if (it->getRoot() == "")
			it->setRoot(Cluster::defaultRoot);
		if (it->isAutoindexAssigned() == false)
			it->setAutoindex(true);
		if (it->getAcceptedMethods().size() == 0)
			it->addMethod("GET");
		fillBlanksLevel(*it, routes);
	}
}

