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
#include <unordered_map>
#include <condition_variable>
#include <tgvisd/Scraper.hpp>
#include <mysql/MySQL.hpp>


namespace tgvisd {


using std::mutex;
using std::unordered_map;
using std::condition_variable;
using namespace std::chrono_literals;


Scraper::Scraper(Main *main, std::thread *threadPtr):
	main_(main)
{
	assert(threadPtr);

#if defined(__linux__)
	pthread_t pt = threadPtr->native_handle();
	pthread_setname_np(pt, "scraper-master");
#endif
}


Scraper::~Scraper(void)
{
	pr_notice("Scraper task work is exiting...");
}


class KWorker;
class KWorkerPool;


static void run_kworker(uint32_t kwrk_id, KWorkerPool *kwrk_pool);


static void run_kworker_event_loop(KWorker *kwrk);


struct task_work {
	td_api::object_ptr<td_api::chat>	chat;
};


struct task_work_list {
	bool			is_used;
	struct task_work	tw;
};


class KWorkerPool
{
public:
	KWorker			*kworker_   = nullptr;
	mutex			ftLock_;
	condition_variable	ftCond_;
	std::stack<uint16_t>	freeThreads_;

private:
	std::thread		**threads_  = nullptr;
	mysql::MySQL		**dbPool_   = nullptr;
	struct task_work_list	*tasks_     = nullptr;
	uint16_t 		wrkNum_     = 0u;


	inline void cleanUp(void)
	{
		uint16_t i;

		if (threads_) {
			for (i = 0; i < wrkNum_; i++) {
				if (threads_[i]) {
					threads_[i]->join();
					delete threads_[i];
					threads_[i] = nullptr;
				}
			}
			free(threads_);
			threads_ = nullptr;
		}


		if (dbPool_) {
			for (i = 0; i < wrkNum_; i++) {
				if (dbPool_[i]) {
					delete dbPool_[i];
					dbPool_[i] = nullptr;
				}
			}
			free(dbPool_);
			dbPool_ = nullptr;
		}

		if (tasks_) {
			free(tasks_);
			tasks_ = nullptr;
		}
	}


public:
	inline KWorkerPool(uint32_t wrkNum, KWorker *kworker):
		kworker_(kworker),
		wrkNum_(wrkNum)
	{
	}


	inline KWorker *getKWorker(void)
	{
		return kworker_;
	}


	inline std::thread *getThread(uint16_t kwrk_id)
	{
		return threads_[kwrk_id];
	}


	inline void spawn(void)
	{
		uint16_t i;

		threads_ = (std::thread **)calloc(wrkNum_, sizeof(*threads_));
		if (unlikely(!threads_))
			goto err_nomem;

		dbPool_ = (mysql::MySQL **)calloc(wrkNum_, sizeof(*dbPool_));
		if (unlikely(!dbPool_))
			goto err_nomem;

		tasks_ = (struct task_work_list *)calloc(wrkNum_, sizeof(*tasks_));
		if (unlikely(!tasks_))
			goto err_nomem;

		for (i = 0; i < wrkNum_; i++) {
			threads_[i] = new std::thread(run_kworker, i, this);
			freeThreads_.push(i);
		}

		return;
	err_nomem:
		cleanUp();
		throw std::bad_alloc();
	}


	inline struct task_work_list *getTW(uint16_t kwrk_id)
	{
		return &tasks_[kwrk_id];
	}


	inline int submitTaskWork(struct task_work *tw)
	{
		uint16_t kwrk_id;

		ftLock_.lock();
		if (freeThreads_.empty()) {
			ftLock_.unlock();
			return -EAGAIN;
		}

		kwrk_id = freeThreads_.top();
		freeThreads_.pop();
		ftLock_.unlock();

		tasks_[kwrk_id].is_used = true;
		tasks_[kwrk_id].tw = std::move(*tw);
		ftCond_.notify_all();
		return 0;
	}


	inline void putTaskWork(uint16_t kwrk_id)
	{
		ftLock_.lock();
		tasks_[kwrk_id].is_used = false;
		freeThreads_.push(kwrk_id);
		ftLock_.unlock();

		ftCond_.notify_all();
	}


	inline void waitForWorker(void)
	{
		std::unique_lock<mutex> lk(ftLock_);
		ftCond_.wait_for(lk, 1000ms);
	}


	inline ~KWorkerPool(void)
	{
		cleanUp();
	}
};


class KWorker
{
private:
	tgvisd::Td::Td		*td_        = nullptr;
	Main			*main_      = nullptr;
	Scraper			*scraper_   = nullptr;
	KWorkerPool		*kwrkPool_  = nullptr;

