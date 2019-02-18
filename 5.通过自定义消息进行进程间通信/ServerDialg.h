#pragma once


// ServerDialg 对话框

class ServerDialg : public CDialogEx
{
	DECLARE_DYNAMIC(ServerDialg)

public:
	ServerDialg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~ServerDialg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT RevcMsg(WPARAM wParam, LPARAM lParam);
};
