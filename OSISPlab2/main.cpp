#include <iostream>
#include <string>
#include "ThreadPool.h"
using namespace std;

DWORD WINAPI callback1(LPVOID param)
{
	string info = *(string*)param;
	info += "\nTask 1 end\n";
	cout << info << endl;
	Sleep(2000);
	return 0;
}
DWORD WINAPI callback2(LPVOID param)
{
	string info = *(string*)param;
	info += "\nTask 2 end\n";
	cout << info << endl;
	Sleep(3000);
	return 0;
}
DWORD WINAPI callback3(LPVOID param)
{
	string info = *(string*)param;
	info += "\nTask 3 end\n";
	cout << info << endl;
	return 0;
}
void main()
{
	ThreadPool tp(4, 2);//max,current
	tp.Queue(new Task(callback1, NULL, 2, THREAD_PRIORITY_LOWEST));
	tp.Queue(new Task(callback2, NULL, 1, THREAD_PRIORITY_LOWEST));
	tp.Queue(new Task(callback3, NULL, 2, THREAD_PRIORITY_LOWEST));
	tp.Queue(new Task(callback3, NULL, 3, THREAD_PRIORITY_NORMAL));
	tp.Queue(new Task(callback2, NULL, 1, THREAD_PRIORITY_LOWEST));
	tp.Queue(new Task(callback3, NULL, 3, THREAD_PRIORITY_NORMAL));
	tp.Queue(new Task(callback1, NULL, 2, THREAD_PRIORITY_HIGHEST));
	tp.Queue(new Task(callback2, NULL, 3, THREAD_PRIORITY_LOWEST));
	tp.Queue(new Task(callback3, NULL, 2, THREAD_PRIORITY_HIGHEST));
	tp.Queue(new Task(callback2, NULL, 1, THREAD_PRIORITY_LOWEST));
	tp.Queue(new Task(callback3, NULL, 3, THREAD_PRIORITY_NORMAL));
	tp.Queue(new Task(callback1, NULL, 2, THREAD_PRIORITY_HIGHEST));
	tp.Queue(new Task(callback2, NULL, 3, THREAD_PRIORITY_LOWEST));
	tp.Queue(new Task(callback3, NULL, 2, THREAD_PRIORITY_HIGHEST));

	Sleep(3000);
	cout << "Plese press any key to exit...\n" << endl;
	getchar();
}