	mutex				clmLock;
	unordered_map<int64_t, mutex *>	chatLockMap_;
	volatile bool			dropChatLock_ = false;
public:
	inline tgvisd::Td::Td *getTd(void)
	{
		return td_;
	}


	inline Main *getMain(void)
	{
		return main_;
	}


	inline Scraper *getScraper(void)
	{
		return scraper_;
	}


	inline KWorkerPool *getKWorkerPool(void)
	{
		return kwrkPool_;
	}


	inline KWorker(Scraper *scraper):
		scraper_(scraper)
	{
		main_ = scraper->getMain();
		td_   = main_->getTd();
	}


	inline ~KWorker(void)
	{
		dropChatLock_ = true;
		for (auto &i: chatLockMap_) {
			mutex *mut = i.second;
			if (!mut)
				continue;

			mut->lock();
			mut->unlock();
			delete mut;
			i.second = nullptr;
		}
	}


	inline mutex *getChatLock(int64_t chat_id)
		__acquires(&clmLock)
		__releases(&clmLock)
	{
		mutex *ret;

		if (unlikely(dropChatLock_))
			return nullptr;

		clmLock.lock();
		const auto &i = chatLockMap_.find(chat_id);
		if (i == chatLockMap_.end()) {
			ret = new mutex;
			chatLockMap_.emplace(chat_id, ret);
		} else {
			ret = i->second;
		}
		clmLock.unlock();
		return ret;
	}


	inline bool kworkerShouldStop(void)
	{
		return unlikely(scraper_->getStop() || main_->getStop());
	}


	inline void run(void)
	{
		kwrkPool_ = new KWorkerPool(10, this);
		kwrkPool_->spawn();
		run_kworker_event_loop(this);
		scraper_->doStop();
		delete kwrkPool_;
		kwrkPool_ = nullptr;
	}


	static constexpr uint32_t query_sync_timeout = 150;


	inline td_api::object_ptr<td_api::chats> getChats(
		td_api::object_ptr<td_api::ChatList> &&chatList, int32_t limit)
	{
		return td_->send_query_sync<td_api::getChats, td_api::chats>(
			td_api::make_object<td_api::getChats>(
				std::move(chatList),
				limit
			),
			query_sync_timeout
		);
	}


	inline td_api::object_ptr<td_api::chat> getChat(int64_t chat_id)
	{
		return td_->send_query_sync<td_api::getChat, td_api::chat>(
			td_api::make_object<td_api::getChat>(chat_id),
			query_sync_timeout
		);
	}


	inline td_api::object_ptr<td_api::messages> getChatHistory(
				int64_t chat_id,
				int64_t from_msg_id,
				int32_t offset,
				int32_t limit,
				bool only_local = false)
	{
		return td_->send_query_sync<td_api::getChatHistory, td_api::messages>(
			td_api::make_object<td_api::getChatHistory>(
				chat_id,
				from_msg_id,
				offset,
				limit,
				only_local
			),
			query_sync_timeout
		);
	}


	uint64_t touchGroup(td_api::object_ptr<td_api::chat> &chat);


	inline uint64_t touchGroupByChatId(int64_t chat_id)
	{
		auto chat = getChat(chat_id);
		if (unlikely(!chat)) {
			pr_err("[tid:%d] touchGroupByChatId() cannot find chat "
			       "%ld", gettid(), chat_id);
			return 0;
		}
		return touchGroup(chat);
	}
};


void Scraper::run(void)
{
	KWorker *kworker = nullptr;

	try {
		kworker = new KWorker(this);
		kworker->run();
	} catch (const std::runtime_error &e) {
		pr_err("In scraper: std::runtime_error: %s", e.what());
		main_->doStop();
	} catch (const std::bad_alloc &e) {
		pr_err("In scraper: Aiee... ENOMEM!");
		main_->doStop();
	}

	if (kworker)
		delete kworker;
}


uint64_t KWorker::touchGroup(td_api::object_ptr<td_api::chat> &chat)
{
	if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID) {
		pr_err("Chat %ld (%s) is not a super group", chat->id_,
		       chat->title_.c_str());
		return 0;
	}
	return 0;
}


static void wait_for_pool_assignment(uint32_t kwrk_id, KWorkerPool *kwrk_pool)
{
#if defined(__linux__)	
	pthread_t pt;
#endif
	std::thread *current;
	char buf[sizeof("scraper-wrk-xxxxxxx")];

	do {
		current = kwrk_pool->getThread(kwrk_id);
		cpu_relax();
	} while (unlikely(!current));

#if defined(__linux__)
	pt = current->native_handle();
	snprintf(buf, sizeof(buf), "scraper-kwrk-%u", kwrk_id);
	pthread_setname_np(pt, buf);
#endif
}


