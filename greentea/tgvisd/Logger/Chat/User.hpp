// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger::Chat
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#ifndef TGVISD__LOGGER__CHAT__USER_HPP
#define TGVISD__LOGGER__CHAT__USER_HPP

#include <tgvisd/Logger/ChatFoundation.hpp>

namespace tgvisd::Logger::Chat {

class User: public tgvisd::Logger::ChatFoundation
{
public:
	inline User(KWorker *kworker,
		    const td_api::chat	&chat):
		tgvisd::Logger::ChatFoundation(kworker, chat)
	{
	}

	uint64_t getPK(void) override;
};

} /* namespace tgvisd::Logger::Chat */

#endif /* #ifndef TGVISD__LOGGER__CHAT__USER_HPP */
