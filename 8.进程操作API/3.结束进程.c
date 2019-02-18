#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>

void terminateProcess();

void main() {

	terminateProcess();

	system("pause");
}

/*
	通过寻找窗口句柄来关闭进程
*/
void terminateProcess() {
	HWND hNotepadWnd = FindWindowA(NULL, "无标题 - 记事本");
	if (hNotepadWnd == NULL)
	{
		printf("窗口寻找失败\n");
		return;
	}

	// 根据窗口句柄获取进程ID
	DWORD dwNotepadPid = 0;
	GetWindowThreadProcessId(hNotepadWnd, &dwNotepadPid);
	if (dwNotepadPid == 0)
	{
		printf("获取进程ID失败\n");
		return;
	}
	else
	{
		printf("PID:%ld\n", dwNotepadPid);
	}
	
	// 根据进程ID得到其句柄
	HANDLE hNotepadHandle = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwNotepadPid);
	if (hNotepadHandle == NULL)
	{
		printf("获取进程句柄失败\n");
		return;
	}

	// 根据句柄结束进程
	BOOL bResult = TerminateProcess(hNotepadHandle, 0);
	if (bResult == TRUE)
	{
		printf("结束进程成功\n");
	}

	CloseHandle(hNotepadHandle);
}