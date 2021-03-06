
// sendFileTestDlg.h : 头文件
//

#pragma once
#include "global.h"
#include "SendFileThread.h"
#include "MulticastServer.h"

// CsendFileTestDlg 对话框
class CsendFileTestDlg : public CDialogEx
{
// 构造
public:
	CsendFileTestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SENDFILETEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	std::vector<CSendFileThread*> m_vecRecogThreadObj;
	Poco::Thread* m_pRecogThread;

	MulticastServer*	m_pMulticastServer;

	CMFCEditBrowseCtrl	m_ctrlSendFile;

	int		m_nThreads;
	int		m_nFileTasks;
	std::string m_strServerIP;
	int			m_nServerPort;
	std::string m_strMulticastIP;
	int			m_nMulticastPort;

	int		m_nChildProcess;
	int		m_nChildThreads;
private:
	void InitConfig();
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnSendfile();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnSendtest();
	afx_msg void OnBnClickedBtnMulticast();
	afx_msg void OnBnClickedBtnStartchild();
	afx_msg void OnBnClickedBtnStartchildthread();
	afx_msg void OnBnClickedBtnTest();
	afx_msg void OnBnClickedBtnTest2();
};
