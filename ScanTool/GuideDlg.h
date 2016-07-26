#pragma once
#include "global.h"
#include "ScanToolDlg.h"

// CGuideDlg 对话框

class CGuideDlg : public CDialog
{
	DECLARE_DYNAMIC(CGuideDlg)

public:
	CGuideDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CGuideDlg();

// 对话框数据
	enum { IDD = IDD_GUIDEDLG };

public:
	pMODEL	m_pModel;
	CScanToolDlg* m_pScanDlg;

private:
	void InitConf();
	void InitLog();
	void InitCtrlPosition();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedBtnScan();
	afx_msg void OnBnClickedBtnExit();
	afx_msg void OnBnClickedBtnModel();
	afx_msg void OnBnClickedBtnParam();
	afx_msg void OnDestroy();
};
