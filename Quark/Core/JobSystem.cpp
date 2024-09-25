#include "Quark/qkpch.h"
#include "Quark/Core/JobSystem.h"

namespace quark {

JobSystem::JobQueue::JobQueue()
	: m_threadId{ std::this_thread::get_id() }
{

}

bool JobSystem::JobQueue::TryPop(Job& outPoppedJob)
{
	// Try to acquire the queue lock
	std::unique_lock<std::mutex> mutexLock{ m_queueMutex, std::try_to_lock };

	if (!mutexLock || m_jobQueue.empty()) // when we check if queue empty it means we already managed to acquire the lock!
		return false;

	outPoppedJob = std::move(m_jobQueue.front());
	m_jobQueue.pop();

	return true;
}

bool JobSystem::JobQueue::BlockingPop(Job& outPoppedJob)
{
	std::unique_lock<std::mutex> mutexLock{ m_queueMutex };

	// If a blocking pop is ordered with an empty queue, the thread gets put to sleep,
	// waiting for the condition variable to be signaled
	while (m_jobQueue.empty() && !m_isWorkDone)
		m_condition.wait(mutexLock);


	if (m_jobQueue.empty()) // If queue empty and work is done, return false
		return false;

	outPoppedJob = std::move(m_jobQueue.front());
	m_jobQueue.pop();

	return true;
}

bool JobSystem::JobQueue::TryPush(const Job& job)
{
	// try acquire the queue lock
	{
		std::unique_lock<std::mutex> lock(m_queueMutex, std::try_to_lock);

		if (!lock)
			return false;

		m_jobQueue.emplace(job);
	}

	// notify the condition variable that a job has been added
	m_condition.notify_one();

	return true;

}

bool JobSystem::JobQueue::BlockingPush(const Job& job)
{
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_jobQueue.emplace(job);
	}

	m_condition.notify_one();

	return true;
}

void JobSystem::JobQueue::SignalWorkDone()
{
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_isWorkDone = true;
	}

	m_condition.notify_all();
};

JobSystem::JobSystem()
{
	m_numWorkerThreads = std::thread::hardware_concurrency() - 1; // Leave one thread for the main thread

	// Initialize the job queues
	m_jobQueues = std::vector<JobQueue>(m_numWorkerThreads);

	// Start the worker threads
	m_workerThreads.reserve(m_numWorkerThreads);
	for (uint32_t i = 0; i < m_numWorkerThreads; ++i)
	{
		m_workerThreads.emplace_back([&, i]()
		{
			RunThread(i);
		});
	}

}

JobSystem::~JobSystem()
{
	// Signal all worker threads to stop working
	for (auto& queue : m_jobQueues)
		queue.SignalWorkDone();

	// Wait for all worker threads to finish
	for (auto& thread : m_workerThreads)
		thread.join();
}

void JobSystem::Execute(const JobFunction& jobFunc, Counter* counter)
{
	Job job;
	job.jobFunction = jobFunc;
	job.counter = counter;

	if (counter)
	{
		// Increment the counter
		counter->count.fetch_add(1, std::memory_order_relaxed);
	}

	uint32_t queueIndex = m_pushQueueId++;
	uint32_t attempts = 3; // Ensure fair work distribution
	for (uint32_t i = 0; i < m_numWorkerThreads * attempts; i++)
	{
		if (m_jobQueues[(queueIndex + i) % m_numWorkerThreads].TryPush(job))
			return;
	}


	// If we were not able to acquire the lock and push to any queue, we are gonna wait for the current queue to be available and then push the task
	m_jobQueues[queueIndex % m_numWorkerThreads].BlockingPush(job);
}

bool JobSystem::IsBusy(const Counter& counter) const
{
	return counter.count > 0;
}

void JobSystem::Wait(const Counter* counters, uint32_t numCounters)
{
	for (size_t i = 0; i < numCounters; i++)
	{
		while (IsBusy(counters[i]))
		{
			std::this_thread::yield();
		}
	}
}

void JobSystem::RunThread(uint32_t threadId)
{
	QK_CORE_LOGT_TAG("Core", "Thread{} Start Working", threadId);

	while (true)
	{
		// Extract a task for the queue and execute it
		Job job;

		// If the queue corresponding to the calling thread is busy, it will try to task steal on the other queues in a circular manner.
		uint32_t attempts = 3;  // Ensure fair work distribution
		for (uint32_t i = 0; i < m_numWorkerThreads * attempts; ++i)
		{
			if (m_jobQueues[(threadId + i) % m_numWorkerThreads].TryPop(job))
				break; // An available queue to pop was found, so we can now execute the function
		}

		// If we did not manage to acquire the lock and pop a task from any queue, 
		// try waiting for the current thread queue to be available to and pop a task.
		// If also this fails, exit the thread loop.
		// TODO is this an optimal behavior? What happens if we later want to push more stuff for this thread instead?	
		if (!job.isValid() && !m_jobQueues[threadId].BlockingPop(job))
			break;

		job.jobFunction();

		if (job.counter)
		{
			// Decrement the counter
			job.counter->count.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	QK_CORE_LOGT_TAG("Core", "Thread {} Finished Execution!", threadId);
}

}