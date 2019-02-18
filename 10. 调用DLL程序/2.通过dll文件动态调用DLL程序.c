#include<Windows.h>
/*
	动态调用: 动态调用不是在链接时完成的.动态调用不会在可执行文件中写入DLL的相关信息.
	HMODULE LoadLibrary(LPCTSTR lpFileName)
		lpFileName: 通过给定的DLL文件名加载DLL文件
		该函数执行成功的情况下返回一个模块句柄
	FARPROC GetProcAddress(HMODULE module, LPCTSTR lpProcName):
		module: 模块句柄, 通常通过LoadLibrary()或者GetModuleHandle函数获得
		lpProcName: 指定要获取函数地址的函数名称
		该函数执行成功的情况下返回lpProcName指向的函数名的函数地址
*/

typedef void(*PFUNMSG)(char *);

void main() {
	HMODULE module = LoadLibrary("9.DLL编程.dll");
	if (module == NULL)
	{
		MessageBox(NULL, "9.DLL编程.dll文件不存在", "DLL文件加载失败", MB_OK);
		return;
	}
	PFUNMSG pFunMsg = (PFUNMSG)GetProcAddress(module, "MsgBox");
	pFunMsg("try to invoke MsgBox method in dll");
	system("pause");
}