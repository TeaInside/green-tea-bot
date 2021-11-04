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
using std::make_pair;
using std::unique_lock;
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


class WorkerPool;


static void scraper_wrk(uint32_t wrk_id, WorkerPool *wrk);


struct task_work {
	int64_t chat_id;
};


class ScraperMaster
{
private:
	Main 			*main_    = nullptr;
	Scraper			*scr_     = nullptr;
	WorkerPool		*wrkPool_ = nullptr;

public:
	inline ScraperMaster(Scraper *scr):
		main_(scr->getMain()),
		scr_(scr)
	{
	}


	inline WorkerPool *getWrkPool(void)
	{
		return wrkPool_;
	}


	inline void setWrkPool(WorkerPool *wrkPool)
	{
		wrkPool_ = wrkPool;
	}


	inline void doStop(void)
	{
		scr_->doStop();
	}


	~ScraperMaster(void);


	inline bool wrkShouldStop(void)
	{
		return scr_->getStop() || main_->getStop();
	}


	inline Main *getMain(void)
	{
		return main_;
	}
};



class WorkerPool
{
private:
	mysql::MySQL		**dbPool_   = nullptr;
	std::thread		**wrkPool_  = nullptr;
	struct task_work	*wrkTask_   = nullptr;
	volatile bool		*wrkIsBusy_ = nullptr;
	ScraperMaster		*master_    = nullptr;

public:
	mutex			wrkLock_;
	condition_variable	wrkCond_;
	uint32_t		wrkNum_ = 0;


	inline ScraperMaster *getMaster(void)
	{
		return master_;
	}


	inline struct task_work *getTask(uint32_t wrk_id)
	{
		return &wrkTask_[wrk_id];
	}

	inline std::thread *getWrkPoolThread(uint32_t wrk_id)
	{
		return wrkPool_[wrk_id];
	}


private:
	inline void initPool(void)
	{
		uint32_t i;

		wrkPool_ = (std::thread **)calloc(wrkNum_, sizeof(*wrkPool_));
		if (unlikely(!wrkPool_))
			goto out_nomem;

		wrkTask_ = (struct task_work *)calloc(wrkNum_, sizeof(*wrkTask_));
		if (unlikely(!wrkTask_)) 
			goto out_nomem;

		wrkIsBusy_ = (volatile bool *)calloc(wrkNum_, sizeof(*wrkIsBusy_));
		if (unlikely(!wrkIsBusy_))
			goto out_nomem;

		dbPool_ = (mysql::MySQL **)calloc(wrkNum_, sizeof(*dbPool_));
		if (unlikely(!dbPool_))
			goto out_nomem;

		for (i = 0; i < wrkNum_; i++)
			wrkPool_[i] = new std::thread(scraper_wrk, i, this);

		return;

	out_nomem:
		cleanUp();
		throw std::bad_alloc();
	}


public:
	void cleanUp(void)
	{
		uint32_t i;

		master_->doStop();

		if (wrkPool_) {
			for (i = 0; i < wrkNum_; i++) {
				if (wrkPool_[i]) {
					wrkPool_[i]->join();
					delete wrkPool_[i];
					wrkPool_[i] = nullptr;
				}
			}
			free(wrkPool_);
			wrkPool_ = nullptr;
		}

		if (dbPool_) {
			for (i = 0; i < wrkNum_; i++) {
				if (dbPool_[i]) {
					delete dbPool_[i];
					dbPool_ = nullptr;
				}
			}
			free(dbPool_);
			dbPool_ = nullptr;
		}

		if (wrkTask_) {
			free(wrkTask_);
			wrkTask_ = nullptr;
		}

		if (wrkIsBusy_) {
			free((bool *)wrkIsBusy_);
			wrkIsBusy_ = nullptr;
		}
	}


	inline WorkerPool(uint32_t wrkNum, ScraperMaster *master):
		master_(master),
		wrkNum_(wrkNum)
	{
		initPool();
	}


	inline ~WorkerPool(void)
	{
		cleanUp();
	}


