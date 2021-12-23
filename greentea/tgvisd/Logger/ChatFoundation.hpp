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
	virtual ~ChatFoundation(void)
	{
		if (db_)
			kworker_->putDbPool(db_);
	}

	virtual uint64_t getPK(void) = 0;

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
		if (db_)
			return db_;

		db_ = kworker_->getDbPool();
		if (unlikely(!db_)) {
			pr_err("Cannot get DB pool");
			return nullptr;
		}

		return db_;
	}
};

} /* namespace tgvisd::Logger */

#endif /* #ifndef TGVISD__LOGGER__CHATFOUNDATION_HPP */