static void _scrape_chat_history(KWorker *kwrk,
				 td_api::object_ptr<td_api::message> &msg)
{
	auto &content = msg->content_;

	if (unlikely(!content))
		return;

	if (content->get_id() != td_api::messageText::ID)
		return;

	auto &text = static_cast<td_api::messageText &>(*content);
	// pr_notice("text = %s", text.text_->text_.c_str());
	__asm__ volatile(""::"m"(text):"memory");
}


static void scrape_chat_history(KWorker *kwrk,
				td_api::object_ptr<td_api::chat> &chat)
{
	int32_t i, count;

	pr_debug("[tid=%d] Touching group %ld (%s)...", gettid(), chat->id_,
		 chat->title_.c_str());

	// kwrk->touchChat(chat);


	auto messages = kwrk->getChatHistory(chat->id_, 0, 0, 300);
	if (unlikely(!messages)) {
		pr_notice("[tid=%d] getChatHistory %ld is NULL", gettid(),
			  chat->id_);
		return;
	}

	count = messages->total_count_;
	for (i = 0; i < count; i++) {
		auto &msg = messages->messages_[i];
		if (unlikely(!msg))
			continue;

		_scrape_chat_history(kwrk, msg);
	}

	pr_notice("[tid=%d] Scraping %ld finished", gettid(), chat->id_);
}


static void __run_kworker(uint32_t kwrk_id, KWorkerPool *kwrk_pool,
			  KWorker *kwrk, struct task_work *tw)
{
	int64_t chat_id;
	mutex *chatLock;

	chat_id  = tw->chat->id_;
	chatLock = kwrk->getChatLock(chat_id);
	if (unlikely(!chatLock)) {
		pr_err("[tid=%d] Cannot get lock for chat_id = %ld", gettid(),
		       chat_id);
		return;
	}

	scrape_chat_history(kwrk, tw->chat);
}


static void _run_kworker(uint32_t kwrk_id, KWorkerPool *kwrk_pool,
			 KWorker *kwrk, std::unique_lock<mutex> &lk)
	__acquires(&lk)
	__releases(&lk)
{
	struct task_work_list *twl;

	lk.lock();
	kwrk_pool->ftCond_.wait_for(lk, 1000ms);
	lk.unlock();

	twl = kwrk_pool->getTW(kwrk_id);
	if (!twl->is_used)
		return;

	__run_kworker(kwrk_id, kwrk_pool, kwrk, &twl->tw);
	kwrk_pool->putTaskWork(kwrk_id);
}


static void run_kworker(uint32_t kwrk_id, KWorkerPool *kwrk_pool)
{
	KWorker *kwrk = kwrk_pool->kworker_;
	std::unique_lock<mutex> lk(kwrk_pool->ftLock_, std::defer_lock);

	wait_for_pool_assignment(kwrk_id, kwrk_pool);

	while (!kwrk->kworkerShouldStop())
		_run_kworker(kwrk_id, kwrk_pool, kwrk, lk);
}


static int __run_kworker_event_loop(KWorker *kwrk, KWorkerPool *kwrk_pool,
				    td_api::object_ptr<td_api::chat> chat)
{
	int ret;
	bool dbg = false;
	struct task_work tw;

	tw.chat = std::move(chat);
	while (1) {

		if (kwrk->kworkerShouldStop())
			return -EOWNERDEAD;

		if (!dbg) {
			pr_debug("scraper-master: Submitting %ld",
				 tw.chat->id_);
			dbg = true;
		}

		ret = kwrk_pool->submitTaskWork(&tw);
		if (ret == -EAGAIN) {
			kwrk_pool->waitForWorker();
		} else {
			break;
		}
	}

	return 0;
}


static void _run_kworker_event_loop(KWorker *kwrk, KWorkerPool *kwrk_pool)
{
	int32_t i;
	int64_t chat_id;
	auto chats = kwrk->getChats(nullptr, 300);

	if (unlikely(!chats))
		return;

	for (i = 0; i < chats->total_count_; i++) {
		chat_id = chats->chat_ids_[i];
		auto chat = kwrk->getChat(chat_id);

		if (unlikely(!chat))
			continue;
		if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID)
			continue;
		if (__run_kworker_event_loop(kwrk, kwrk_pool, std::move(chat)))
			break;
	}
}


static void run_kworker_event_loop(KWorker *kwrk)
{
	KWorkerPool *kwrk_pool;

	kwrk_pool = kwrk->getKWorkerPool();
	while (!kwrk->kworkerShouldStop())
		_run_kworker_event_loop(kwrk, kwrk_pool);
}


} /* namespace tgvisd */
