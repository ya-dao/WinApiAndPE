
// APIKeyBoardDlg.h : 头文件
//

#pragma once


// APIKeyBoardDlg 对话框
class APIKeyBoardDlg : public CDialogEx
{
// 构造
public:
	APIKeyBoardDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MY4API_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedkeyboard();
	void findAndFocus();
	afx_msg void OnBnClickedmouse();
};
