#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<TlHelp32.h>
/*
	远程线程:进程A在进程B中创建一个线程
	一. DLL的代码注入原理
		该注入和卸载的过程其实就是让远程线程执行一次LoadLibrary和FreeLibrary函数.
		远程线程加载一个DLL文件,通过DllMain()调用DLL中的具体功能代码,这样注入DLL文件后就可以让DLL做很多事情了.
预备知识:
	问题1:
		无论是木马还是病毒,都是可执行程序,如果他们是EXE文件的话,运行时必定会产生一个进程,很容易被发现.
		在编写木马或者病毒时,可以选择将其编写为DLL文件.DLL文件运行时不会单独创建一个进程,它的运行会被加载到某个进程的地址空间中,因此隐蔽性较好.
		DLL文件如果不被进程加载又如何在进程的地址空间中运行呢?
	解决:
		强制让进程加载DLL文件到其地址空间中去,这个强制的手段就是远程线程
	创建远程线程相关的API函数:
		HANDLE CreateRemoteThread{
			HANDLE hProcess,
			...,
			LPTHREAD_START_ROUTINE lpStartAddress,
			...,
			LPVOID lpParameter,
			...
		}
		hProcess: 正常情况下,该给其他进程创建线程使用的,其第一个参数是指定某进程的句柄,获取进程的句柄使用OpenProcess(),该函数需要提供PID作为参数.
		lpStartAddress: 指定线程函数的地址
		lpParameter: 指定传递给线程函数的参数
	注:
		该函数相比CreateThread来说,只多了个hProcess参数,指定要在里面创建线程的进程句柄.
		其实创建线程的CreateThread函数的实现就是依赖于CreateRemoteThread函数来完成的,其中的hProcess参数传入的是NtCurrentProcess().
		NtCurrentProcess()函数的功能是获得当前进程的句柄.
	问题2:
		    由于每个进程的地址空间是隔离的,那么新创建的线程函数的地址也应该在目标进程中,而不应该在调用CreateRemoteThread()函数的进程中.
		同样,传递给线程函数的参数也应该在目标进程中.
		    如何让线程函数的地址在目标进程中呢?如何线程函数的参数也可以传递到目标进程中呢?
	解决:
		在讨论这个问题以前,先来考虑线程函数需要完成的功能,前面提到,这里主要完成的功能是注入一个DLL文件到目标进程中,那么线程函数的功能就是加载DLL文件.
			加载DLL文件的方法就是LoadLibrary()函数,定义如下:
				HMODULE LoadLibrary{
					LPCTSTR lpFileName //模块名称
				}
			线程函数的定义如下:
				DWORD WINAPI ThreadProc{
					LPVOID lpParameter //线程参数
				}
			比较这两个函数,除了函数的返回值类型和参数类型以外,其函数格式是相同的,这里只考虑其相同的部分,因为其函数的格式相同,
		首先调用约定相同,都是WINAPI(即_stdcall方式);其次参数个数相同,都只有一个.
		那么可以直接把LoadLibrary()函数作为线程函数创建到指定的进程中.LoadLibrary()的参数是欲加载的DLL文件的完整路径,
		只要在CreateRemoteThread()函数中赋值一个指向DLL文件完整路径的指针给LoadLibrary()函数即可.这样使用CreateRemoteThread()函数就可以创建一个远程线程了.
	问题3:
			不过,还有两个问题没有解决,首先是如何将LoadLibrary()函数的地址放到目标进程空间中让CreateRemoteThread()调用,
		其次是传递给LoadLibrary()函数的参数也需要在目标进程空间中,并且要通过CreateRemoteThread()指定给LoadLibrary()函数.
	解决:
			首先解决第一个问题,即如何将LoadLibrary()函数的地址放到目标进程空间中,LoadLibrary()函数是系统中的Kernel32.dll的导出函数,
		Kernel32.dll这个DLL文件在任何进程中的加载位置都是相同的,也就是说,LoadLibrary()函数的地址在任何进程中的地址都是相同的.
		因此,只要在进程中获得LoadLibrary()函数的地址,那么该地址在目标进程中也可以使用.CreateRemoteThread()函数的线程地址参数直接传递LoadLibrary()函数的地址即可.
			其次解决第二个问题,即如何将欲加载的DLL文件完整路径写入目标进程中.这需要借助WriteProcessMemory()函数,定义如下:
		BOOL WriteProcessMemory{
			HANDLE hProcess,
			LPVOID lpBaseAddress,
			LPVOID lpBuffer,
			DWORD nSize,
			LPDWORD lpNumberOfBytesWritten
		};
		该函数的功能是把lpBuffer中的内容写入到进程句柄是hProcess进程的lpBaseAddress地址处,写入的长度为nSize.
		参数说明:
			hProcess:指定进程的进程句柄
			lpBaseAddress:指定写入目标进程内存的起始地址
			lpBuffer:要写入目标进程内存的缓冲区起始地址
			nSize:指定写入目标内存中的缓冲区的长度
			lpNumberOfBytesWritten:用于接收实际写入内容的长度
		使用该函数可以把DLL文件的完整地址写入目标进程中,这样就可以在目标进程中用LoadLibrary()函数加载指定的DLL文件了.
	注:
		该函数的功能非常强大,比如在破解方面,用该函数可以实现一个"内存补丁";在开发方面,该函数可以用于修改目标进程中指定的值(比如游戏修改器可以修改游戏中的钱).
	问题4:
			解决了上面的问题,还有个问题需要解决:WriteProcessMemory()函数的第二个参数是指定写入目标进程内存的缓冲区起始地址,这个地址在目标进程中,
		那么这个地址在目标进程的哪个位置呢?目标进程中的内存块允许把DLL文件的路径写进去吗?
	解决:
			接下来要解决的问题是如何确定应该将DLL文件的完整路径写入目标进程的哪个地址.对于目标进程来说,事先是不会准备一块地址让用户进行写入的,
		用户能做的是自己在目标进程中申请一块内存,然后把DLL文件的路径进行写入,写入在目标进程新申请到的内存空间中,在目标进程中申请内存的函数是
		VirtualAllocEx(),其定义如下:
			LPVOID VirtualAllocEx{
				HANDLE hProcess,
				LPVOID lpAddress,
				SIZE_T dwSize,
				DWORD flAllocationType,
				DWORD flProtect
			};
		参数说明:
			hProcess:指定进程的进程句柄
			lpAddress:指定在目标进程中申请内存的起始地址,保留页面的内存地址；一般用NULL自动分配 。
			dwSize:指定在目标进程中申请内存的长度,字节单位；注意实际分配的内存大小是页内存大小的整数倍
			flAllocationType:指定申请内存的状态类型,通常为这个值:
				MEM_COMMIT：为特定的页面区域分配内存中或磁盘的页面文件中的物理存储
			flProtect:指定申请内存的属性,通常为这个值:
				PAGE_READWRITE 区域可被应用程序读写
				
		该函数的返回值是在目标进程申请到的内存块的起始地址.

	二. 卸载被注入的DLL文件
		DLL卸载使用的API函数是FreeLibrary(),定义如下:
		BOOL FreeLibrary(
			HMODULE HModule // handle to dll module
		)
		该函数的参数是要卸载的模块的句柄
			FreeLibrary()函数使用的模块句柄可以通过前面介绍的Module32Fisrt()和Module32Next两个函数获取.
		在使用Module32First()和Module32Next两个函数的时候,需要用到MODULEeNTRY32结构体,该结构体中保存了模块的句柄.
		该结构体中的hModule为模块的句柄,szModule为模块的名称,szExePath是 完整的模块的名称(即路径加模块名称)
*/
void adjustPrivilege();
void injectDLL(char *szDllName, int dwPid);
void unInjectDll(DWORD dwPid, char* szDllName);
void getDllNameAndProcessName(char *szDllName, char *szProcessName);
int getPidByEnumProcessName(char *szProcessName);

