// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger::Chat
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#ifndef TGVISD__LOGGER__CHATFOUNDATION_HPP
#define TGVISD__LOGGER__CHATFOUNDATION_HPP

#include <tgvisd/KWorker.hpp>

namespace tgvisd::Logger {

class ChatFoundation
{
public:
	virtual ~ChatFoundation(void) = default;
	virtual uint64_t getPK(void) = 0;

	inline void setDbPool(mysql::MySQL *db)
	{
		db_ = db;
	}

protected:
	KWorker				*kworker_ = nullptr;
	const td_api::chat		&chat_;
	mysql::MySQL			*db_ = nullptr;

	inline ChatFoundation(KWorker *kworker, const td_api::chat &chat):
		kworker_(kworker),
		chat_(chat)
	{
	}

	inline mysql::MySQL *getDbPool(void)
	{
		return db_;
	}
};

} /* namespace tgvisd::Logger */

#endif /* #ifndef TGVISD__LOGGER__CHATFOUNDATION_HPP */
