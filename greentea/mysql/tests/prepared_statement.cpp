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

#define QUERY_BUF_SIZE 4096

#define pr_err(FMT, ...) \
	printf(FMT " at %s %s:%d\n", __VA_ARGS__, __FILE__, __func__, __LINE__)


static int test_prep_stmt_001_create_table(mysql::MySQL *db, int rnum)
{
	static const char q_create[] =
		"CREATE TABLE `users_%d` (" 				\
			"`id` bigint unsigned NOT NULL AUTO_INCREMENT,"	\
			"`username` varchar(255) NOT NULL,"		\
			"PRIMARY KEY (`id`),"				\
			"UNIQUE KEY `username` (`username`)"		\
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;";

	int ret;
	char qbuf[QUERY_BUF_SIZE];

	snprintf(qbuf, sizeof(qbuf), q_create, rnum);
	ret = db->query(qbuf);
	if (unlikely(ret))
		pr_err("Error on query(): %s", db->getError());

	/* Create table should not have a result set. */
	assert(!db->storeResult());
	return ret;
}


static int test_prep_stmt_001_insert_data(mysql::MySQL *db, int rnum)
{
	static const char q_insert[] = "INSERT INTO `users_%d` VALUES (NULL, ?), (NULL, ?), (NULL, ?);";

	int i, errret = 0;
	const char *stmtErrFunc = nullptr;
	const char *errstr = nullptr;
	mysql::MySQLStmt *stmt;
	char qbuf[QUERY_BUF_SIZE];

	snprintf(qbuf, sizeof(qbuf), q_insert, rnum);
	stmt = db->prepare(3, qbuf);
	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt)) {

		if (MYSQL_IS_ERR<mysql::MySQLStmt>(stmt)) {
			errret = MYSQL_PTR_ERR<mysql::MySQLStmt>(stmt);
			errstr = strerror(errret);
		} else {
			errstr = db->getError();
			errret = db->getErrno();
		}

		stmt = nullptr;
		pr_err("Error on prepare(): (%d) %s", errret, errstr);
		goto err;
	}

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	for (i = 0; i < (10 * 3); i += 3) {
		char ba[16], bb[16], bc[16];
		size_t la, lb, lc;

		la = (size_t) snprintf(ba, sizeof(ba), "user_%d", i + 0);
		lb = (size_t) snprintf(bb, sizeof(bb), "user_%d", i + 1);
		lc = (size_t) snprintf(bc, sizeof(bc), "user_%d", i + 2);

		assert(stmt->bind(0, MYSQL_TYPE_STRING, ba, la));
		assert(stmt->bind(1, MYSQL_TYPE_STRING, bb, lb));
		assert(stmt->bind(2, MYSQL_TYPE_STRING, bc, lc));

		if (unlikely(stmt->bindStmt())) {
			stmtErrFunc = "bindStmt";
			goto stmt_err;
		}

		if (unlikely(stmt->execute())) {
			stmtErrFunc = "execute";
			goto stmt_err;
		}
	}

	delete stmt;
	return 0;

stmt_err:
	errstr = stmt->getError();
	errret = stmt->getErrno();
	pr_err("Error on %s(): (%d) %s", stmtErrFunc, errret, errstr);
err:
	if (stmt)
		delete stmt;

	return errret;
}


static int test_prep_stmt_001_select_data(mysql::MySQL *db, int rnum)
{
	static const char q_select[] = "SELECT `id`, `username` FROM `users_%d` WHERE `username` = ? ORDER BY `id`";

	int i, errret = 0;
	const char *stmtErrFunc = nullptr;
	const char *errstr = nullptr;
	mysql::MySQLStmt *stmt;
	char qbuf[QUERY_BUF_SIZE];

	snprintf(qbuf, sizeof(qbuf), q_select, rnum);
	stmt = db->prepare(1, qbuf);
	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt)) {

		if (MYSQL_IS_ERR<mysql::MySQLStmt>(stmt)) {
			errret = MYSQL_PTR_ERR<mysql::MySQLStmt>(stmt);
			errstr = strerror(errret);
		} else {
			errstr = db->getError();
			errret = db->getErrno();
		}

		stmt = nullptr;
		pr_err("Error on prepare(): (%d) %s", errret, errstr);
		goto err;
	}

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	for (i = 0; i < (10 * 3); i++) {
		bool is_null[2];
		size_t reslen[2];
		char buf[2][0xff];

		size_t la;
		char ba[16];
		mysql::MySQLStmtRes *res;

		la = (size_t) snprintf(ba, sizeof(ba), "user_%d", i + 0);

		stmt->bind(0, MYSQL_TYPE_STRING, ba, la);

		if (unlikely(stmt->bindStmt())) {
			stmtErrFunc = "bindStmt";
			goto stmt_err;
		}

		if (unlikely(stmt->execute())) {
			stmtErrFunc = "execute";
			goto stmt_err;
		}

		res = stmt->storeResult(3);
		assert(MYSQL_PTR_ERR<mysql::MySQLStmtRes>(res) == -EINVAL);

		res = stmt->storeResult(2);
		assert(!MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmtRes>(res));

		res->bind(0, MYSQL_TYPE_STRING, buf[0], sizeof(buf[0]), &is_null[0], &reslen[0]);
		res->bind(1, MYSQL_TYPE_STRING, buf[1], sizeof(buf[1]), &is_null[1], &reslen[1]);
		assert(!res->bindResult());

		while (!res->fetchRow()) {
			assert(atoi(buf[0]) == (i + 1));
			assert(!strcmp(buf[1], ba));
		}

		delete res;
	}

	delete stmt;
	return 0;
stmt_err:
	errstr = stmt->getError();
	errret = stmt->getErrno();
	pr_err("Error on %s(): (%d) %s", stmtErrFunc, errret, errstr);
err:
	if (stmt)
		delete stmt;

	return errret;
}


static int test_prep_stmt_001_drop_table(mysql::MySQL *db, int rnum)
{

	static const char q_drop[] = "DROP TABLE `users_%d`";

	/* Clean up! */
	int ret;
	char qbuf[QUERY_BUF_SIZE];

	snprintf(qbuf, sizeof(qbuf), q_drop, rnum);
	ret = db->query(qbuf);
	if (unlikely(ret))
		pr_err("Error on query(): %s", db->getError());

	/* Drop table should not have a result set. */
	assert(!db->storeResult());
	return ret;
}


static int test_prep_stmt_001(mysql::MySQL *db)
{
	int rnum, ret = 0;

	rnum = rand();
	ret |= test_prep_stmt_001_create_table(db, rnum);
	if (unlikely(ret))
		return ret;

	ret |= test_prep_stmt_001_insert_data(db, rnum);
	if (unlikely(ret))
		goto drop_tbl;

	ret |= test_prep_stmt_001_select_data(db, rnum);
	if (unlikely(ret))
		goto drop_tbl;

drop_tbl:
	ret |= test_prep_stmt_001_drop_table(db, rnum);
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
		if (unlikely(!db->connect()))
			throw new std::runtime_error(db->getError());

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
