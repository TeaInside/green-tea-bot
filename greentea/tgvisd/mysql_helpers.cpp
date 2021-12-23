// SPDX-License-Identifier: GPL-2.0-only

#include <string.h>
#include <tgvisd/common.hpp>
#include <tgvisd/mysql_helpers.hpp>

void mysql_handle_stmt_err(const char *stmtErrFunc, mysql::MySQLStmt *stmt)
{
	int err_ret;
	const char *err_str;

	err_ret = stmt->getErrno();
	err_str = stmt->getError();
	pr_err("%s(): (%d) %s", stmtErrFunc, err_ret, err_str);
}

void mysql_handle_prepare_err(mysql::MySQL *db, mysql::MySQLStmt *stmt)
{
	int err_ret;
	const char *err_str;

	if (MYSQL_IS_ERR<mysql::MySQLStmt>(stmt)) {
		err_ret = -MYSQL_PTR_ERR<mysql::MySQLStmt>(stmt);
		err_str = strerror(err_ret);
	} else {
		err_ret = db->getErrno();
		err_str = db->getError();
	}

	pr_err("prepare(): (%d) %s", err_ret, err_str);
}
