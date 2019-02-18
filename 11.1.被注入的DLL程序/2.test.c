#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>

//BOOL WINAPI DllMain(
//	_In_ HINSTANCE hinstDLL, // 指向自身的句柄
//	_In_ DWORD fdwReason, // 调用原因
//	_In_ LPVOID lpvReserved // 隐式加载和显式加载
//){
//	switch (fdwReason)
//	{
//	case DLL_PROCESS_ATTACH:
//		MessageBox(NULL, L"注入成功",L"TIPS",MB_OK);
//		MessageBox(NULL, L"DLL被进程加载", L"TIPS", MB_OK);
//		break;
//	case DLL_PROCESS_DETACH:
//		MessageBox(NULL, L"DLL被进程卸载", L"TIPS", MB_OK);
//		break;
//	case DLL_THREAD_ATTACH:
//		MessageBox(NULL, L"DLL被加载的进程有新线程创建", L"TIPS", MB_OK);
//		break;
//	case DLL_THREAD_DETACH:
//		MessageBox(NULL, L"DLL被加载的进程有线程结束", L"TIPS", MB_OK);
//		break;
//	}
//	return TRUE;
//}

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
