// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#if defined(__linux__)
	#include <unistd.h>
	#include <pthread.h>
#endif

#include <stack>
#include <mutex>
#include <queue>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <tgvisd/Td/Td.hpp>
#include <tgvisd/common.hpp>
#include <tgvisd/KWorker.hpp>
#include <tgvisd/Scraper.hpp>


namespace tgvisd {


Scraper::Scraper(Main *main):
	main_(main),
	kworker_(main->getKWorker())
{
}


__hot void Scraper::run(void)
{
	while (!main_->isReady()) {
		if (shouldStop())
			return;
		sleep(1);
	}

	while (!shouldStop()) {
		_run();
		sleep(1);
	}
}


__hot void Scraper::_run(void)
{
	int32_t i;
	int64_t chat_id;

	pr_notice("Getting chat list...");
	auto chats = kworker_->getChats(nullptr, 300);
	if (unlikely(!chats))
		return;

	for (i = 0; i < chats->total_count_; i++) {
		if (shouldStop())
			break;

		chat_id = chats->chat_ids_[i];
		auto chat = kworker_->getChat(chat_id);

		if (unlikely(!chat))
			continue;

		if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID)
			continue;

		pr_notice("Submitting visit to %ld...", chat_id);
		visit_chat(chat);
	}
}


__hot void Scraper::visit_chat(td_api::object_ptr<td_api::chat> &chat)
{
	int ret;
	struct task_work tw;

	tw.func = [this](struct tw_data *data){
		this->_visit_chat(data, data->tw->data);
	};
	tw.data = std::move(chat);

	do {
		if (shouldStop())
			break;

		ret = kworker_->submitTaskWork(&tw);
		if (ret == -EAGAIN)
			sleep(1);

	} while (ret == -EAGAIN);
}


__hot void Scraper::_visit_chat(struct tw_data *data,
				td_api::object_ptr<td_api::chat> &chat)
{
	int32_t count, i;
	struct thpool *current = data->current;
	std::mutex *chat_lock;

	pr_notice("Scraping messages from (%ld) [%s]...", chat->id_,
		  chat->title_.c_str());

	chat_lock = kworker_->getChatLock(chat->id_);
	if (unlikely(!chat_lock)) {
		pr_notice("Could not get chat lock (%ld) [%s]", chat->id_,
			  chat->title_.c_str());
		return;
	}

	auto messages = kworker_->getChatHistory(chat->id_, 0, 0, 300);
	if (unlikely(!messages)) {
		pr_notice("Could not get message history from (%ld) [%s]",
			  chat->id_, chat->title_.c_str());
		return;
	}

	count = messages->total_count_;
	current->setInterruptible();
	for (i = 0; i < count; i++) {
		if (shouldStop())
			break;

		auto &msg = messages->messages_[i];
		if (unlikely(!msg))
			continue;

		current->setUninterruptible();
		chat_lock->lock();
		extract_msg_content(msg);
		chat_lock->unlock();
		current->setInterruptible();
	}
}


void Scraper::extract_msg_content(td_api::object_ptr<td_api::message> &msg)
{
	auto &content = msg->content_;

	if (unlikely(!content))
		return;

	if (content->get_id() != td_api::messageText::ID)
		return;

	auto &text = static_cast<td_api::messageText &>(*content);
	pr_notice("text = %s", text.text_->text_.c_str());
	sleep(3);
}


} /* namespace tgvisd */
