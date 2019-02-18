// ServerDialg.cpp : 实现文件
//

#include "stdafx.h"
#include "5.通过自定义消息进行进程间通信.h"
#include "ServerDialg.h"
#include "afxdialogex.h"
#define WM_UMSG (WM_USER + 1)

// ServerDialg 对话框

IMPLEMENT_DYNAMIC(ServerDialg, CDialogEx)

ServerDialg::ServerDialg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{
	
}

ServerDialg::~ServerDialg()
{
}

void ServerDialg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ServerDialg, CDialogEx)
	// 接收消息映射
	ON_WM_SYSCOMMAND(ServerDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_UMSG, RevcMsg)
END_MESSAGE_MAP()


/*
	ServerDialg 消息处理程序
	在VC6中可以返回void,在VS中必须返回LRESULT类型,
	否则ON_MESSAGE映射中会报类型转换无效错误
*/
LRESULT ServerDialg::RevcMsg(WPARAM wParam, LPARAM lParam) {
	int num1, num2, sum;
	num1 = (int)wParam;
	num2 = (int)lParam;
	sum = num1 + num2;
	CString str;
	str.Format(L"%d",sum);
	SetDlgItemText(IDC_EDIT_RESULT, str);
	return NULL;
}
