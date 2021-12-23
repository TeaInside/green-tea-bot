// SPDX-License-Identifier: GPL-2.0-only

#ifndef TGVISD__MYSQL_HELPER_H
#define TGVISD__MYSQL_HELPER_H

#include <mysql/MySQL.hpp>

void mysql_handle_stmt_err(const char *stmtErrFunc, mysql::MySQLStmt *stmt);
void mysql_handle_prepare_err(mysql::MySQL *db, mysql::MySQLStmt *stmt);

#endif /* #ifndef TGVISD__MYSQL_HELPER_H */
