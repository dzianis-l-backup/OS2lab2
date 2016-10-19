#ifndef LAB2
#define LAB2
#include <Windows.h>
#include <vector>//for task and workers storage
#include <string>
#include "easylogging++.h"//logging with easylogging
using namespace std;
INITIALIZE_EASYLOGGINGPP
#define DEFAULT_THREAD_QUANTITY 4
typedef DWORD (WINAPI *FUNC)(LPVOID lpParam);//func pointer

DWORD WINAPI callback4(LPVOID param)
{
	string info = *(string*)param;
	info += "\nTask 4 end\n";
	cout << info << endl;
	return 0;
}

class Task
{
	public:
		Task(LPTHREAD_START_ROUTINE callbackTaskFunc, LPVOID taskParam, int task_priority, int thread_priority)
		{
			this->callbackTaskFunc = callbackTaskFunc;//callback for the task
			this->taskParam = taskParam;//param for callback
			this->task_priority = task_priority;//the priority of the task
			this->thread_priority = thread_priority;//the priority of the thread
		}
		LPTHREAD_START_ROUTINE GetCallbackTaskFunc()
		{
			return callbackTaskFunc;//to return callback
		}
		LPVOID GetTaskParam()//
		{
			return taskParam;
		}
		int GetTaskPriority()
		{
			return task_priority;
		}

		int GetThreadPriority()
		{
			return thread_priority;
		}

	private:
		LPTHREAD_START_ROUTINE callbackTaskFunc;
		LPVOID taskParam;
		int task_priority;
		int thread_priority;
};

class Wrapper{
	Task * task;
	public:
		Wrapper()
		{
			task = NULL;
		}
		Task * getTask()
		{
			return task;
		}
		void  setTask(Task * task)
		{
			this->task = task;
		}
};

class ThreadPool
{
private:
	class Worker
	{
		private:
			bool isFree;
			HANDLE hThread;
			

		public:		
			Wrapper * wrapper;

			Worker()
			{
				wrapper = new Wrapper();
				hThread = CreateThread(NULL, 0, task_proc, wrapper, CREATE_SUSPENDED, NULL);
				isFree = true;
				
			}

			void StartWorker(LPVOID lpParameter)
			{
				wrapper->setTask((Task*)lpParameter);
				int i = ResumeThread(hThread);
			}

			bool IsFree()
			{
				return isFree;
			}

			void SetFree(bool b)
			{
				isFree = b;
			}

			HANDLE GetThreadHandle()
			{
				return hThread;
			}

			void SetThreadHandle(HANDLE h)
			{
				hThread = h;
			}
	};

	int max_thread_count;
	int current_thread_count;

	vector<Worker> workers;
	vector<Task *> tasks;

	HANDLE hMutex;
	HANDLE hDispatcherThread;

	void CreateWorkers()
	{
		for (int i = 0; i<current_thread_count; i++)
		{
			workers.push_back(Worker());
		}
		string info = "";
		info = to_string(current_thread_count);
		info += " workers have been created";
		LOG(INFO) << info;
	}

	int GetFreeWorkerIndex()
	{
		for (int i = 0; i < workers.size(); i++)
			if (workers[i].IsFree())
				return i;
		return -1;
	}

	void SortByTaskPriority()
	{
		for (int i = 0; i < tasks.size() - 1; i++)
			for (int j = i + 1; j < tasks.size(); j++)
				if (tasks[j]->GetTaskPriority() < tasks[i]->GetTaskPriority())
				{
					Task *temp = tasks[i];
					tasks[i] = tasks[j];
					tasks[j] = temp;
				}
	}

