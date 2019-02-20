#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <iostream>
#include <pthread.h>
#include <queue>

#define POOL_NUM 5		// 线程池中线程的个数

typedef void (*handler_t)(int);

class Task
{
private:
	int sock;
	handler_t handler;
	
public:
	Task(int sock_, handler_t handler_):sock(sock_), handler(handler_)
	{}

	void Run()
	{
		handler(sock);
	}

	~Task()
	{}
};

class ThreadPool
{
private:
	int num;
	int idle_num;
	std::queue<Task> task_queue;
	pthread_mutex_t lock;
	pthread_cond_t cond;

public:
	ThreadPool(int num_) : num(num_), idle_num(0)
	{
		pthread_mutex_init(&lock, nullptr);
		pthread_cond_init(&cond, nullptr);
	}

	void LockQueue()
	{
		pthread_mutex_lock(&lock);
	}

	void UnLockQueue()
	{
		pthread_mutex_unlock(&lock);
	}

	bool IsTaskQueueEmpty()
	{
		return task_queue.size() == 0 ? true : false;
	}

	void Idle()
	{
		idle_num++;
	
		pthread_cond_wait(&cond, &lock);

		idle_num--;
	}

	void WakeUpOne()
	{
		pthread_cond_signal(&cond);
	}

	void WakeUpAll()
	{
		pthread_cond_broadcast(&cond);
	}

	Task PopTask()
	{
		Task t = task_queue.front();
		task_queue.pop();

		return t;
	}

	void PushTask(Task &t)
	{
		LockQueue();

		task_queue.push(t);
		WakeUpOne();

		UnLockQueue();
	}

	static void *ThreadRoutinue(void *arg)
	{
		pthread_detach(pthread_self());
		ThreadPool *tp = (ThreadPool*)arg;

		while (1)
		{
			tp->LockQueue();
			while (tp->IsTaskQueueEmpty())
			{
				tp->Idle();
			}

			Task t = tp->PopTask();
			tp->UnLockQueue();
			std::cout << "task handler by: " << pthread_self() << std::endl;
			t.Run();
		}
	}

	void InitThreadPool()
	{
		pthread_t tid;

		for (int i = 0; i < num; i++)
		{
			pthread_create(&tid, nullptr, ThreadRoutinue, (void*)this);
		}
	}

	~ThreadPool()  
	{
		pthread_mutex_destroy(&lock);
		pthread_cond_destroy(&cond);
	}

};


// 懒汉模式
class Singleton
{
private:
	static ThreadPool *p;
	static pthread_mutex_t mutex;
	
	Singleton() = delete;						// 构造函数私有
	
	// 防拷贝
	Singleton(Singleton const&) = delete;	
	Singleton& operator=(Singleton const&) = delete;

public:
	static ThreadPool *GetInstance()
	{
		if (nullptr == p)
		{
			pthread_mutex_lock(&mutex);
			if (nullptr == p)
			{
				p = new ThreadPool(POOL_NUM);
				p->InitThreadPool();
			}
			pthread_mutex_unlock(&mutex);
		}
		return p;
	}

public:
	class CGarbo
	{
		public:
			~CGarbo()
			{
				if (Singleton::p)
				{
					delete Singleton::p;
				}
			}
	};

	static CGarbo Garbo;
};

ThreadPool *Singleton::p = nullptr;
Singleton::CGarbo Garbo;
pthread_mutex_t Singleton::mutex = PTHREAD_MUTEX_INITIALIZER;

#endif  // __THREADPOOL_H__
