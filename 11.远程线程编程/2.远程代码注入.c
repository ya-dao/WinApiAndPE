#define _CRT_SECURE_NO_WARNINGS

#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<TlHelp32.h>
/*
	目标: 不依赖于DLL文件直接向目标进程写入要执行的代码,以完成特定功能.
		要在目标进程中完成一定的功能,就需要使用相关的API函数,不同的API函数实现在不同的DLL中.
	Kernel32.dll文件在每个进程的地址是相同的,	但不代表所有dll文件在每个进程中的地址都是一样的.
	这样,在目标进程中调用API函数时,必须使用LoadLibrary()和GetProcAddress()函数动态调用所用到的API函数.
	把想要使用的API函数及API函数所在的DLL文件都封闭到一个结构体中,直接写入目标进程的空间中.
	直接把要在远程线程中执行的代码也写入目标进程的内存空间中,	最后调用CreateRemoteThread()函数即可将其运行.
	坑:
		1. 被注入的程序崩溃(问题事件名称:	APPCRASH)
			可能的情况1:
				Debug版本会加入很多调试信息.而某些调试信息并不存在于代码中,而是在其他DLL模块中,
				这样当执行到调试相关的代码时会访问不存在的DLL模块中的代码,就导致了报错.
			解决:
				将程序用Release方式进行编译连接.
			可能的情况2:
				在编译器代码优化的情况下,可能会导致出错.
			解决:
				将Release模式的项目属性进行修改:C/C++->优化
					将优化设置为:已禁用
					优选大小或速度设置为:均不
		2. 被注入的程序崩溃(问题事件名称:  BEX)
			分析:
				注入程序崩溃的问题是因为申请执行代码的内存空间没有写执行属性PAGE_READWRITE.
			解决: 
				申请执行代码的内存空间要带执行属性PAGE_EXECUTE_READWRITE
*/
#define STRLEN 50

void adjustPrivilege();
int getPidByEnumProcessName(char *szProcessName);
void injectCode(DWORD dwPid);
DWORD WINAPI remoteThreadProc(LPVOID lpParam);

typedef struct _DATA {
	/*
		这四个函数在都属性kernel32.dll的导出函数,可以在注入前就获取到其地址.
		方便后续在远程线程通过这几个函数调用其他函数.
	*/
	SIZE_T dwLoadLibrary;
	SIZE_T dwGetProcAddress;
	// 获取一个应用程序或动态链接库的模块句柄。
	SIZE_T dwGetModuleHandle;
	// 获取当前进程已加载模块的文件的完整路径，该模块必须由当前进程加载。
	SIZE_T dwGetModuleFileName;

	/*
		user32DLL保存"User32.DLL",因为MessageBoxA()函数是User32.dll的导出函数.
	*/
	char user32Dll[STRLEN];
	char messageBox[STRLEN];
	/*
		MessageBoxA()弹出的字符串
	*/
	char message[STRLEN];
}DATA, *PDATA;

void main() {
	char szProcessName[MAX_PATH] = { 0 };
	DWORD dwPid = 0;

	adjustPrivilege();

	printf("输入进程名称:\n");
	scanf("%s", szProcessName);

	dwPid = getPidByEnumProcessName(szProcessName);

	injectCode(dwPid);

	system("pause");
}