		static DWORD WINAPI dispatcher_proc(LPVOID param)
		{
			while (true)
			{
				Task *t;
				ThreadPool *tp = (ThreadPool*)param;//current istance of threadpool class
				int task_quantity;

				//compute the number of tasks to comlete
				do
				{
					Sleep(1000);
					WaitForSingleObject(tp->hMutex, INFINITE);
					task_quantity = tp->tasks.size();
					ReleaseMutex(tp->hMutex);
				} 
				while (task_quantity == 0);


				WaitForSingleObject(tp->hMutex, INFINITE);
				int free_worker_index;
				tp->SortByTaskPriority();
				while (tp->tasks.size() != 0)
				{
					t = tp->tasks[tp->tasks.size() - 1];
					tp->tasks.pop_back();
					free_worker_index = tp->GetFreeWorkerIndex();
					if (free_worker_index != -1)
					{
						tp->workers[free_worker_index].SetFree(false);	
						//tp->workers[free_worker_index].task = t;
						SetThreadPriority(tp->workers[free_worker_index].GetThreadHandle(), t->GetThreadPriority());
						
						tp->workers[free_worker_index].StartWorker(t);
						string message = "workers in queue - " + to_string(tp->current_thread_count);
						LOG(INFO) << message;
					}
					else
					{
						if (tp->current_thread_count < tp->max_thread_count)
						{
							tp->workers.push_back(Worker());
							tp->workers[tp->workers.size() - 1].SetFree(false);
							//tp->workers[tp->workers.size() - 1].task = t;
							SetThreadPriority(tp->workers[tp->workers.size() - 1].GetThreadHandle(), t->GetThreadPriority());

							tp->workers[tp->workers.size() - 1].StartWorker(t);
							tp->current_thread_count = tp->workers.size();
							string message = "workers in queue - " + to_string(tp->current_thread_count);
							LOG(INFO) << message;
						}
						else
						{	
							string error = "Max thread limit is achieved: workers in queue - " + to_string(tp->workers.size());
							error += "\n";
							error += "Max thread limit is achieved: current tasks quantity - " + to_string(tp->current_thread_count);
							error += "\n";
							LOG(ERROR) << (error);
							delete t;
						}
					}
					for (int i = 0; i < tp->workers.size(); i++)
					{
						if (!tp->workers[i].IsFree())//if busy
						{
							int code = WaitForSingleObject(tp->workers[i].GetThreadHandle(), 0);
							if (code == WAIT_OBJECT_0)
							{
								TerminateThread(tp->workers[i].GetThreadHandle(),0);
								//CloseHandle(tp->workers[i].GetThreadHandle());
								tp->workers[i] = Worker();
							}
						}
					}


				}
				ReleaseMutex(tp->hMutex);
			}
		}

		static DWORD WINAPI task_proc(LPVOID lParam)
		{
			Wrapper * currentWrapper = (Wrapper *)lParam;
			while (currentWrapper->getTask()== NULL){
				
			}
			Task *t = currentWrapper->getTask();
			string priority;
			string info;
			FUNC callback = t->GetCallbackTaskFunc();

			switch (GetThreadPriority(GetCurrentThread()))
			{
			case THREAD_PRIORITY_NORMAL:
				priority = "Normal";
				break;
			case THREAD_PRIORITY_LOWEST:
				priority = "Lowest";
				break;
			case THREAD_PRIORITY_HIGHEST:
				priority = "Highest";
				break;
			default:
				priority = "Other";
				break;
			}

			info = "...";
			info += "\nThread id: ";
			info += to_string(GetCurrentThreadId());
			info += "\n";
			info += "Thread priority: ";
			info += priority;
			info += "\n";
			info += "Task priority: ";
			info += to_string(t->GetTaskPriority());
			info += "\n...";

			try
			{
				callback(&info);
			}
			catch (exception &e)
			{
				LOG(ERROR) << e.what();
			}

			return 0;
	}

public:
	ThreadPool(int max_thread_count, int current_thread_count = DEFAULT_THREAD_QUANTITY)
	{
		this->max_thread_count = max_thread_count;
		this->current_thread_count = current_thread_count;
		hMutex = CreateMutex(NULL, false, NULL);
		hDispatcherThread = CreateThread(NULL, 0, dispatcher_proc, this, NULL, NULL);
		CreateWorkers();
	}

	~ThreadPool()
	{
		for (int i = 0; i < workers.size(); i++)
		{
			TerminateThread(workers[i].GetThreadHandle(), 0);
		}
		workers.clear();
		tasks.clear();
		TerminateThread(hDispatcherThread, 0);
		CloseHandle(hMutex);
	}

	void Queue(Task *task)
	{
		WaitForSingleObject(hMutex, INFINITE);//unsafe part
		tasks.push_back(task);
		LOG(INFO) << "New task has been added!";
		ReleaseMutex(hMutex);
	}

};

#endif

