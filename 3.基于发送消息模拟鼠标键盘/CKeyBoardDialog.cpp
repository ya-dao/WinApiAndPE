
// CKeyBoardDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "3.基于发送消息模拟鼠标键盘.h"
#include "CKeyBoardDialog.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CKeyBoardDialog 对话框



CKeyBoardDialog::CKeyBoardDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MY3_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKeyBoardDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKeyBoardDialog, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CKeyBoardDialog::OnBnClickedButton1)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CKeyBoardDialog 消息处理程序

BOOL CKeyBoardDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CKeyBoardDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CKeyBoardDialog::OnPaint()
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
HCURSOR CKeyBoardDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CKeyBoardDialog::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	// 获取控件
	CButton *pBtn = (CButton*)GetDlgItem(IDC_BUTTON1);
	HWND hButton = pBtn->GetSafeHwnd();

	CString buttonText = NULL;
	int interval = 0;

	// 获取输入的浏览器标题
	GetDlgItemText(IDC_EDIT1, caption);
	// 获取输入的刷新间隔
	interval = GetDlgItemInt(IDC_EDIT2, FALSE, TRUE);

	// 判断输入的值是否合法
	if (caption == "" || interval == 0) {
		return;
	}


	// 获取按钮的标题
	pBtn->GetWindowTextW(buttonText);

	if (buttonText == "开始") {
		// 设置定时器
		SetTimer(1, interval * 1000, NULL);
		pBtn->SetWindowTextW(L"停止");
		// 获取输入框对象并设置不可编辑
		GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
	}
	else {
		// 结束定时器
		KillTimer(1);
		pBtn->SetWindowTextW(L"开始");
		// 获取输入框对象并设置可编辑
		GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT2)->EnableWindow(TRUE);
	}
}


void CKeyBoardDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	HWND hWnd = ::FindWindow(NULL, caption.GetBuffer(0));
	
	// 发送键盘按下.（lParam）低16位（0001）表示按键一次
	::PostMessage(hWnd, WM_KEYDOWN, VK_F5, 1);
	// 发送键盘抬起
	::PostMessage(hWnd, WM_KEYUP, VK_F5, 1);

	CDialog::OnTimer(nIDEvent);
}