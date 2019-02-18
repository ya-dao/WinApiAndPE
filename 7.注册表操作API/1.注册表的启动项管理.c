#define _CRT_SECURE_NO_WARNINGS

#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#define REG_RUN "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\"

void showRunList(char* masterKeyString, HKEY masterKey, LPCTSTR lpSubKey, int *order);
void enumRunList();
void addRunApplication();
void deleteRunApplication();

/*
1|  RTHDVCPL|"C:\Program Files\Realtek\Audio\HDA\RAVCpl64.exe" -s
2|          |
3|   Sysdiag|"D:\Huorong\Sysdiag\bin\HipsTray.exe"

1|vmware-tray.exe|"D:\虚拟机\虚拟机程序\vmware-tray.exe"
2|          |
3|SunloginClient|"D:\常用软件\SunloginClient\SunloginClient.exe" --cmd=autorun
*/
void main() {
	int option = 0;
	while (1)
	{
		printf("1.查看启动项\t2.添加启动项\t3.删除启动项\t4.退出\n");
		scanf("%d",&option);
		switch (option) {
		case 1:
			enumRunList();
			break;
		case 2:
			addRunApplication();
		case 3:
			deleteRunApplication();
			break;
		case 4:
			return;
		}
	}
	system("pause");
}

/*
	删除指定路径下的启动项:HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
*/
void deleteRunApplication() {
	char keyName[100] = { 0 };
	printf("输入键名:\n");
	scanf("%s", keyName);
	if (strlen(keyName) == 0) {
		printf("输入无效!");
		return;
	}
	// 打开注册表
	HKEY hKey = NULL;
	LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_RUN, 0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// 注册表打开失败
		printf("%s\n", "注册表打开失败");
		return;
	}

	RegDeleteValue(hKey, keyName);

	RegCloseKey(hKey);
}

/*
	遍历三个路径下的所有启动项
*/
void enumRunList() {
	int order = 1;
	showRunList("HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE, REG_RUN, &order);
	showRunList("HKEY_CURRENT_USER", HKEY_CURRENT_USER,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\", &order);
	showRunList("HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", &order);
	order = 1;
}

void showRunList(char* masterKeyString, HKEY masterKey, LPCTSTR lpSubKey, int *order) {
	printf("%s\\%s:\n", masterKeyString, lpSubKey);
	DWORD dwType = 0;
	DWORD dwBufferSize = MAXBYTE;
	DWORD dwKeySize = MAXBYTE;
	char szValueName[MAXBYTE] = { 0 };
	char szValueKey[MAXBYTE] = { 0 };

	HKEY hKey = NULL;
	// 打开注册表
	/* 注意:必须使用KEY_WOW64_64KEY权限值来访问64位注册表
		    使用KEY_WOW32_64KEY权限值来访问32位注册表
			否则注册表会被重定向.
			注册表会被重定向,参考:
			https://blog.csdn.net/is2120/article/details/7246334
	*/
	LONG lReturn = RegOpenKeyEx(
		masterKey, lpSubKey, 0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &hKey);
	if (lReturn != ERROR_SUCCESS)
	{
		return;
	}

	int i = 0;
	while (TRUE) {
		// 枚举键项
		lReturn = RegEnumValue(hKey,i,
			szValueName,&dwBufferSize,NULL,
			&dwType,(unsigned char*)szValueKey,&dwKeySize);
		// 没有则退出循环
		if (lReturn == ERROR_NO_MORE_ITEMS) {
			break;
		}
		// 打印出当前项
		printf("%d|", (*order)++);
		printf("%8s|", szValueName);
		printf("%10s\n", szValueKey);

		memset(szValueName, 0, MAXBYTE);
		memset(szValueKey, 0, MAXBYTE);

		i++;
	}
	printf("\n");
	RegCloseKey(hKey);
}

/*
	添加启动项到指定路径下:HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
*/
void addRunApplication() {
	char keyName[100] = { 0 };
	char keyValue[100] = { 0 };
	printf("输入键名:\n");
	scanf("%s", keyName);
	printf("输入键值:\n");
	scanf("%s", keyValue);
	if (strlen(keyName) == 0 || strlen(keyValue) == 0) {
		printf("输入无效!");
		return;
	}
	// 打开注册表
	HKEY hKey = NULL;
	LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_RUN, 0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, &hKey);
	if (lResult != ERROR_SUCCESS)
	{
		// 注册表打开失败
		printf("%s\n", "注册表打开失败");
		return;
	}

	// 添加注册表项  sizeof(char):C语言字符串以\0结束,所有加1
	RegSetValueEx(hKey, keyName, 0, REG_SZ, (const unsigned char*)keyValue, strlen(keyValue) + sizeof(char));

	// 关闭注册表
	RegCloseKey(hKey);
}
