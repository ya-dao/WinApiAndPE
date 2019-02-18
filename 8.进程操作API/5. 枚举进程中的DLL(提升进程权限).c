#define  _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<TlHelp32.h>

void enumDLL();
void enumProcess();
void adjustProcessPrivilege();
/*
	思路和过程跟枚举进程差不多.
		1. 先创建快照CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid)
		2. 再遍历快照中的每一项ModuleEntry32结构体.
			Module32First
	
 1. 进程的权限不够可能会导致使用CreateToolhelp32Snapshot()创建快照,openProcess()函数
	打开smss.exe,winlogon.exe等系统进程时或者远程线程进行注入时调用失败.
	解决该问题的办法就是将该进程的权限提升到SeDebugPrivilege.
	步骤如下(参考https://baike.baidu.com/item/OpenProcessToken):
		1. 使用OpenProcessToken()函数打开当前进程的访问令牌
			BOOL OpenProcessToken(
				__in HANDLE ProcessHandle, //要修改访问权限的进程句柄
				__in DWORD DesiredAccess, //指定你要进行的操作类型
				__out PHANDLE TokenHandle //返回的访问令牌指针
			)；
		2. 使用LookupPrivilegeValue()函数取得描述权限的LUID(指locally unique identifier),不同系统上值不同
			BOOL LookupPrivilegevalue(
				LPCTSTR lpSystemName, // system name,本地系统传入NULL即可
				LPCTSTR lpName, // privilege name
				PLUID lpLuid // locally unique identifier,就是返回LUID的指针
			);
		3. 使用AdjustTokenPrivilege()函数调整访问令牌的权限
  2. 如果调整了进程的优先级还是无法枚举系统进程的DLL(即CreateToolhelp32Snapshot()函数打开系统进程失败),
	根据StackOverflow上的答案:如果指定的进程是64位进程且调用程序是32位进程，则此函数将失败，最后一个错误代码为ERROR_PARTIAL_COPY（299）。
	解决办法是编译成64位的程序
*/
void main() {
	adjustProcessPrivilege();
	enumProcess();
	enumDLL();
	system("pause");
}

/*
	调整进程优先级
*/
void adjustProcessPrivilege() {
	HANDLE hToken = NULL;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
	if (result == TRUE)
	{
		TOKEN_PRIVILEGES tp;
		// PrivilegeCount指的数组元素的个数
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

void enumDLL() {
	printf("--------输入进程PID------------\n");
	int pid = 0;
	scanf("%d", &pid);
	printf("DLL名称\t\t|  DLL路径\n");
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

	if (snapshot == INVALID_HANDLE_VALUE)
	{
		printf("创建DLL快照失败!\n");
		return;
	}

	MODULEENTRY32 entry = { 0 };
	entry.dwSize = sizeof(MODULEENTRY32);
	BOOL result = Module32First(snapshot, &entry);
	while (result)
	{
		printf("%15ls  |  %ls\n", entry.szModule, entry.szExePath);
		result = Module32Next(snapshot, &entry);
	}

	CloseHandle(snapshot);
}

void enumProcess() {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		printf("创建进程快照失败!\n");
		return;
	}
	printf("PID    |  进程名称\n");
	PROCESSENTRY32 entry = { 0 };
	entry.dwSize = sizeof(PROCESSENTRY32);
	BOOL result = Process32First(snapshot, &entry);
	while (result)
	{
		printf("%-6d | %ls\n", entry.th32ProcessID, entry.szExeFile);
		result = Process32Next(snapshot, &entry);
	}
	CloseHandle(snapshot);
}