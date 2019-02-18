
// APIKeyBoardDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "4.通过API函数模拟鼠标和键盘的操作.h"
#include "APIKeyBoardDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// APIKeyBoardDlg 对话框



APIKeyBoardDlg::APIKeyBoardDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MY4API_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void APIKeyBoardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(APIKeyBoardDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(Button_keyboard, &APIKeyBoardDlg::OnBnClickedkeyboard)
	ON_BN_CLICKED(button_mouse, &APIKeyBoardDlg::OnBnClickedmouse)
END_MESSAGE_MAP()


// APIKeyBoardDlg 消息处理程序

BOOL APIKeyBoardDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void APIKeyBoardDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR APIKeyBoardDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void APIKeyBoardDlg::OnBnClickedkeyboard()
{
	// TODO: 在此添加控件通知处理程序代码
	// 找到窗口并获取焦点
	findAndFocus();
	Sleep(1000);

	// 模拟F5三次
	keybd_event(VK_F5, 0, 0, 0);
	keybd_event(VK_F5, 0, 2, 0);
	Sleep(1000);
	keybd_event(VK_F5, 0, 0, 0);
	keybd_event(VK_F5, 0, 2, 0);
	Sleep(1000);
	keybd_event(VK_F5, 0, 0, 0);
	keybd_event(VK_F5, 0, 2, 0);
}

void APIKeyBoardDlg::findAndFocus() {
	CString caption = NULL;
	GetDlgItemText(Edit_Caption, caption);

	//判断输入是否为空
	if ("" == caption) {
		return;
	}

	HWND hWnd = ::FindWindow(NULL, caption.GetBuffer(0));
	// 将指定窗口设置到前台并获取焦点
	::SetForegroundWindow(hWnd);
}

void APIKeyBoardDlg::OnBnClickedmouse()
{
	// TODO: 在此添加控件通知处理程序代码
	findAndFocus();

	CString caption = NULL;
	GetDlgItemText(Edit_Caption, caption);
	HWND hWnd = ::FindWindow(NULL, caption.GetBuffer(0));

	// 得到窗口在屏幕的坐标{x,y}
	POINT point = { 0 };
	// 将窗口区域的坐标转换为屏幕的坐标,即得到指定窗口在屏幕中的坐标位置
	::ClientToScreen(hWnd, &point);

	/*
		设置鼠标位置,该函数通常和GetCursorPos一起使用,
		在设置之前先获取保存当前鼠标位置,操作完成之后需要将鼠标设置回原来的位置
		mouse_event在这一点上没有该函数方便
	*/
	SetCursorPos(point.x + 36, point.y + 100);

	// 模拟单击鼠标右键,浏览器会弹出右键菜单
	mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
	Sleep(1000);
	mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
	Sleep(1000);

	// 在弹出菜单后按下R键即可刷新,R键的虚拟码为0x52
	keybd_event(0x52, 0, 0, 0);
}
