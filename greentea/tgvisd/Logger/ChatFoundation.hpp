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
protected:
	KWorker				*kworker_ = nullptr;
	const td_api::chat		&chat_;

	inline ChatFoundation(KWorker *kworker, const td_api::chat &chat):
		kworker_(kworker),
		chat_(chat)
	{
	}
};

} /* namespace tgvisd::Logger */

#endif /* #ifndef TGVISD__LOGGER__CHATFOUNDATION_HPP */
