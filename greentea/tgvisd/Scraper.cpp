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
	int64_t		chat_id;
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
		tasks_[kwrk_id].tw = *tw;
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


	inline bool kworkerShouldStop(void)
	{
		return unlikely(scraper_->getStop() || main_->getStop());
	}


	inline void run(void)
	{
		kwrkPool_ = new KWorkerPool(4, this);
		kwrkPool_->spawn();
		run_kworker_event_loop(this);
		scraper_->doStop();
		delete kwrkPool_;
		kwrkPool_ = nullptr;
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


static void _run_kworker(uint32_t kwrk_id, KWorkerPool *kwrk_pool,
			 std::unique_lock<mutex> &lk)
{
	struct task_work_list *twl;

	lk.lock();
	kwrk_pool->ftCond_.wait_for(lk, 1000ms);
	lk.unlock();

	twl = kwrk_pool->getTW(kwrk_id);
	if (!twl->is_used)
		return;

	printf("Scraping chat_id = %ld\n", twl->tw.chat_id);
	kwrk_pool->putTaskWork(kwrk_id);
}


static void run_kworker(uint32_t kwrk_id, KWorkerPool *kwrk_pool)
{
	KWorker *kwrk = kwrk_pool->kworker_;
	std::unique_lock<mutex> lk(kwrk_pool->ftLock_, std::defer_lock);

	wait_for_pool_assignment(kwrk_id, kwrk_pool);

	while (!kwrk->kworkerShouldStop())
		_run_kworker(kwrk_id, kwrk_pool, lk);
}


static void run_kworker_event_loop(KWorker *kwrk)
{
	int64_t i = 0;
	struct task_work tw;
	KWorkerPool *kwrk_pool;

	kwrk_pool = kwrk->getKWorkerPool();
	for (i = 0; i < 1000; i++) {
		int ret;

		tw.chat_id = i;
	submit_again:
		if (kwrk->kworkerShouldStop())
			return;

		ret = kwrk_pool->submitTaskWork(&tw);
		if (ret == -EAGAIN) {
			pr_notice("Threads are all busy...");
			goto submit_again;
		}
	}
}


} /* namespace tgvisd */
