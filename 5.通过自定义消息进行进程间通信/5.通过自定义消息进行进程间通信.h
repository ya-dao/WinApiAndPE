
// 5.通过自定义消息进行进程间通信.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// ClientApp: 
// 有关此类的实现，请参阅 5.通过自定义消息进行进程间通信.cpp
//

class ClientApp : public CWinApp
{
public:
	ClientApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern ClientApp theApp;