	inline int submitWork(struct task_work *task, uint32_t timeout = 30)
	{
		uint32_t i, secs = 0;
		unique_lock lk(wrkLock_);

		do {
			if (unlikely(master_->wrkShouldStop()))
				return -EOWNERDEAD;

			for (i = 0; i < wrkNum_; i++) {
				if (wrkIsBusy_[i])
					continue;

				/*
				 * We find a free worker here. Give the
				 * work to it!
				 */
				wrkTask_[i] = *task;
				wrkIsBusy_[i] = true;
				__sync_synchronize();
				lk.unlock();
				wrkCond_.notify_all();
				return 0;
			}
			wrkCond_.wait_for(lk, 1000ms);
		} while (++secs < timeout);

		return -EAGAIN;
	}


	inline bool isWorkerBusy(uint32_t wrk_id)
	{
		return wrkIsBusy_[wrk_id];
	}


	inline void clearWork(uint32_t wrk_id)
	{
		unique_lock lk(wrkLock_);

		memset(&wrkTask_[wrk_id], 0, sizeof(*wrkTask_));
		wrkIsBusy_[wrk_id] = false;
		__sync_synchronize();
		wrkCond_.notify_all();
	}
};


ScraperMaster::~ScraperMaster(void)
{
	scr_->doStop();
	if (wrkPool_)
		wrkPool_->cleanUp();
}


static void _scraper_wrk(uint32_t wrk_id, WorkerPool *wrk, ScraperMaster *master,
			 unique_lock<mutex> &lk)
{
	struct task_work *tw;

	lk.lock();
	wrk->wrkCond_.wait_for(lk, 1000ms, [wrk_id, wrk](){
		return wrk->isWorkerBusy(wrk_id);
	});
	lk.unlock();

	tw = wrk->getTask(wrk_id);

	printf("Scraping chat_id = %ld\n", tw->chat_id);
	/* Do the work here... */
	// sleep(1);

	wrk->clearWork(wrk_id);
}


static int wait_for_pool_assignment(uint32_t wrk_id, WorkerPool *wrk,
				    ScraperMaster *master)
{
	while (unlikely(!wrk->getWrkPoolThread(wrk_id))) {
		cpu_relax();
		if (unlikely(master->wrkShouldStop()))
			return -ECONNABORTED;
	}

	return 0;
}


static void init_thread_name(uint32_t wrk_id, WorkerPool *wrk)
{
	char buf[sizeof("scraper-wrk-xxxxxxxxxx")];
	std::thread *threadPtr = wrk->getWrkPoolThread(wrk_id);

#if defined(__linux__)
	pthread_t pt = threadPtr->native_handle();
	snprintf(buf, sizeof(buf), "scraper-wrk-%u", wrk_id);
	pthread_setname_np(pt, buf);
#endif
}


static void scraper_wrk(uint32_t wrk_id, WorkerPool *wrk)
{
	unique_lock<mutex> lk(wrk->wrkLock_, std::defer_lock);
	ScraperMaster *master = wrk->getMaster();

	if (wait_for_pool_assignment(wrk_id, wrk, master))
		return;

	init_thread_name(wrk_id, wrk);

	while (likely(!master->wrkShouldStop()))
		_scraper_wrk(wrk_id, wrk, master, lk);
}


static void run_scraper(ScraperMaster *master)
{
	int64_t i;
	WorkerPool *wrkPool;
	struct task_work wrk;

	wrkPool = master->getWrkPool();

	for (i = 0; i < 1000; i++) {
		wrk.chat_id = i;
		wrkPool->submitWork(&wrk);
	}
}


void Scraper::run(void)
{
	WorkerPool *wrkPool = nullptr;
	ScraperMaster *master = nullptr;

	try {
		master  = new ScraperMaster(this);
		wrkPool = new WorkerPool(4, master);

		master->setWrkPool(wrkPool);
		while (likely(!master->wrkShouldStop()))
			run_scraper(master);

	} catch (const std::runtime_error &e) {
		pr_err("In scraper: std::runtime_error: %s", e.what());
		main_->doStop();
	} catch (const std::bad_alloc &e) {
		pr_err("In scraper: Aiee... ENOMEM!");
		main_->doStop();
	}

	if (master)
		delete master;

	if (wrkPool)
		delete wrkPool;
}


} /* namespace tgvisd */
