#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<TlHelp32.h>

/*
 1. 在Windows当中,枚举进程,线程或者DLL文件时,方法都是相同的,都是通过创建指定的相关快照,再通过循环逐条遍历快照的内容.
	类似操作的区别只是在创建快照时参数不同,在逐条获取快照内容时的API函数不同而已.
 2. 相关函数都包含在Tlhelp32.h头文件中.
 3.	创建进程,线程或者DLL文件快照的API函数:
		HANDLE WINAPI CreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID);
		参数说明:
			dwFlags:指明要建立系统快照的类型,对于要枚举的内容,该参数可以指定如下值:
				TH32CS_SNAPMODULE:  在枚举进程中的DLL时指定
				TH32CS_SNAPPROCESS: 在枚举系统中的进程时指定
				TH32CS_SNAPTHREAD:  在枚举系统中的线程时指定
			th32ProcessID:该参数根据dwFlags参数的不同而不同.如果枚举的是系统中的进程或系统中的线程,该参数为NULL;
				如果枚举的是进程中加载的DLL时,该参数是进程ID
		返回值: 该函数返回一个快照的句柄,并提供给枚举函数使用.
	进程枚举函数:
		Process32First(HANDLE hSnapshot, LPPROCESSENTRY32 lppe):
			参数说明:
				hSnapshot:该参数为CreateTollhelp32Snapshot()函数返回的句柄.
				lppe:该参数为指向PROCESSENTRY32结构体的指针,该结构体使用之前需要对变量dwSize进程赋值.
					该变量保存了PROCESSENTRY32结构体的大小.
		Process32Next(HANDLE hSnapshot, LPPROCESSENTRY32 lppe):
			与Process32First使用相同.
*/
void enumProcess();

void main() {

	enumProcess();

	system("pause");
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
	char buffer[MAXBYTE] = { 0 };
	while (result)
	{
		printf("%-6d | %ls\n",entry.th32ProcessID,entry.szExeFile);
		result = Process32Next(snapshot, &entry);
	}
	CloseHandle(snapshot);
}