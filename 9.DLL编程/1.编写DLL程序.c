#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>

/*
1. DLL程序的入口函数
	BOOL WINAPI DllMain(
		HINSTANCE hInstanceDLL, 
		DWORD reasonForCall, 
		LPVOID lpReserved
	) {
	参数说明:
		hInstanceDLL: 当前DLL模块的句柄,即本动态链接库的实例句柄
		reasonForCall: DLLMain()函数被调用的原因.
			该参数的取值有4种,即存在4种调用DllMain函数的情况,这4个值分别是
			DLL_PROCESS_ATTACH: 当DLL被某进程加载时,DllMain函数被调用
			DLL_PROCESS_DETACH: 当DLL被某进程卸载时,DllMain函数被调用
			DLL_THREAD_ATTACH: 当进程中有线程被创建时,DllMain函数被调用
			DLL_THREAD_DETACH: 当进程中有线程结束时,DllMain函数被调用
		lpReserved:保留参数,即不被程序员使用的参数
2.	由于DllMain()函数不止一次的被调用,根据调用的情况不同,需要执行不同的代码,
	比如当进程加载该DLL文件时,可能在DLL中要申请一些资源;
	而在卸载该DLL时,则需要释放申请的资源.所以在编写Dll程序时,需要在里面加上switch结构判断其调用原因.
3. 添加导出函数
	(extern "C") __declspec(dllexport) VOID MsgBox(char *szMsg);
	extern "C": 在C++代码中使用,表示以C方式导出,被extern "C"修饰的变量和函数是按照C语言方式进行编译和链接的
	__declspec(dllexport): 声明一个导出函数,将该函数从本DLL中开放提供给其他模块使用.
4. 导出DLL中函数的两种方法:
	4.1 __declspec(dllexport): 在代码较少的时候比较方便
	4.2 .DEF文件,在代码较多的情况下可以较为方便的管理DLL中的导出函数

5. 该程序编译后会生成两个文件,分别是.lib和.dll.
	.lib: 库文件,其中包含一些导出函数的相关信息.
	.dll: dll文件,其中包含编写的代码
*/

__declspec(dllexport) VOID MsgBox(char *szMsg);
void virusCode();

BOOL WINAPI DllMain(
	HINSTANCE hInstanceDLL, // 当前DLL模块的句柄,即本动态链接库的实例句柄
	DWORD reasonForCall, 
	LPVOID lpReserved
) {
	switch (reasonForCall)
	{
	case DLL_PROCESS_ATTACH:
		MsgBox("DLL被进程加载");
		/*
			当DLL被加载时,我们可以让其里面的指定函数以达到我们想要的效果
		*/
		virusCode();
		break;
	case DLL_PROCESS_DETACH:
		MsgBox("DLL被进程卸载");
		break;
	case DLL_THREAD_ATTACH:
		MsgBox("DLL被加载的进程有新线程创建");
		break;
	case DLL_THREAD_DETACH:
		MsgBox("DLL被加载的进程有线程结束");
		break;
	}
	return TRUE;
}

void MsgBox(char *szMsg) {
	char szModuleName[MAXBYTE] = { 0 };
	GetModuleFileName(NULL, szModuleName, MAXBYTE);
	MessageBox(NULL, szMsg, szModuleName, MB_OK);
}

void virusCode() {
	char szModuleName[MAXBYTE] = { 0 };
	GetModuleFileName(NULL, szModuleName, MAXBYTE);
	MessageBox(NULL, "假设这里是一段恶意代码", szModuleName, MB_OK);
}