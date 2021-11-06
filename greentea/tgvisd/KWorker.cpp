// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
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
#include <mysql/MySQL.hpp>
#include <tgvisd/common.hpp>
#include <condition_variable>
#include <tgvisd/KWorker.hpp>


namespace tgvisd {


using namespace std::chrono_literals;


__cold KWorker::KWorker(Main *main, uint32_t maxThPool, uint32_t maxDbPool,
			uint32_t maxNRTasks):
	main_(main),
	maxThPool_(maxThPool),
	activeThPool_(0)
{
	uint32_t i;

	initMySQLConfig();

	thPool_ = new thpool[maxThPool];
	dbPool_ = new dbpool[maxDbPool];

	for (i = maxThPool; i--;) {
		thPool_[i].idx  = i;
		thPool_[i].stop = false;
		thPoolStk_.push(i);
	}

	for (i = maxDbPool; i--;) {
		dbPool_[i].idx = i;
		dbPoolStk_.push(i);
	}

	tasks_ = new task_work[maxNRTasks];
	for (i = maxNRTasks; i--;) {
		tasks_[i].idx = i;
		freeTask_.push(i);
	}

	return;
enomem:
	cleanUp();
	throw std::bad_alloc();
}


__hot int KWorker::submitTaskWork(struct task_work *tw)
	__acquires(&taskLock_)
	__releases(&taskLock_)
{
	uint32_t idx;

	taskLock_.lock();
	if (unlikely(freeTask_.empty())) {
		taskLock_.unlock();
		return -EAGAIN;
	}
	idx = freeTask_.top();
	freeTask_.pop();
	tasks_[idx] = *tw;
	tasks_[idx].idx = idx;
	tasksQueue_.push(idx);
	taskLock_.unlock();

	if (activeThPool_.load() <= tasksQueue_.size())
		masterCond_.notify_one();
	else
		taskCond_.notify_one();

	return 0;
}


void KWorker::putDbPool(mysql::MySQL *db)
	__acquires(&dbPoolLock_)
	__releases(&dbPoolLock_)
{
	struct dbpool *dbp;

	dbp = container_of(db, struct dbpool, db);
	dbPoolLock_.lock();
	dbPoolStk_.push(dbp->idx);
	dbPoolLock_.unlock();
}


mysql::MySQL *KWorker::getDbPool(void)
	__acquires(&dbPoolLock_)
	__releases(&dbPoolLock_)
{
	uint32_t idx;
	mysql::MySQL *ret;

	dbPoolLock_.lock();
	if (unlikely(dbPoolStk_.empty())) {
		dbPoolLock_.unlock();
		return nullptr;
	}
	idx = dbPoolStk_.top();
	dbPoolStk_.pop();
	dbPoolLock_.unlock();

	ret = &dbPool_[idx].db;
	if (!ret->getConn()) {
		ret->init(sqlHost_, sqlUser_, sqlPass_, sqlDBName_);
		ret->setPort(sqlPort_);
		ret->connect();
	}
	return ret;
}


struct task_work *KWorker::getTaskWork(void)
	__acquires(&taskLock_)
	__releases(&taskLock_)
{
	uint32_t idx;
	struct task_work *ret;

	taskLock_.lock();
	if (tasksQueue_.empty()) {
		taskLock_.unlock();
		return nullptr;
	}
	idx = tasksQueue_.front();
	tasksQueue_.pop();
	ret = &tasks_[idx];
	taskLock_.unlock();

	if (unlikely(ret->idx != idx)) {
		panic("Bug ret->idx != idx");
		__builtin_unreachable();
	}
	return ret;
}


void KWorker::putTaskWork(struct task_work *tw)
	__acquires(&taskLock_)
	__releases(&taskLock_)
{
	taskLock_.lock();
	freeTask_.push(tw->idx);
	taskLock_.unlock();
	taskPutCond_.notify_one();
}


struct thpool *KWorker::getThPool(void)
{
	uint32_t idx;
	struct thpool *ret;

	thPoolLock_.lock();
	if (thPoolStk_.empty()) {
		thPoolLock_.unlock();
		return nullptr;
	}
	idx = thPoolStk_.top();
	thPoolStk_.pop();
	thPoolLock_.unlock();

	ret = &thPool_[idx];
	if (unlikely(ret->thread)) {
		panic("ret->thread is not NULL!");
		__builtin_unreachable();
	}

