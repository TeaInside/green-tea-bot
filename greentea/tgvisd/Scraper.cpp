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
#include <cinttypes>
#include <tgvisd/Td/Td.hpp>
#include <tgvisd/common.hpp>
#include <tgvisd/KWorker.hpp>
#include <tgvisd/Scraper.hpp>
#include <tgvisd/Logger/Message.hpp>


namespace tgvisd {

__cold Scraper::Scraper(Main *main):
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

	while (1) {
		if (runFastPhase())
			break;
		if (runSlowPhase())
			break;
	}
}

/*
 * Returns true  = stop.
 * Returns false = continue.
 */
bool Scraper::runFastPhase(void)
{
	bool stop = false;
	uint32_t i;

	for (i = 0; i < 50; i++) {
		if ((stop = shouldStop()))
			break;
		_run();
		sleep(1);
	}
	return stop;
}

/*
 * Returns true  = stop.
 * Returns false = continue.
 */
bool Scraper::runSlowPhase(void)
{
	bool stop = false;
	uint32_t i, j;

	for (i = 0; i < 100; i++) {
		if ((stop = shouldStop()))
			break;
		_run();

		for (j = 0; j < 15; j++) {
			if ((stop = shouldStop()))
				goto out;
			sleep(1);
		}
	}

out:
	return stop;
}

__hot void Scraper::_run(void)
{
	int32_t i, j;
	int64_t chat_id;

	pr_notice("Getting chat list...");
	auto chats = kworker_->getChats(nullptr, 500);
	if (unlikely(!chats))
		return;

	for (i = 0; i < chats->total_count_; i++) {
		if (shouldStop())
			break;

		chat_id = chats->chat_ids_[i];
		pr_notice("Scraping %ld...", chat_id);
		for (j = 0; j < 5; j++) {
			auto chat = kworker_->getChat(chat_id);

			if (unlikely(!chat))
				continue;

			if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID)
				continue;

			if (shouldStop())
				return;
			visit_chat(chat);
		}
		sleep(1);
	}
}

struct scraper_payload {
	td_api::object_ptr<td_api::chat>	chat;
};

static void scraper_payload_deleter(void *p)
{
	delete (struct scraper_payload *)p;
}

__hot void Scraper::visit_chat(td_api::object_ptr<td_api::chat> &chat)
{
	int ret;
	struct task_work tw;
	struct scraper_payload *payload;

	payload = new struct scraper_payload;
	payload->chat = std::move(chat);

	tw.func = [this](struct tw_data *data){
		struct scraper_payload *payload;

		payload = (struct scraper_payload *)data->tw->payload;
		this->_visit_chat(data, payload->chat);
	};
	tw.payload = (void *)payload;
	tw.deleter = scraper_payload_deleter;

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
	int64_t startMsgId = 0;
	td_api::object_ptr<td_api::error> err;
	static std::mutex startMsgIdMapLock;
	static std::unordered_map<int64_t, int64_t> startMsgIdMap;

	pr_notice("Scraping messages from (%ld) [%s]...", chat->id_,
		  chat->title_.c_str());

	chat_lock = kworker_->getChatLock(chat->id_);
	if (unlikely(!chat_lock)) {
		pr_notice("Could not get chat lock (%ld) [%s]", chat->id_,
			  chat->title_.c_str());
		return;
	}

	startMsgIdMapLock.lock();
	const auto &it = startMsgIdMap.find(chat->id_);
	if (it != startMsgIdMap.end()) {
		startMsgId = startMsgIdMap[chat->id_];
		startMsgIdMap[chat->id_] += 50;
	} else {
		startMsgIdMap[chat->id_] = 1;
	}
	startMsgIdMapLock.unlock();

	pr_notice("Scraping %ld with startMsgId = %ld", chat->id_, startMsgId);

	auto messages = kworker_->getChatHistory(chat->id_,
						 startMsgId << 20u, -10, 100,
						 false, &err);
	if (unlikely(!messages)) {
		pr_notice("Could not get message history from (%ld) [%s]: %s",
			  chat->id_, chat->title_.c_str(),
			  err ? to_string(err).c_str() : "no err info");
		return;
	}

	count = messages->total_count_;
	current->setInterruptible();
	for (i = 0; i < count; i++) {
		tgvisd::Logger::Message *m_msg;

		if (shouldStop())
			break;

		auto &msg = messages->messages_[i];
		if (unlikely(!msg))
			continue;

		current->setUninterruptible();
		m_msg = new tgvisd::Logger::Message(kworker_, *msg);
		m_msg->set_chat(std::move(chat));
		m_msg->set_chat_lock(chat_lock);
		m_msg->save();
		chat = m_msg->get_chat();
		delete m_msg;
		current->setInterruptible();
	}
}

} /* namespace tgvisd */
