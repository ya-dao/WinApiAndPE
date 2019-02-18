#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include <WinUser.h>
/*
	WinMain函数中的流程:
		1. 注册一个窗口类,创建该窗口并显示创建的窗口
		2. 不停的获取属于自己的消息并分发给自己的窗口处理过程
		3. 直到收到WM_QUIT消息后退出消息循环结束进程
*/

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL initInstance(HINSTANCE hInstance, int cmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(
	// 当前实例句柄
	HINSTANCE hInstance,
	// 同一个资源文件创建的上一个实例句柄,该参数是Win16平台下的遗留物,Win32下已经不再使用
	HINSTANCE previousInstance,
	// 启动主函数时所传入的参数
	LPSTR lpCmdLine,
	// 进程的显示方式(最大化/最小化/隐藏等方式)
	int nCmdShow
) {
	MSG msg;
	BOOL result;

	// 注册窗口类
	ATOM atom = MyRegisterClass(hInstance);
	if (0 == atom)
	{
		MessageBox(NULL, L"注册类失败", L"提示", 0);
		return 0;
	}


	// 创建窗口并显示
	BOOL flag = initInstance(hInstance, SW_NORMAL);
	if (!flag) {
		return FALSE;
	}

	// 消息循环,获取属性自己的消息并进行分发
	while ((result = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (result == -1) {
			// 取到错误码了,即将退出
			break;
		}
		else
		{
			// 处理键盘消息,将虚拟码消息转换成字符消息
			TranslateMessage(&msg);
			// 将消息分发到窗口处理过程中去
			DispatchMessage(&msg);
		}
	}
}

/*
	注册窗口类的函数,流程如下:
	1. 填充WNDCLASSEX结构体
	2. 调用RegisterClassEx进行注册
*/
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEX windowClass;

	// 填充结构体为0
	ZeroMemory(&windowClass, sizeof(WNDCLASSEX));

	// cbSize是结构体大小
	windowClass.cbSize = sizeof(WNDCLASSEX);
	// ipfnProc是窗口处理过程地址
	windowClass.lpfnWndProc = WindowProc;
	// hInstance是实例句柄
	windowClass.hInstance = hInstance;
	// lpszClassName是窗口类类名
	windowClass.lpszClassName = L"firstWindow";
	// style是窗口风格
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	// hbrBackground是窗口类背景色
	windowClass.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME + 1;
	// hCursor是鼠标句柄
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	// hIcon是图标句柄
	windowClass.hIcon = LoadIcon(NULL, IDI_QUESTION);
	// 其他
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;

	return RegisterClassEx(&windowClass);
}

/*
	创建主窗口并显示
*/
BOOL initInstance(HINSTANCE hInstance, int cmdShow) {
	HWND hWnd = NULL;
	// 创建窗口
	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"firstWindow",
		L"MyFirstWindow",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL
	);
	if (NULL == hWnd)
	{
		MessageBox(NULL, L"创建窗口失败", L"提示", 0);
		return FALSE;
	}
	else {
		// 显示窗口
		ShowWindow(hWnd, cmdShow);
		// 更新窗口
		UpdateWindow(hWnd);
	}
	return TRUE;
}

/*
	处理消息的窗口过程,函数名随意,但必须与前面注册窗口时填的HWDCLASSEX结构体中的ipfnWndProc的成员变量的值一样
*/
LRESULT CALLBACK WindowProc (
	HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam
	) {
	PAINTSTRUCT paint;
	HDC hDc;
	RECT rect;

	WCHAR *pszDrawText = L"Hello Window Program";

	switch (uMsg)
	{
	case WM_PAINT:
		hDc = BeginPaint(hWnd, &paint);
		GetClientRect(hWnd, &rect);
		/*
			对于宽字符求长度需要使用wcs开头的函数
		*/
		DrawText(hDc, pszDrawText, wcslen(pszDrawText), &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		EndPaint(hWnd, &paint);
		break;
	case WM_CLOSE:
		if (IDYES == MessageBox(hWnd, L"是否退出程序", L"提示", MB_YESNO))
		{
			// 销毁窗口
			DestroyWindow(hWnd);
			// 退出消息循环
			PostQuitMessage(0);
		}
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}