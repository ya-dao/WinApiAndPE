#define  _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<TlHelp32.h>
int getProcessId();
void enumProcess();
void suspendProcess();
void resumeProcess();
void adjustProcessPrivilege();
/*
 1. 背景:
	有的时候,不得不让某些进程处理暂停状态,比如,病毒有两个运行的进程,它们在不断互相检测,
	当一个病毒进程发现另一个病毒进程被结束了,那么它会再次把被结束的进程运行起来.
	由于两个病毒进行的互相检测频率较高,因此很难把两个病毒进程都结束掉.
	因此,只能让两个病毒进程都暂停后,再结束两个病毒的进程.
 2. 暂停进程原理(没有现成的SuspendProcess()API函数可以用):
	解释1: 其原理就是按照进程的ID遍历进程的所有线程，通过这些线程的ID获得句柄并挂起它们。
			由于所有线程都被挂起了，因此进程也被挂起了。
	解释2: 进程的暂停实质上是线程的暂停,因为进程是一个资源单位,而真正占用CPU时间的是线程,
			如果需要将进程暂停,就需要将进程中的所有线程全部暂停.
 3. API函数
	3.1 暂停与恢复线程所需函数:
		让线程暂停使用的API函数是SuspendThread(),定义如下:
			DWORD SuspendThread(HANDLE hThread);
		该函数只有一个参数,即需要暂停的线程句柄,获得线程的句柄可以使用OpenThread()函数,定义如下:
			HANDLE OpenThread(
				DWORD dwDesiredAccess, //访问权限
				BOOL bInheritHandle, // 是否可以继承
				DWORD dwThreadId //线程ID
			);
	3.2	枚举线程细节:
		要暂停进程中的全部线程,则离不开枚举线程,枚举线程的函数是Thread32First()和Thread32Next().
		在枚举线程时,不能创建指定进程中的线程快照,线程快照只能是系统中所有线程的快照.所以在暂停线程时,必须对枚举到
		的线程进行判断,看其是否属于指定进程中的线程.如何判断一个线程属于哪个进程呢?需要借助THREADENTRY32结构体,
		在该结构体中,th32ThreadID表示当前枚举到的线程的线程ID,而th32OwnerProcessID则表示线程所属的进程ID.
		这样在枚举线程时,只要判断是否属于指定的进程,即可进行暂停操作.
	3.3 恢复线程的API函数:
		DWORD ResumeThread(HANDLE hThread);
*/
void main() {
	adjustProcessPrivilege();
	int option = 0;
	while (1)
	{
		printf("选择功能: 1.枚举进程  2.暂停进程  3.恢复进程  4.退出\n");
		scanf("%d",&option);
		
		if (option == 4)
		{
			break;
		}

		switch (option)
		{
		case 1:
			enumProcess();
			break;

		case 2:
			suspendProcess();
			break;
		case 3:
			resumeProcess();
			break;
		}
	}

	system("pause");
}

int getProcessId() {
	int id = 0;
	printf("输入进程ID:\n");
	scanf("%d", &id);
	return id;
}

void suspendProcess() {
	int pid = getProcessId();
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);
	if (snap == INVALID_HANDLE_VALUE)
	{
		printf("创建系统线程快照失败!\n");
		return;
	}
	THREADENTRY32 entry = { 0 };
	entry.dwSize = sizeof(entry);
	
	BOOL result = Thread32First(snap, &entry);
	while (result) {
		// 判断该线程是否属于指定进程
		if (entry.th32OwnerProcessID == pid)
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, entry.th32ThreadID);
			SuspendThread(hThread);
			CloseHandle(hThread);
		}
		result = Thread32Next(snap, &entry);
	}
	CloseHandle(snap);
}

void resumeProcess() {
	int pid = getProcessId();
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, NULL);
	if (snap == INVALID_HANDLE_VALUE)
	{
		printf("创建系统线程快照失败!\n");
		return;
	}
	THREADENTRY32 entry = { 0 };
	entry.dwSize = sizeof(entry);

	BOOL result = Thread32First(snap, &entry);
	while (result) {
		// 判断该线程是否属于指定进程
		if (entry.th32OwnerProcessID == pid)
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, entry.th32ThreadID);
			ResumeThread(hThread);
			CloseHandle(hThread);
		}
		result = Thread32Next(snap, &entry);
	}
	CloseHandle(snap);
}

void enumProcess() {
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 entry = { 0 };
	entry.dwSize = sizeof(entry);
	BOOL result = Process32First(handle, &entry);
	while (result)
	{
		while (result)
		{
			printf("%-6d | %ls\n", entry.th32ProcessID, entry.szExeFile);
			result = Process32Next(handle, &entry);
		}
	}
	CloseHandle(handle);
}

void adjustProcessPrivilege() {
	HANDLE hToken = NULL;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
	if (result)
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		CloseHandle(hToken);
	}
	else
	{
		printf("修改进程优先级失败!\n");
	}
}