
// 1.Windows消息机制Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "1.Windows消息机制.h"
#include "1.Windows消息机制Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMy1Windows消息机制Dlg 对话框



CMy1Windows消息机制Dlg::CMy1Windows消息机制Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MY1WINDOWS_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMy1Windows消息机制Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMy1Windows消息机制Dlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMy1Windows消息机制Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CMy1Windows消息机制Dlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CMy1Windows消息机制Dlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CMy1Windows消息机制Dlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CMy1Windows消息机制Dlg 消息处理程序

BOOL CMy1Windows消息机制Dlg::OnInitDialog()
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

void CMy1Windows消息机制Dlg::OnPaint()
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
HCURSOR CMy1Windows消息机制Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/*
	打开记事本
*/
void CMy1Windows消息机制Dlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	WinExec("notepad.exe",SW_SHOW);
}

/*
	关闭记事本
*/
void CMy1Windows消息机制Dlg::OnBnClickedButton2()
{
	// 加_T()宏定义用于处理将LPCTSTR转换为支持Unicode的字符串
	HWND hWnd = ::FindWindow(_T("Notepad"), NULL);
	if (NULL == hWnd) {
		MessageBox(_T("没有找到记事本"),_T("提示"),0);
	}
	else
	{
		/*
			使用::的原因:
				::在这里表示使用类外的全局API函数,它与不加冒号的区别在于,它额外提供了一个参数,可以指明在哪个句柄中使用该函数
		*/
		::SendMessage(hWnd, WM_CLOSE, NULL, NULL);
	}
}

/*
	修改记事本标题
*/
void CMy1Windows消息机制Dlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	// HWND hWnd = ::FindWindow(_T("Notepad"), NULL);
	HWND hWnd = ::FindWindow(NULL, _T("无标题 - 记事本"));
	if (NULL == hWnd) {
		MessageBox(_T("没有找到记事本"), _T("提示"), 0);
	}
	else
	{
		wchar_t *pCaption = _T("消息机制测试");
		::SendMessage(hWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)pCaption);
	}
}

/*
	获取标题
*/
void CMy1Windows消息机制Dlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	// TODO: 在此添加控件通知处理程序代码
	HWND hWnd = ::FindWindow(_T("Notepad"), NULL);
	if (NULL == hWnd) {
		MessageBox(_T("没有找到记事本"), _T("提示"), 0);
	}
	else
	{
		char caption[1024] = { 0 };
		::SendMessage(hWnd, WM_GETTEXT, 1024, (LPARAM)caption);
		MessageBox((LPCTSTR)caption,_T("提示"),0);
	}
}