void main() {
	//printf("sizeof(SIZE_T)=%d,sizeof(DWORD)=%d\n",sizeof(SIZE_T), sizeof(DWORD));
	// 提升进程权限以便注入系统进程
	adjustPrivilege();
	char szDllName[MAX_PATH] = { 0 };
	char szProcessName[MAX_PATH] = { 0 };
	DWORD dwPid = 0;
	int option = 0;
	while (1) {
		printf("1. DLL注入  2. DLL卸载  3. 退出\n");
		scanf("%d", &option);

		// 退出
		if (3 == option)
		{
			break;
		}

		// 获取两个名称输入
		getDllNameAndProcessName(szDllName, szProcessName);

		// 采用枚举进程的方式根据进程名获取对应的Pid
		dwPid = getPidByEnumProcessName(szProcessName);

		switch (option)
		{
		case 1:
			injectDLL(szDllName, dwPid);
			break;

		case 2:
			printf("输入要卸载的模块名:\n");
			scanf("%s", szDllName);
			unInjectDll(dwPid, szDllName);
			break;
		}

	}

	system("pause");
}

/*
	注入指定的DLL文件到指定的进程中
	参数是存放两个名称的char数组指针
*/
void injectDLL(char *szDllName, int dwPid) {
	if (dwPid == -1 || szDllName == NULL || strlen(szDllName) == 0)
	{
		return;
	}

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if (hProcess == NULL)
	{
		DWORD dwErr = GetLastError();
		printf("打开进程失败:%ld\n", dwErr);
		return;
	}

	// 计算需要注入DLL文件完整路径的长度,加上1个结束符的位置
	 int dllLength = strlen(szDllName) + sizeof(char);
	// int dllLength = sizeof(WCHAR)*(wcslen(szDllName) + 1);

	// 在目标进程申请一块长度为dllLength大小的内存空间
	PVOID pDllAddress = VirtualAllocEx(hProcess, NULL, dllLength, MEM_COMMIT, PAGE_READWRITE);

	if (pDllAddress == NULL)
	{
		DWORD dwErr = GetLastError();
		printf("申请内存空间失败:%ld\n", dwErr);
		CloseHandle(hProcess);
		return;
	}

	// 实际写入内存的长度
	/*
		注意:类型不要写错,不是DWORD类型,是SIZE_T(统一传入SIZE_T即可),该类型在64位和32位程序中大小不一样,分别为8和4,DWORD恒为4.
			否则传递给WriteProcessMemory函数会导致堆栈损坏
		网上的分析:
			程序结束时，会报错"Run-Time Check Failure #2 - Stack around the variable 'xxx' was corrupted."
			A.造成此报错的原因一般是内存越界
			B.这里造成的原因是：WriteProcessMemory/ReadProcessMemory第5个参数在X32下要传入DWORD类型，在X64下要传入SIZE_T类型。
			DWORD无论在X32/X64下均占用4BYTE，而SIZE_T在X64下占用8BYTE。如果，在X64下你传入DWORD，所以会报内存越界的错误，即使用强制转换。
			X64下解决办法即是考虑与SIZE_T。
	*/
	SIZE_T dwWriteCount = 0;
	// 将需要注入的DLL文件的完整路径写入到目标进程中申请的内存空间
	WriteProcessMemory(hProcess, pDllAddress, szDllName, dllLength, &dwWriteCount);
	/*
		获取LoadLibrary()函数的地址,GetModuleHandle是返回一个已经映射进调用进程地址空间的模块的句柄
		注意:
			LoadLibrary只是一个宏,在编程时可以直接使用,如果需要获取它的地址,则需要明确指出是LoadLibraryA还是LoadLibraryW.
	*/
	FARPROC pFunAddress = GetProcAddress(GetModuleHandle("Kernel32.dll"), "LoadLibraryA");

	// 创建远程线程
	HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunAddress, pDllAddress, 0, NULL);

	if (hRemoteThread == NULL)
	{
		/*
			如果错误码为5,检查是否编码的32位版本,64位的程序不能用32位的来注入
		*/
		DWORD dwErr = GetLastError();
		printf("创建远程线程失败:%ld\n", dwErr);
		CloseHandle(hProcess);
		return;
	}

	WaitForSingleObject(hRemoteThread, INFINITE);

	CloseHandle(hRemoteThread);
	CloseHandle(hProcess);
}


