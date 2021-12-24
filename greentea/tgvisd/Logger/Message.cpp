// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#include <tgvisd/Logger/Message.hpp>

using SenderUser = tgvisd::Logger::Sender::User;
using SenderChat = tgvisd::Logger::Sender::Chat;
using ChatGroup = tgvisd::Logger::Chat::Group;
using ChatUser = tgvisd::Logger::Chat::User;

namespace tgvisd::Logger {

Message::Message(KWorker *kworker, const td_api::message &message):
	message_(message),
	kworker_(kworker)
{

}

Message::~Message(void)
{
	if (m_sender_) {
		delete m_sender_;
		m_sender_ = nullptr;
	}

	if (m_chat_) {
		delete m_chat_;
		m_chat_ = nullptr;
	}
}

bool Message::resolve_chat(void)
{
	if (!chat_) {
		chat_ = kworker_->getChat(message_.chat_id_);
		if (unlikely(!chat_)) {
			pr_err("get_chat(): "
			       "Could not get chat from message object %ld",
			       message_.id_);
			return false;
		}
	}

	if (!chat_lock_) {
		chat_lock_ = kworker_->getChatLock(chat_->id_);
		if (unlikely(!chat_lock_)) {
			pr_err("get_chat(): "
			       "Could not get chat lock (%ld) [%s]",
			       chat_->id_, chat_->title_.c_str());
			return false;
		}
	}

	switch (chat_->type_->get_id()) {
	case td_api::chatTypeBasicGroup::ID:
	case td_api::chatTypeSupergroup::ID:
		m_chat_ = new ChatGroup(kworker_, *chat_);
		break;
	case td_api::chatTypeSecret::ID:
	case td_api::chatTypePrivate::ID:
		m_chat_ = new ChatUser(kworker_, *chat_);
		break;
	default:
		pr_err("Invalid chat type on resolve_sender()");
		return false;
	}

	return true;
}

bool Message::resolve_sender(void)
{
	const auto &s = message_.sender_;

	switch (s->get_id()) {
	case td_api::messageSenderUser::ID:
		m_sender_ = new SenderUser(kworker_, *s);
		break;
	case td_api::messageSenderChat::ID:
		m_sender_ = new SenderChat(kworker_, *s);
		break;
	default:
		pr_err("Invalid sender type on resolve_sender()");
		return false;
	}

	return true;
}

bool Message::resolve_db_pool(void)
{
	db_ = kworker_->getDbPool();
	if (unlikely(!db_))
		return false;

	assert(m_chat_);
	assert(m_sender_);
	m_chat_->setDbPool(db_);
	m_sender_->setDbPool(db_);
	return true;
}

bool Message::resolve_pk(void)
	__acquires(chat_lock_)
	__releases(chat_lock_)
{
	bool ret = false;
	assert(m_chat_);
	assert(m_sender_);
	assert(chat_lock_);

	chat_lock_->lock();

	pk_chat_id_ = m_chat_->getPK();
	if (unlikely(!pk_chat_id_))
		goto out;

	pk_sender_id_ = m_sender_->getPK();
	if (unlikely(!pk_sender_id_))
		goto out;

	ret = true;
out:
	chat_lock_->unlock();
	return ret;
}

void Message::save(void)
{
	if (!resolve_sender())
		return;

	if (!resolve_chat())
		return;

	if (!resolve_db_pool())
		return;

	if (!resolve_pk())
		return;
}

} /* namespace tgvisd::Logger */
