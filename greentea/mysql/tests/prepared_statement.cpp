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
	mysql::MySQLRes *res;
	mysql::MySQLStmt *st;
	char qbuf[4096], i;
	int rnum, ret;

	static const char q_create_tbl[] =
		"CREATE TABLE `users_%d` (" 							\
			"`id` bigint unsigned NOT NULL AUTO_INCREMENT,"				\
			"`username` varchar(255) COLLATE utf8mb4_unicode_520_ci NOT NULL,"	\
			"PRIMARY KEY (`id`),"							\
			"UNIQUE KEY `username` (`username`)"					\
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_520_ci;";


	static const char q_insert[] =
		"INSERT INTO `users_%d` VALUES (NULL, ?), (NULL, ?), (NULL, ?);";

	static const char q_select[] =
		"SELECT `id`, `username` FROM `users_%d` WHERE `username` = ? ORDER BY `id`";

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
	assert(!st->stmtInit());

	for (i = 0; i < (10 * 3); i += 3) {
		char ba[16], bb[16], bc[16];
		size_t la, lb, lc;

		la = (size_t) snprintf(ba, sizeof(ba), "user_%d", i + 0);
		lb = (size_t) snprintf(bb, sizeof(bb), "user_%d", i + 1);
		lc = (size_t) snprintf(bc, sizeof(bc), "user_%d", i + 2);

		st->bind(0, MYSQL_TYPE_STRING, ba, la);
		st->bind(1, MYSQL_TYPE_STRING, bb, lb);
		st->bind(2, MYSQL_TYPE_STRING, bc, lc);
		assert(!st->bindStmt());
		assert(!st->execute());
	}

	delete st;

	snprintf(qbuf, sizeof(qbuf), q_select, rnum);
	st = db->prepare(1, qbuf);
	assert(st);
	assert(!st->stmtInit());

	for (i = 0; i < (10 * 3); i++) {
		bool is_null[2];
		size_t reslen[2];
		char buf[2][0xff];

		mysql::MySQLStmtRes *res2;
		char ba[16];
		size_t la;

		la = (size_t) snprintf(ba, sizeof(ba), "user_%d", i + 0);

		st->bind(0, MYSQL_TYPE_STRING, ba, la);

		res2 = st->storeResult(3);
		assert(MYSQL_PTR_ERR<mysql::MySQLStmtRes>(res2) == -EINVAL);
		res2 = st->storeResult(2);
		assert(!MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmtRes>(res2));

		assert(!st->bindStmt());
		assert(!st->execute());

		res2->bind(0, MYSQL_TYPE_STRING, buf[0], sizeof(buf[0]), &is_null[0], &reslen[0]);
		res2->bind(1, MYSQL_TYPE_STRING, buf[1], sizeof(buf[1]), &is_null[1], &reslen[1]);
		assert(!res2->bindResult());

		while (!res2->fetchRow()) {
			assert(atoi(buf[0]) == (i + 1));
			assert(!strcmp(buf[1], ba));
		}

		delete res2;
	}

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
