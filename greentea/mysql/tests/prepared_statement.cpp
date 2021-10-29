/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <time.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "MySQL.hpp"


static int test_prep_stmt_001(mysql::MySQL *db)
{
	int rnum;
	int ret, i;
	MYSQL_ROW row;
	int num_fields;
	mysql::MySQLRes *res;
	mysql::MySQLStmt *st;
	char qbuf[4096];

	static const char q_create_tbl[] =
		"CREATE TABLE `users_%d` (" 							\
			"`id` bigint unsigned NOT NULL AUTO_INCREMENT,"				\
			"`username` varchar(255) COLLATE utf8mb4_unicode_520_ci NOT NULL,"	\
			"PRIMARY KEY (`id`),"							\
			"UNIQUE KEY `username` (`username`)"					\
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_520_ci;";


	static const char q_insert[] =
		"INSERT INTO `users_%d` VALUES (NULL, ?), (NULL, ?), (NULL, ?);";


	static const char q_drop_tbl[] = "DROP TABLE `users_%d`";

	rnum = rand();

	snprintf(qbuf, sizeof(qbuf), q_create_tbl, rnum);
	ret = db->query(qbuf);
	assert(ret == 0);

	/* Create table should not have a result set. */
	res = db->storeResult();
	assert(!res);

	/* Insert with prepared statement. */
	snprintf(qbuf, sizeof(qbuf), q_insert, rnum);
	st = db->prepare(3, qbuf);
	assert(st);

	st->bind(0, MYSQL_TYPE_STRING, (void *)"user_a", sizeof("user_a") - 1, NULL, NULL);
	st->bind(1, MYSQL_TYPE_STRING, (void *)"user_b", sizeof("user_b") - 1, NULL, NULL);
	st->bind(2, MYSQL_TYPE_STRING, (void *)"user_c", sizeof("user_c") - 1, NULL, NULL);
	st->bindStmt();
	st->execute();

	delete st;

	/* Clean up! */
	snprintf(qbuf, sizeof(qbuf), q_drop_tbl, rnum);
	ret = db->query(qbuf);
	assert(ret == 0);

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

		ret = test_prep_stmt_001(db);
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
	srand((unsigned int)time(NULL));
	return do_test();
}
