/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <cstdio>
#include <stdexcept>

#include "MySQL.hpp"

#define ERR_BUF 512

namespace mysql {


MySQL::MySQL(const char *host, const char *user, const char *passwd):
	host_(host),
	user_(user),
	passwd_(passwd)
{
	mysql_ = mysql_init(NULL);
	if (!mysql_)
		throw std::runtime_error("Cannot init mysql on mysql_init()\n");
}


void MySQL::connect(void)
{
	int ret;

	ret = mysql_real_connect(mysql_, host_, user_, passwd_, dbname_,
				 0, NULL, 0);
	if (!ret) {
		char buf[ERR_BUF];
		snprintf(buf, sizeof(buf),
			 "Cannot connect on mysql_real_connect(): %s\n",
			 mysql_error(mysql_));
		throw std::runtime_error(std::string(buf));
	}
}

} /* namespace mysql */
