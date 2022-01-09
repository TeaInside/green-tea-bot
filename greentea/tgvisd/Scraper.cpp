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

using LogMessage = tgvisd::Logger::Message;

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

	for (i = 0; i < 500; i++) {
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

	for (i = 0; i < 10; i++) {
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
	int32_t i, count;

	pr_notice("Getting chat list...");
	auto chats = kworker_->getChats(nullptr, 500);
	if (unlikely(!chats))
		return;

	count = chats->total_count_;
	for (i = 0; i < count; i++) {
		int64_t chat_id;

		if (shouldStop())
			return;

		chat_id = chats->chat_ids_[i];
		auto chat = kworker_->getChat(chat_id);
		if (unlikely(!chat))
			continue;

		if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID)
			continue;

		if (shouldStop())
			return;

		pr_notice("Visiting %lld...", (long long) chat_id);
		visit_chat(chat);
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

	while (1) {
		if (shouldStop())
			break;

		ret = kworker_->submitTaskWork(&tw);
		if (ret != -EAGAIN)
			break;

		sleep(1);
	}
}

__hot void Scraper::_visit_chat(struct tw_data *data,
				td_api::object_ptr<td_api::chat> &chat)
{
	int32_t count, i;
	std::mutex *chat_lock;
	int64_t startMsgId = 0;
	int64_t shiftedMsgId;
	td_api::object_ptr<td_api::error> err;
	struct thpool *current = data->current;
	static std::mutex minMaxMapLock;
	static std::unordered_map<int64_t, uint8_t> minMaxMap;

	/* false = min; true = max;*/
	bool minMax;

	minMaxMapLock.lock();
	if (unlikely(minMaxMap.find(chat->id_) == minMaxMap.end())) {
		minMaxMap.emplace(chat->id_, 0);
		minMax = true;
	} else {
		uint8_t nn = ++minMaxMap[chat->id_];

		/* Dominate min. */
		minMax = ((nn % 5) == 0);
	}
	minMaxMapLock.unlock();


	pr_notice("Scraping messages from (%lld) [%s]...",
		  (long long) chat->id_, chat->title_.c_str());

	chat_lock = kworker_->getChatLock(chat->id_);
	if (unlikely(!chat_lock)) {
		pr_notice("Could not get chat lock (%lld) [%s]",
			  (long long) chat->id_, chat->title_.c_str());
		return;
	}

	/*
	 * @startMsgId will be zero when we don't have any message from
	 * the corresponding @chat->id_ in our database.
	 */
	chat_lock->lock();
	startMsgId = LogMessage::getMinMaxMsgIdByTgGroupId(kworker_, chat->id_,
							   minMax);
	chat_lock->unlock();
	if (unlikely(startMsgId == -1)) {
		pr_err("Cannot check last message id from (%lld)",
		       (long long) chat->id_);
		return;
	}

	if (startMsgId > 0) {
		if (!minMax)
			startMsgId += 1;
		else if (startMsgId > 30)
			startMsgId -= 30;
	}

	pr_notice("Scraping %lld with startMsgId = %lld; (%s)",
		  (long long) chat->id_, (long long) startMsgId,
		  (minMax ? "max" : "min"));

	shiftedMsgId = startMsgId ? startMsgId << 20ull : startMsgId;
	auto messages = kworker_->getChatHistory(chat->id_,
						 shiftedMsgId,
						 -30,
						 100,
						 false,
						 &err);
	if (unlikely(!messages)) {
out_get_msg_fail:
		pr_notice("Could not get message history from (%lld) [%s]: %s",
			  (long long) chat->id_, chat->title_.c_str(),
			  err ? to_string(err).c_str() : "no err info");
		return;
	}

	count = messages->total_count_;
	if (unlikely(count == 0))
		goto out_get_msg_fail;

	current->setInterruptible();
	for (i = 0; i < count; i++) {
		LogMessage *m_msg;

		if (shouldStop())
			break;

		auto &msg = messages->messages_[i];
		if (unlikely(!msg))
			continue;

		current->setUninterruptible();
		m_msg = new LogMessage(kworker_, *msg);
		m_msg->set_chat(std::move(chat));
		m_msg->set_chat_lock(chat_lock);
		m_msg->save();
		chat = m_msg->get_chat();
		delete m_msg;
		current->setInterruptible();
	}

	pr_notice("Scraped %lld messages from %lld with startMsgId = %lld; (%s)",
		  (long long) count, (long long) chat->id_,
		  (long long) startMsgId, (minMax ? "max" : "min"));
}

} /* namespace tgvisd */
