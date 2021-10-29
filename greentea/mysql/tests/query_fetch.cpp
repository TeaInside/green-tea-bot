/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "MySQL.hpp"

#define pr_err(FMT, ...) \
	printf(FMT " at %s %s:%d\n", __VA_ARGS__, __FILE__, __func__, __LINE__)

static int test_fetch_001(mysql::MySQL *db)
{
	int ret, i;
	MYSQL_ROW row;
	int num_fields;
	mysql::MySQLRes *res;
	static const char q[] = "SELECT 1, \"A\" UNION SELECT 2, \"B\" UNION SELECT 3, \"C\"";

	ret = db->realQuery(q, sizeof(q) - 1);
	if (ret) {
		pr_err("Error query(): %s", db->getError());
		ret = 1;
		goto out;
	}

	res = db->storeResult();
	if (MYSQL_IS_ERR_OR_NULL(res)) {
		pr_err("Error storeResult(): %s", db->getError());
		ret = 1;
		goto out;
	}

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


out:
	return ret;
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
		if (unlikely(!db->connect())) {
			pr_err("Error connect(): %s", db->getError());
			ret = 1;
			goto out;
		}

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
