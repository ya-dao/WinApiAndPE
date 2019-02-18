
// 4.通过API函数模拟鼠标和键盘的操作.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// APIKeyBoardAPP: 
// 有关此类的实现，请参阅 4.通过API函数模拟鼠标和键盘的操作.cpp
//

class APIKeyBoardAPP : public CWinApp
{
public:
	APIKeyBoardAPP();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern APIKeyBoardAPP theApp;