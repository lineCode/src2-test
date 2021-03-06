#pragma once
#include "global.h"
#include "ExamInfoDlg.h"
// CPapersInfoSaveDlg 对话框

class CPapersInfoSaveDlg : public CDialog
{
	DECLARE_DYNAMIC(CPapersInfoSaveDlg)

public:
	CPapersInfoSaveDlg(pPAPERSINFO pPapers, pMODEL pModel, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPapersInfoSaveDlg();

// 对话框数据
	enum { IDD = IDD_PAPERSINFOSAVEDLG };

public:
	CString		m_strPapersName;
	int			m_nPaperCount;
	CString		m_strPapersDetail;

	CComboBox	m_comboExamName;
	CComboBox	m_comboSubject;

	int			m_nExamID;
	CString		m_strExamID;
	CString		m_strExamTypeName;
	CString		m_strGradeName;
	int			m_SubjectID;
private:
	pPAPERSINFO m_pPapers;
	pMODEL		m_pModel;
	CExamInfoDlg* m_pExamInfoDlg;
	void	InitUI(int nShow);
	void	InitCtrlPosition();
protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeComboExamname();
	afx_msg void OnCbnSelchangeComboSubjectname();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
