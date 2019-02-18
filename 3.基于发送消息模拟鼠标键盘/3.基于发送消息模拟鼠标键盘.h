
// 3.基于发送消息模拟鼠标键盘.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// CKeyBoardApp: 
// 有关此类的实现，请参阅 3.基于发送消息模拟鼠标键盘.cpp
//

class CKeyBoardApp : public CWinApp
{
public:
	CKeyBoardApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CKeyBoardApp theApp;