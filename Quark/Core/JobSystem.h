#pragma once
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>

#include "Quark/Core/Base.h"
#include "Quark/Core/Assert.h"

namespace quark {

class JobSystem 
{
public:
	using JobFunction = std::function<void()>;

	struct Counter
	{
		std::atomic<uint32_t> count;
	};

	JobSystem();
	~JobSystem();

	void Execute(const JobFunction& jobFunc, Counter* counter = nullptr);

	bool IsBusy(const Counter& conter) const;

	void Wait(const Counter* counter, uint32_t numCounters);

private:
	void RunThread(uint32_t threadId);

	struct Job
	{
		JobFunction jobFunction;
		Counter* counter = nullptr;

		bool isValid() const { return jobFunction != nullptr; }
	};

	class JobQueue 
	{
	public:
		JobQueue();

		// True if managed to acquire a lock on the queue mutex and pop a Job from the queue.
		bool TryPop(Job& outPoppedJob);

		bool BlockingPop(Job& outPoppedJob);

		// True if managed to acquire a lock on the queue mutex and push a Job into the queue.
		bool TryPush(const Job& job);

		bool BlockingPush(const Job& job);

		void SignalWorkDone();

	private:
		std::thread::id m_threadId;
		std::mutex m_queueMutex;
		std::condition_variable m_condition;
		std::queue<Job> m_jobQueue;

		bool m_isWorkDone = false;
	};


	uint32_t m_numWorkerThreads;

	std::vector<JobQueue> m_jobQueues;

	std::vector<std::thread> m_workerThreads;

	std::atomic<uint32_t> m_pushQueueId{ 0 };
};


};