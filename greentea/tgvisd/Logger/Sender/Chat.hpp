// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger::Sender
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#ifndef TGVISD__LOGGER__SENDER__CHAT_HPP
#define TGVISD__LOGGER__SENDER__CHAT_HPP

#include <tgvisd/Logger/SenderFoundation.hpp>

namespace tgvisd::Logger::Sender {

class Chat: public tgvisd::Logger::SenderFoundation
{
public:
	inline Chat(KWorker *kworker,
		    const td_api::MessageSender &sender):
		tgvisd::Logger::SenderFoundation(kworker, sender)
	{
	}

	uint64_t getPK(void) override;
};

} /* namespace tgvisd::Logger::Sender */

#endif /* #ifndef TGVISD__LOGGER__SENDER__CHAT_HPP */