/*
	卸载DLL
*/
void unInjectDll(DWORD dwPid, char* szDllName) {
	if (dwPid == 0 || NULL == szDllName || strlen(szDllName) == 0)
	{
		return;
	}
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);
	MODULEENTRY32 me = { 0 };
	me.dwSize = sizeof(me);

	//查找匹配的进程名称
	BOOL bRet = Module32First(snapshot, &me);
	while (bRet)
	{
		if (strcmp(me.szExePath, szDllName) == 0)
		{
			break;
		}
		bRet = Module32Next(snapshot, &me);
	}

	CloseHandle(snapshot);

	char *pFunName = "FreeLibrary";
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);

	if (hProcess == NULL)
	{
		return;
	}

	FARPROC pFunAddress = GetProcAddress(GetModuleHandle("kernel32.dll"), pFunName);

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pFunAddress, me.hModule, 0, NULL);
	if (NULL == hThread)
	{
		return;
	}

	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hProcess);
}

/*
	接收用户的输入保存到指定的char类型数组中.
*/
void getDllNameAndProcessName(char *szDllName, char *szProcessName) {
	printf("输入进程名称:\n");
	scanf("%s", szProcessName);
	printf("输入DLL名称:\n");
	scanf("%s", szDllName);
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
		if (lstrcmp(pe32.szExeFile, szProcessName) == 0)
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