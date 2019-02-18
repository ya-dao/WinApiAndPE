#define  _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>

/*
	线程创建函数,定义如下:
		HANDLE thread = CreateThread(
			NULL, // 安全选项,通常为NULL
			NULL, // 新线程的栈大小,通常为NULL,与主线程大小相同
			threadProc, // 线程执行函数的地址
			(void*)&a, // 线程函数参数
			0, // 创建的选项,比如CREATE_SUSPEND创建之后先挂起,等待激活
			&threadId //返回新创建的线程ID
		);
	同步函数定义如下:
		等待一个线程的函数:
		WaitForSingleObject(
			HANDLE hHandle,	// 需要等待的对象句柄
			DWORD dwMillliseconds // 指定等待超时的毫秒数,如果设为0,则会立即返回,如果为INFINITE表示一值等待线程函数的返回
		);
		等待一个线程的函数:
		WaitForMultipleObjects(
			DWORD nCount,
			CONST HANDLE *lpHandles, // 需要等待的对象句柄数组
			BOOL fWaitAll, // 表示是否等待全部线程的状态完成,如果设置为TRUE,则等待全部.
			DWORD dwMillliseconds  // 该参数指定等待超时的毫秒数,如果设为0,则会立即返回,如果为INFINITE表示一值等待线程函数的返回
		);
*/

DWORD WINAPI threadProc(LPVOID lpParameter) {
	int *a = (int *)lpParameter;
	printf("thread parameter is %d\n", *a);
}
void main() {
	int a = 2;
	DWORD threadId;
	HANDLE thread = CreateThread(
		NULL, // 安全选项,通常为NULL
		NULL, // 新线程的栈大小,通常为NULL,与主线程大小相同
		threadProc, // 线程执行函数的地址
		(void*)&a, // 线程函数参数
		0, //创建的选项,比如CREATE_SUSPEND创建之后先挂起,等待激活
		&threadId //返回新创建的线程ID
	);
	printf("main thread created a thread which id is %d\n", threadId);

	WaitForSingleObject(thread, INFINITE);

	CloseHandle(thread);
	system("pause");
}