/*
	在该段注入代码中定义一个结构体变量,并进行初始化.
*/
void injectCode(DWORD dwPid) {
	// 打开进程并获取进程句柄
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if (NULL == hProcess)
	{
		DWORD dwErr = GetLastError();
		printf("打开进程失败:%ld\n", dwErr);
		return;
	}
	
	// 获取Kernel32.dll中相关的导出函数,并初始化结构体
	DATA data = { 0 };
	char *moduleName = "kernel32.dll";
	data.dwLoadLibrary = (SIZE_T)GetProcAddress(
		GetModuleHandle(moduleName), "LoadLibraryA");
	data.dwGetProcAddress = (SIZE_T)GetProcAddress(
		GetModuleHandle(moduleName), "GetProcAddress");
	data.dwGetModuleHandle = (SIZE_T)GetProcAddress(
		GetModuleHandle(moduleName), "GetModuleHandleA"); 
	// 这里Release汇编代码生成都有问题
	data.dwGetModuleFileName = (SIZE_T)GetProcAddress(
		GetModuleHandle(moduleName), "GetModuleFileNameA");
	
	// 需要使用的其他DLL和导出函数
	strcpy(data.user32Dll, "user32.dll");
	strcpy(data.messageBox, "MessageBoxA");
	// messageBox中显示的字符串
	strcpy(data.message, "注入的代码显示一个对话框!");

	// 在目标进程中申请空间保存API相关信息的结构体
	LPVOID lpData = VirtualAllocEx(hProcess, NULL, sizeof(DATA), MEM_COMMIT, PAGE_READWRITE);
	if (NULL == lpData)
	{
		DWORD dwErr = GetLastError();
		printf("申请空间失败:%ld\n", dwErr);
	}
	SIZE_T stWriteNum = 0;
	WriteProcessMemory(hProcess, lpData, &data, sizeof(data), &stWriteNum);

	/* 
		在目标进程空间申请的用于保存代码的长度,大小为1KB.
		申请执行代码的内存空间要带执行属性PAGE_EXECUTE_READWRITE
		注入程序崩溃的问题是因为申请执行代码的内存空间没有写执行属性PAGE_READWRITE.
	*/
	SIZE_T stFunSize = 0x400;
	LPVOID lpCode = VirtualAllocEx(hProcess, NULL, stFunSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (NULL == lpData)
	{
		DWORD dwErr = GetLastError();
		printf("申请空间失败:%ld\n", dwErr);
	}
	// 将需要执行的代码写入到目标进程空间
	WriteProcessMemory(hProcess, lpCode, &remoteThreadProc, stFunSize, &stWriteNum);

	// 指定该远程线程需要执行的函数地址为写入的代码地址
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)lpCode, lpData, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hProcess);
}

DWORD WINAPI remoteThreadProc(LPVOID lpParam) {
	PDATA pData = (PDATA)lpParam;

	// 使用函数指针,定义API函数原型
	HMODULE(__stdcall *MyLoadLibrary)(LPCTSTR);
	FARPROC(__stdcall *MyGetProcAddress)(HMODULE,LPCTSTR);
	HMODULE(__stdcall *MyGetModuleHandle)(LPCTSTR);
	DWORD(__stdcall *MyGetModuleFileName)(HMODULE,LPCTSTR,DWORD);
	int(__stdcall *MyMessageBox)(HWND, LPCTSTR, LPCTSTR, UINT);

	// 对各函数地址进行赋值
	MyLoadLibrary = (HMODULE(__stdcall *)(LPCTSTR))pData->dwLoadLibrary;
	MyGetProcAddress = (FARPROC(__stdcall *)(HMODULE, LPCTSTR))pData->dwGetProcAddress;
	MyGetModuleHandle = (HMODULE(__stdcall *)(LPCTSTR))pData->dwGetModuleHandle;
	MyGetModuleFileName = (DWORD(__stdcall *)(HMODULE, LPCTSTR, DWORD))pData->dwGetModuleFileName;

	// 加载User32.dll
	HMODULE hModule = MyLoadLibrary(pData->user32Dll);
	// 获得MessageBoxA的函数地址
	MyMessageBox = (int(__stdcall *)(HWND, LPCTSTR, LPCTSTR, UINT))
		MyGetProcAddress(hModule, pData->messageBox);

	char szModuleFile[MAX_PATH] = { 0 };
	/*
		获取当前进程对应的可执行文件完整路径名
		hModule: 应用程序或DLL实例句柄,NULL则为获取当前程序可执行文件路径名
	*/
	MyGetModuleFileName(NULL, szModuleFile, MAX_PATH);

	MyMessageBox(NULL, pData->message, szModuleFile, MB_OK);

	return 0;
}

/*
	采用枚举进程的方式根据进程名获取对应的Pid
	查找失败返回-1
*/
int getPidByEnumProcessName(char *szProcessName) {
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snap == INVALID_HANDLE_VALUE)
	{
		printf("CreateToolhelp32Snapshot 失败\n");
		return -1;
	}
	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(pe32);
	BOOL result = Process32First(snap, &pe32);
	while (result)
	{
		// 判断进程名是否相等,相等则返回其对应的pid
		if (strcmp(pe32.szExeFile, szProcessName) == 0)
		{
			return pe32.th32ProcessID;
		}

		result = Process32Next(snap, &pe32);
	}
	CloseHandle(snap);
	return -1;
}
/*
	提升进程优先级
*/
void adjustPrivilege() {
	HANDLE hKey = NULL;
	BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hKey);
	if (result == FALSE)
	{
		DWORD dwErr = GetLastError();
		printf("打开进程访问令牌失败:%ld\n", dwErr);
		return;
	}
	TOKEN_PRIVILEGES tp;
	// PrivilegeCount指的数组元素的个数
	tp.PrivilegeCount = 1;
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hKey, FALSE, &tp, sizeof(tp), NULL, NULL);
}