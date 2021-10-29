/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "MySQL.hpp"


static int test_fetch_001(mysql::MySQL *db)
{
	int ret, i;
	MYSQL_ROW row;
	int num_fields;
	mysql::MySQLRes *res;

	ret = db->query("SELECT 1, \"A\" UNION SELECT 2, \"B\" UNION SELECT 3, \"C\"");
	assert(ret == 0);

	res = db->storeResultRaw();
	assert(res);

	num_fields = res->numFields();
	assert(num_fields == 2);

	i = 0;
	while ((row = res->fetchRow())) {
		i++;
		char str[32];

		snprintf(str, sizeof(str), "%d", i);
		assert(!strcmp(row[0], str));

		snprintf(str, sizeof(str), "%c", 'A' + (i - 1));
		assert(!strcmp(row[1], str));
	}

	assert(i == 3);
	delete res;

	return 0;
}


static int do_test(void)
{
	int ret = 0;
	mysql::MySQL *db = nullptr;
	const char *host = getenv("TEST_MYSQL_HOST");
	const char *user = getenv("TEST_MYSQL_USER");
	const char *passwd = getenv("TEST_MYSQL_PASSWORD");
	const char *dbname = getenv("TEST_MYSQL_DBNAME");
	const char *port_str = getenv("TEST_MYSQL_PORT");
	uint16_t port = (uint16_t)atoi(port_str ? port_str : "0");

	assert(host);
	assert(user);
	assert(passwd);
	assert(dbname);

	try {
		db = new mysql::MySQL(host, user, passwd, dbname);
		db->setPort(port);
		db->connect();

		ret = test_fetch_001(db);
		if (ret)
			goto out;

	} catch (const std::runtime_error& e) {
		ret = 1;
		std::cout << "Error: " << e.what() << std::endl;
		goto out;
	}

out:
	delete db;
	return ret;
}


int main(void)
{
	return do_test();
}
