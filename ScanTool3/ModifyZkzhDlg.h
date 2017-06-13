#pragma once

#include "global.h"
#include "PicShow.h"
#include "XListCtrl.h"
#include "StudentMgr.h"
#include "VagueSearchDlg.h"
#include "ShowPicDlg.h"
//#include "TipBaseDlg.h"
// CModifyZkzhDlg 对话框

#define	NewListModelTest	//测试

class CModifyZkzhDlg : public CDialog
{
	DECLARE_DYNAMIC(CModifyZkzhDlg)

public:
	CModifyZkzhDlg(pMODEL pModel, pPAPERSINFO pPapersInfo, CStudentMgr* pStuMgr, pST_PaperInfo pShowPaper = NULL, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CModifyZkzhDlg();

// 对话框数据
	enum { IDD = IDD_MODIFYZKZHDLG };

	CXListCtrl		m_lcZkzh;				//需要修改的准考证号列表

	pMODEL			m_pModel;				//扫描试卷时的校验模板
	pPAPERSINFO		m_pPapers;
	pST_PaperInfo	m_pCurrentShowPaper;
	int				m_nCurrentSelItem;		//当前选择的项

	CString			m_strCurZkzh;
	COLORREF		crOldText, crOldBackground;

// 	int				m_nSearchType;		//搜索类型，1-按姓名搜索，2-按准考证号
// 	CString			m_strSearchKey;		//搜索关键字
//	CListCtrl		m_lcBmk;			//报名库列表控件
private:
	CStudentMgr*	m_pStudentMgr;
	pST_PaperInfo	m_pShowPaper;		//默认显示的试卷
	CVagueSearchDlg* m_pVagueSearchDlg;	//模糊搜索窗口
	CShowPicDlg*	m_pShowPicDlg;		//图片显示窗口
private:
	void	InitUI();
	void	InitTab();
	void	InitCtrlPosition();
	void	InitData();
	bool	ReleaseData();

	bool	VagueSearch(int nItem);		//对某项进行模糊查找

	LRESULT RoiRBtnUp(WPARAM wParam, LPARAM lParam);
	void	LeftRotate();
	void	RightRotate();

 	void	ShowPaperByItem(int nItem);
// 	void	ShowPaperZkzhPosition(pST_PaperInfo pPaper);
//	void	PrintRecogRect(int nPic, pST_PaperInfo pPaper, pST_PicInfo pPic, cv::Mat& matImg);		//打印所有模板上的矩形位置
	BOOL	PreTranslateMessage(MSG* pMsg);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	afx_msg LRESULT OnEditEnd(WPARAM, LPARAM);
	afx_msg LRESULT OnLBtnDownEdit(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMDblclkListZkzh(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMHoverListZkzh(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnNMDblclkListZkzhsearchresult(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClose();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
