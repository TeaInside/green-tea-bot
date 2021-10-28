/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef mysql__MySQL__HPP
#define mysql__MySQL__HPP

#include <mysql.h>
#include <cstdint>

namespace mysql {

class MySQL
{
private:
	MYSQL *mysql_ = nullptr;
	const char *host_ = nullptr;
	const char *user_ = nullptr;
	const char *passwd_ = nullptr;

public:
	MySQL(const char *host, const char *user, const char *passwd);
	void connect(void);

	inline const char *error(void)
	{
		return mysql_error(mysql_);
	}

	inline int query(const char *q)
	{
		return mysql_query(mysql_, q);
	}

	inline void close(void)
	{
		if (mysql_)
			mysql_close(mysql_);
	}

	inline ~MySQL(void)
	{
		this->close();
	}
};

} /* namespace mysql */

#endif /* #ifndef mysql__MySQL__HPP */
