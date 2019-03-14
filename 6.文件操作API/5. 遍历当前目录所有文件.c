#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
void main() {
	/*
	FindFirstFile第一个参数举例
		1. c:\\Windows\\System32\\*.dll
			在c:\Windows\System32目录中查找所有dll文件
		2. *.* 
			在当前目录查找所有文件
	*/
	char *szFileName = NULL;
	LPCTSTR lpFileName = "*.exe";
	WIN32_FIND_DATA pNextInfo;
	HANDLE hSearch = FindFirstFile(lpFileName, &pNextInfo);
	if (hSearch == INVALID_HANDLE_VALUE)
	{
		printf("无目标文件!\n");
	}
	else {
		do {
			szFileName = pNextInfo.cFileName;
			printf("得到文件：%s\n", szFileName);
		} while (FindNextFile(hSearch, &pNextInfo));
		FindClose(hSearch);
	}
	system("pause");
}