	ret->thread = new std::thread([this, ret]{
		this->runThreadPool(ret);
	});
	ret->setInterruptible();
	return ret;
}


void KWorker::runThreadPool(struct thpool *pool)
	__acquires(&taskLock_)
	__releases(&taskLock_)
{
	uint32_t idle_c = 0;
	struct task_work *tw;
	std::unique_lock<std::mutex> lk(taskLock_, std::defer_lock);

	activeThPool_++;
	while (!(pool->stop || shouldStop())) {
		struct tw_data data;

		tw = getTaskWork();
		if (!tw) {
			lk.lock();
			auto cvret = taskCond_.wait_for(lk, 5000ms);
			lk.unlock();

			if (cvret != std::cv_status::timeout)
				continue;
			if (++idle_c < 10)
				continue;
			goto idle_exit;
		}

		if (unlikely(!tw->func))
			continue;

		data.tw   = tw;
		data.kwrk = this;
		pool->setUninterruptible();
		tw->func(&data);
		putTaskWork(tw);
		pool->setInterruptible();
		idle_c = 0;
	}
	activeThPool_--;
	return;

idle_exit:
	pr_notice("tgvkwrk-%u is exiting due to inactivity...", pool->idx);
	pool->thread->detach();
	delete pool->thread;
	pool->thread = nullptr;

	thPoolLock_.lock();
	thPoolStk_.push(pool->idx);
	thPoolLock_.unlock();

	activeThPool_--;
}


void KWorker::runMasterKWorker(void)
	__acquires(&taskLock_)
	__releases(&taskLock_)
{
	std::unique_lock<std::mutex> lk(taskLock_, std::defer_lock);

	while (!shouldStop()) {
		uint32_t act_thread, num_of_queues;

		lk.lock();
		masterCond_.wait_for(lk, 10000ms);
		lk.unlock();

		act_thread    = activeThPool_.load();
		num_of_queues = tasksQueue_.size();

		if (num_of_queues >= act_thread && act_thread < maxThPool_) {
			uint32_t i, loop_c;

			loop_c = num_of_queues - act_thread;
			if (loop_c > maxThPool_)
				loop_c = maxThPool_;

			for (i = 0; i < loop_c; i++)
				getThPool();

			taskCond_.notify_all();
		} else {
			taskCond_.notify_one();
		}
	}
}


__cold void KWorker::initMySQLConfig(void)
{
	const char *tmp;

	sqlHost_ = getenv("TGVISD_MYSQL_HOST");
	if (unlikely(!sqlHost_))
		throw std::runtime_error("Missing TGVISD_MYSQL_HOST env");

	sqlUser_ = getenv("TGVISD_MYSQL_USER");
	if (unlikely(!sqlUser_))
		throw std::runtime_error("Missing TGVISD_MYSQL_USER env");

	sqlPass_ = getenv("TGVISD_MYSQL_PASS");
	if (unlikely(!sqlPass_))
		throw std::runtime_error("Missing TGVISD_MYSQL_PASS env");

	sqlDBName_ = getenv("TGVISD_MYSQL_DBNAME");
	if (unlikely(!sqlDBName_))
		throw std::runtime_error("Missing TGVISD_MYSQL_DBNAME env");

	tmp = getenv("TGVISD_MYSQL_PORT");
	if (unlikely(!tmp))
		throw std::runtime_error("Missing TGVISD_MYSQL_PORT env");

	sqlPort_ = (uint16_t)atoi(tmp);
}


__cold void KWorker::cleanUp(void)
{
	uint32_t i;

	if (thPool_) {
		for (i = 0; i < maxThPool_; i++) {
			if (thPool_[i].thread)
				thPool_[i].stop = true;
		}
		taskCond_.notify_all();

		for (i = 0; i < maxThPool_; i++) {
			if (thPool_[i].thread) {
				thPool_[i].thread->join();
				delete thPool_[i].thread;
				thPool_[i].thread = nullptr;
			}
		}
		delete[] thPool_;
		thPool_ = nullptr;
	}


	if (masterTh_) {
		masterCond_.notify_all();
		masterTh_->join();
		delete masterTh_;
		masterTh_ = nullptr;
	}


	if (dbPool_) {
		delete[] dbPool_;
		dbPool_ = nullptr;
	}

	if (tasks_) {
		delete[] tasks_;
		tasks_ = nullptr;
	}
}


} /* namespace tgvisd */
