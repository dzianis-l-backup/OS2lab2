#ifndef LAB2
#define LAB2
#include <Windows.h>
#include <vector>//for task and workers storage
#include <string>
#include "easylogging++.h"//logging with easylogging
using namespace std;
INITIALIZE_EASYLOGGINGPP
#define DEFAULT_THREAD_COUNT 4
typedef DWORD (WINAPI *FUNC)(LPVOID lpParam);//func pointer
class ThreadPool
{
	private:

		class Worker
		{
			private:
				bool isOn;
				HANDLE hThread;
			public:
				Worker()
				{
					isOn = true;
				}

				bool IsOn()
				{
					return isOn;
				}

				void SetOn(bool b)
				{
					isOn = b;
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
			string info = "";
			info = to_string(current_thread_count);
			info += " workers were created";

			for (int i = 0; i<current_thread_count; i++)
			{
				workers.push_back(Worker());
			}
			LOG(INFO) << info;
		}

		int GetFreeWorkerIndex()
		{
			for (int i = 0; i < workers.size(); i++)
				if (workers[i].IsOn())
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
				ThreadPool *tp = (ThreadPool*)param;//current threadpool
				int task_count;

				//compute the number of tasks to comlete
				do
				{
					Sleep(1000);
					WaitForSingleObject(tp->hMutex, INFINITE);
					task_count = tp->tasks.size();
					ReleaseMutex(tp->hMutex);
				} while (task_count == 0);


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
						tp->workers[free_worker_index].SetOn(false);
						tp->workers[free_worker_index].SetThreadHandle(CreateThread(NULL, 0, task_proc, t, CREATE_SUSPENDED, NULL));
						SetThreadPriority(tp->workers[free_worker_index].GetThreadHandle(), t->GetThreadPriority());
						ResumeThread(tp->workers[free_worker_index].GetThreadHandle());
					}
					else
					{
						if (tp->current_thread_count < tp->max_thread_count)
						{
							tp->workers.push_back(Worker());
							tp->workers[tp->workers.size() - 1].SetOn(false);
							tp->workers[tp->workers.size() - 1].SetThreadHandle(CreateThread(NULL, 0, task_proc, t, CREATE_SUSPENDED, NULL));
							SetThreadPriority(tp->workers[tp->workers.size() - 1].GetThreadHandle(), t->GetThreadPriority());
							ResumeThread(tp->workers[tp->workers.size() - 1].GetThreadHandle());
							tp->current_thread_count = tp->workers.size();
						}
						else
						{
							LOG(ERROR) << "Max thread limit is achieved.\n";
							delete t;
						}
					}
					for (int i = 0; i < tp->workers.size(); i++)
					{
						if (!tp->workers[i].IsOn())
						{
							int code = WaitForSingleObject(tp->workers[i].GetThreadHandle(), 0);
							if (code == WAIT_OBJECT_0)
							{
								CloseHandle(tp->workers[i].GetThreadHandle());
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
			Task *t = (Task*)lParam;
			string priority;
			string info;
			FUNC func = t->GetCallbackTaskFunc();

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
			info = "";
			info += "\nID: ";
			info += to_string(GetCurrentThreadId());
			info += "\n";
			info += "Thread priority: ";
			info += priority;
			info += "\n";
			info += "Task priority: ";
			info += to_string(t->GetTaskPriority());
			info += "\n";

			try
			{
				func(&info);
			}
			catch (exception &e)
			{
				LOG(ERROR) << e.what();
			}

			return 0;
		}

	public:
		ThreadPool(int max_thread_count, int current_thread_count = DEFAULT_THREAD_COUNT)
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

		void Enqueue(Task *task)
		{
			WaitForSingleObject(hMutex, INFINITE);
			tasks.push_back(task);
			LOG(INFO) << "New task was added";
			ReleaseMutex(hMutex);
		}

};
