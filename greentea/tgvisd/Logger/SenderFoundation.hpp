// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger::Chat
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#ifndef TGVISD__LOGGER__SENDERFOUNDATION_HPP
#define TGVISD__LOGGER__SENDERFOUNDATION_HPP

#include <tgvisd/KWorker.hpp>

namespace tgvisd::Logger {

class SenderFoundation
{
public:
	virtual ~SenderFoundation(void) = default;
	virtual uint64_t getPK(void) = 0;

protected:
	KWorker				*kworker_ = nullptr;
	const td_api::MessageSender	&sender_;

	inline SenderFoundation(KWorker *kworker,
				const td_api::MessageSender &sender):
		kworker_(kworker),
		sender_(sender)
	{
	}
};

} /* namespace tgvisd::Logger */

#endif /* #ifndef TGVISD__LOGGER__SENDERFOUNDATION_HPP */
