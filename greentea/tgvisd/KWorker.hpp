// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef TGVISD__KWORKER_HPP
#define TGVISD__KWORKER_HPP

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
#include <mysql/MySQL.hpp>
#include <tgvisd/Main.hpp>
#include <tgvisd/Td/Td.hpp>
#include <tgvisd/common.hpp>
#include <condition_variable>


namespace tgvisd {


using std::chrono::duration;
using namespace std::chrono_literals;


class KWorker;


struct thpool {
	volatile bool				stop    = false;
	std::thread				*thread = nullptr;
	uint32_t				idx;

	inline void setInterruptible(void)
	{
#if defined(__linux__)
		pthread_t pt;
		char buf[sizeof("tgvkwrk-xxxxxxxxx")];
		if (unlikely(!this->thread))
			return;
		pt = this->thread->native_handle();
		snprintf(buf, sizeof(buf), "tgvkwrk-%u", idx);
		pthread_setname_np(pt, buf);
#endif
	}


	inline void setUninterruptible(void)
	{
#if defined(__linux__)
		pthread_t pt;
		char buf[sizeof("tgvkwrk-D-xxxxxxxxx")];
		if (unlikely(!this->thread))
			return;
		pt = this->thread->native_handle();
		snprintf(buf, sizeof(buf), "tgvkwrk-D-%u", idx);
		pthread_setname_np(pt, buf);
#endif
	}
};


struct dbpool {
	mysql::MySQL				db;
	uint32_t				idx;
};


struct tw_data {
	KWorker					*kwrk;
	struct task_work			*tw;
};


struct task_work {
	void					(*func)(struct tw_data *) = nullptr;
	void					*data = nullptr;
	uint32_t				idx;
};


class KWorker
{
private:
	volatile bool		stop_          = false;
	Main			*main_         = nullptr;
	struct thpool		*thPool_       = nullptr;
	struct dbpool		*dbPool_       = nullptr;
	std::thread		*masterTh_     = nullptr;
	struct task_work	*tasks_        = nullptr;
	uint32_t		maxThPool_     = 32;
	std::atomic<uint32_t>	activeThPool_  = 0;

	std::condition_variable masterCond_;
	std::mutex		thPoolLock_;
	std::stack<uint32_t>	thPoolStk_;
	std::mutex		dbPoolLock_;
	std::stack<uint32_t>	dbPoolStk_;

	std::condition_variable taskPutCond_;
	std::condition_variable	taskCond_;
	std::mutex		taskLock_;
	std::stack<uint32_t>	freeTask_;
	std::queue<uint32_t>	tasksQueue_;

	const char		*sqlHost_   = nullptr;
	const char		*sqlUser_   = nullptr;
	const char		*sqlPass_   = nullptr;
	const char		*sqlDBName_ = nullptr;
	uint16_t		sqlPort_    = 0u;


	void cleanUp(void);
	void runMasterKWorker(void);
	void runThreadPool(struct thpool *pool);
	struct task_work *getTaskWork(void);
	struct thpool *getThPool(void);
	void putTaskWork(struct task_work *tw);
	void initMySQLConfig(void);

public:
	inline ~KWorker(void)
	{
		cleanUp();
	}

	KWorker(Main *main, uint32_t maxThPool = 16, uint32_t maxDbPool = 256,
		uint32_t maxNRTasks = 4096);
	int submitTaskWork(struct task_work *tw);
	mysql::MySQL *getDbPool(void);
	void putDbPool(mysql::MySQL *db);

	template<class Rep, class Period>
	inline void waitQueue(const duration<Rep, Period> &rel_time)
	{
		std::unique_lock<std::mutex> lk(taskLock_);
		taskPutCond_.wait_for(lk, rel_time);
	}

	inline bool shouldStop(void)
	{
		return unlikely(stop_ || main_->getStop());
	}

	inline void run(void)
	{
		runMasterKWorker();
	}

	inline void stop(void)
	{
		stop_ = true;
		taskCond_.notify_all();
		masterCond_.notify_all();
		taskPutCond_.notify_all();
	}


	inline static void setMasterThreadName(std::thread *task)
	{
#if defined(__linux__)
		pthread_t pt = task->native_handle();
		pthread_setname_np(pt, "tgvkwrk-master");
#endif
	}
};


} /* namespace tgvisd */


#endif /* #ifndef TGVISD__KWORKER_HPP */
