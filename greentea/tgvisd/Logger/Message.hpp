// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#ifndef TGVISD__LOGGER__MESSAGE_HPP
#define TGVISD__LOGGER__MESSAGE_HPP

#include <tgvisd/KWorker.hpp>
#include <tgvisd/Logger/Chat/Group.hpp>
#include <tgvisd/Logger/Chat/User.hpp>
#include <tgvisd/Logger/Sender/Chat.hpp>
#include <tgvisd/Logger/Sender/User.hpp>

namespace tgvisd::Logger {

class Message
{
public:
	Message(KWorker *kworker, const td_api::message &message);
	~Message(void);

	inline void set_chat_lock(std::mutex *chat_lock)
	{
		chat_lock_ = chat_lock;
	}

	inline void set_chat(td_api::object_ptr<td_api::chat> chat)
	{
		chat_ = std::move(chat);
	}

	inline td_api::object_ptr<td_api::chat> get_chat(void)
	{
		return std::move(chat_);
	}

	void save(void);

protected:
	const td_api::message			&message_;
	KWorker					*kworker_ = nullptr;
	td_api::object_ptr<td_api::chat>	chat_ = nullptr;
	std::mutex				*chat_lock_ = nullptr;
	mysql::MySQL				*db_ = nullptr;

	/* Models */
	tgvisd::Logger::ChatFoundation		*m_chat_ = nullptr;
	tgvisd::Logger::SenderFoundation	*m_sender_ = nullptr;

	/* Primary keys. */
	uint64_t				pk_chat_id_ = 0;
	uint64_t				pk_sender_id_ = 0;
private:
	/**
	 * This will fill @m_sender_.
	 */
	bool resolve_sender(void);

	/**
	 * This will fill @m_chat_ and @chat_lock_.
	 */
	bool resolve_chat(void);

	/**
	 * This will fill @db_.
	 */
	bool resolve_db_pool(void);

	/**
	 * This will fill @pk_chat_id and pk_sender_id_.
	 */
	bool resolve_pk(void);
};

} /* namespace tgvisd::Logger */

#endif /* #ifndef TGVISD__LOGGER__MESSAGE_HPP */
