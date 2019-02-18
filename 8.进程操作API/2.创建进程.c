#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#define FILE_PATH "C:\\Windows\\notepad.exe"

void main() {
	// 该结构体用于保存新进程和主线程的句柄和进程ID,进程创建后,里面的两个句柄需要关闭
	PROCESS_INFORMATION pi = { 0 };
	// 该结构体决定进程启动的状态,使用前需要对其cb成员进行赋值,该成员变量用于保存该结构体的大小
	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);

	BOOL bResult = CreateProcessA(FILE_PATH, 
		NULL, NULL, NULL, FALSE,
		NULL, NULL, NULL, &si, &pi);
	if (bResult == FALSE)
	{
		printf("create process error!\n");
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	system("pause");
}