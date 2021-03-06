// PaperInputDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ScanTool3.h"
#include "ScanTool3Dlg.h"

#include "PaperInputDlg.h"
#include "afxdialogex.h"
#include <string.h>
#include <algorithm>
#include "ModifyZkzhDlg.h"
#include "PapersMgr.h"
#include "NewMessageBox.h"

using namespace cv;
using namespace std;
// CPaperInputDlg 对话框

IMPLEMENT_DYNAMIC(CPaperInputDlg, CDialog)

CPaperInputDlg::CPaperInputDlg(pMODEL pModel, CWnd* pParent /*=NULL*/)
	: CDialog(CPaperInputDlg::IDD, pParent)
	, m_strPapersPath(_T("")), m_nModelPicNums(1), /*m_nCurrTabSel(0), m_pCurrentPicShow(NULL),*/ m_pModel(pModel), m_pOldModel(pModel)
	, m_strModelName(_T("")), m_strPapersName(_T("")), m_strPapersDesc(_T("")), m_nCurrItemPapers(-1)
	, m_colorStatus(RGB(0, 0, 255)), m_nStatusSize(20), m_pCurrentShowPaper(NULL), m_nCurrItemPaperList(-1), m_pCurrentPapers(NULL), m_pStudentMgr(NULL)
	, m_pShowPicDlg(NULL), m_pAnswerShowDlg(NULL)
{

}

CPaperInputDlg::~CPaperInputDlg()
{
	if (m_pModel != m_pOldModel)
		SAFE_RELEASE(m_pModel);
	if (m_pShowPicDlg)
	{
		m_pShowPicDlg->DestroyWindow();
		SAFE_RELEASE(m_pShowPicDlg);
	}
	if (m_pAnswerShowDlg)
	{
		m_pAnswerShowDlg->DestroyWindow();
		SAFE_RELEASE(m_pAnswerShowDlg);
	}
}

void CPaperInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_Papers, m_lPapersCtrl);
	DDX_Control(pDX, IDC_LIST_Paper, m_lPaperCtrl);
//	DDX_Control(pDX, IDC_LIST_IssuePaper, m_lIssuePaperCtrl);
	DDX_Control(pDX, IDC_COMBO_ModelList, m_comboModel);
	DDX_Control(pDX, IDC_BTN_Broswer, m_btnBroswer);
	DDX_Text(pDX, IDC_EDIT_PapersPath, m_strPapersPath);
	DDX_Text(pDX, IDC_EDIT_ModelInfo, m_strModelName);
	DDX_Text(pDX, IDC_EDIT_PapersName, m_strPapersName);
	DDX_Text(pDX, IDC_EDIT_PapersDesc, m_strPapersDesc);
}


BEGIN_MESSAGE_MAP(CPaperInputDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BTN_Broswer, &CPaperInputDlg::OnBnClickedBtnBroswer)
	ON_BN_CLICKED(IDC_BTN_Start, &CPaperInputDlg::OnBnClickedBtnStart)
	ON_CBN_SELCHANGE(IDC_COMBO_ModelList, &CPaperInputDlg::OnCbnSelchangeComboModellist)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_Papers, &CPaperInputDlg::OnNMDblclkListPapers)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_Paper, &CPaperInputDlg::OnNMDblclkListPaper)
//	ON_NOTIFY(NM_DBLCLK, IDC_LIST_IssuePaper, &CPaperInputDlg::OnNMDblclkListIssuepaper)
	ON_MESSAGE(WM_CV_LBTNDOWN, &CPaperInputDlg::RoiLBtnDown)
	ON_BN_CLICKED(IDC_BTN_SAVE_PAPERSINPUTDLG, &CPaperInputDlg::OnBnClickedBtnSave)
	ON_MESSAGE(MSG_ZKZH_RECOG, &CPaperInputDlg::MsgZkzhRecog)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST_Paper, &CPaperInputDlg::OnLvnKeydownListPaper)
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_NOTIFY(NM_HOVER, IDC_LIST_Paper, &CPaperInputDlg::OnNMHoverListPaper)
	ON_NOTIFY(NM_HOVER, IDC_LIST_Papers, &CPaperInputDlg::OnNMHoverListPapers)
END_MESSAGE_MAP()

BOOL CPaperInputDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_vecCHzkzh.clear();
	USES_CONVERSION;
	InitUI();
	SeachModel();
	if (NULL != m_pModel)
	{
		for (int i = 0; i < m_comboModel.GetCount(); i++)
		{
			CString strItemName;
			m_comboModel.GetLBText(i, strItemName);
			if (strItemName == A2T(m_pModel->strModelName.c_str()))
			{
				m_comboModel.SetCurSel(i);
				m_ncomboCurrentSel = i;
				break;
			}
		}
	}
	else
		m_comboModel.SetCurSel(0);

	InitTmpSubjectBmk();
	return TRUE;
}

void CPaperInputDlg::InitUI()
{
	if (m_pModel)
		m_nModelPicNums = m_pModel->nPicNum;

	USES_CONVERSION;
	if (!m_pShowPicDlg)
	{
		m_pShowPicDlg = new CShowPicDlg(this);
		m_pShowPicDlg->Create(CShowPicDlg::IDD, this);
		m_pShowPicDlg->ShowWindow(SW_SHOW);
	}
	m_pShowPicDlg->setShowModel(2);

	if (!m_pAnswerShowDlg)
	{
		m_pAnswerShowDlg = new CAnswerShowDlg(this);
		m_pAnswerShowDlg->Create(IDD_ANSWERSHOWDLG, this);
		m_pAnswerShowDlg->ShowWindow(SW_SHOW);

//		m_pAnswerShowDlg->InitModel(m_pModel);
	}

	m_lPapersCtrl.DeleteAllItems();

	while (m_lPapersCtrl.DeleteColumn(0));
	m_lPapersCtrl.SetExtendedStyle(m_lPapersCtrl.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_SHOWSELALWAYS);
	m_lPapersCtrl.InsertColumn(0, _T("试卷袋名"), LVCFMT_CENTER, 80);
	m_lPapersCtrl.InsertColumn(1, _T("数量"), LVCFMT_CENTER, 36);

	m_lPaperCtrl.DeleteAllItems();
	while (m_lPaperCtrl.DeleteColumn(0));
	m_lPaperCtrl.SetExtendedStyle(m_lPaperCtrl.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_SHOWSELALWAYS);
	m_lPaperCtrl.InsertColumn(0, _T("题卡"), LVCFMT_CENTER, 36); 
//	m_lPaperCtrl.InsertColumn(1, _T("考生"), LVCFMT_CENTER, 36);
	m_lPaperCtrl.InsertColumn(1, _T("试卷名"), LVCFMT_CENTER, 130);
//	m_lPaperCtrl.InsertColumn(3, _T("*"), LVCFMT_CENTER, 20);

// 	m_lIssuePaperCtrl.DeleteAllItems();
// 	m_lIssuePaperCtrl.SetExtendedStyle(m_lIssuePaperCtrl.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_SHOWSELALWAYS);
// 	m_lIssuePaperCtrl.InsertColumn(0, _T("序号"), LVCFMT_CENTER, 36);
// 	m_lIssuePaperCtrl.InsertColumn(1, _T("试卷名"), LVCFMT_CENTER, 80);
// 	m_lIssuePaperCtrl.InsertColumn(2, _T("原因"), LVCFMT_LEFT, 100);
// 	m_lIssuePaperCtrl.EnableToolTips(TRUE);

	m_comboModel.AdjustDroppedWidth();

	SetFontSize(m_nStatusSize);
	
	InitCtrlPosition();	
}

// CPaperInputDlg 消息处理程序

void CPaperInputDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	InitCtrlPosition();
}

void CPaperInputDlg::InitCtrlPosition()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.right;
	int cy = rcClient.bottom;

	int nGap = 2;	//控件的间隔
	int nTopGap = 2;	//距离上边边缘的间隔
	int nBottomGap = 0;	//距离下边边缘的间隔
	int nLeftGap = 2;	//距离左边边缘的间隔
	int nRightGap = 2;	//距离右边边缘的间隔
	int nStaticHeight = 20;						//静态提示控件高度
	int nLeftCtrlWidth = rcClient.Width() / 3;		//左边控件的最大宽度
	if (nLeftCtrlWidth > 300)
		nLeftCtrlWidth = 300;
	int nListWidth = (nLeftCtrlWidth - nGap) / 2;	//List列表控件的宽度
	int nCurrentTop = nTopGap;
	int nCurrentLeft = nLeftGap;
	if (GetDlgItem(IDC_STATIC_PathTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PathTips)->MoveWindow(nCurrentLeft, nCurrentTop, nLeftCtrlWidth, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	if (GetDlgItem(IDC_EDIT_PapersPath)->GetSafeHwnd())
	{
		GetDlgItem(IDC_EDIT_PapersPath)->MoveWindow(nCurrentLeft, nCurrentTop, nLeftCtrlWidth - nGap - 20, nStaticHeight);
//		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	if (m_btnBroswer.GetSafeHwnd())
	{
		m_btnBroswer.MoveWindow(nCurrentLeft + nLeftCtrlWidth - 20, nCurrentTop, 20, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	int nModelStaticWidth = 65;		//模板列表提示控件的宽度
	if (GetDlgItem(IDC_STATIC_ModelTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_ModelTips)->MoveWindow(nCurrentLeft, nCurrentTop, nModelStaticWidth, nStaticHeight);
		nCurrentLeft += (nModelStaticWidth + nGap);
	}
	if (m_comboModel.GetSafeHwnd())
	{
		m_comboModel.MoveWindow(nCurrentLeft, nCurrentTop, nLeftCtrlWidth - nModelStaticWidth - nGap, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
		nCurrentLeft = nLeftGap;
	}
#if 1
	if (GetDlgItem(IDC_STATIC_PapersTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PapersTips)->MoveWindow(nCurrentLeft, nCurrentTop, nLeftCtrlWidth, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	int nPapersListWidth = static_cast<int>(nLeftCtrlWidth * 0.4);
	int nIssueListWdith = nLeftCtrlWidth - nPapersListWidth - nGap;
	int nListH = (rcClient.Height() - nCurrentTop) * 0.6;
	if (m_lPapersCtrl.GetSafeHwnd())
	{
		m_lPapersCtrl.MoveWindow(nCurrentLeft, nCurrentTop, nPapersListWidth, nListH);
	}
	if (GetDlgItem(IDC_STATIC_PaperTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PaperTips)->MoveWindow(nCurrentLeft + nPapersListWidth + nGap, nCurrentTop - nStaticHeight - nGap, nIssueListWdith, nStaticHeight);		//与试卷袋列表提示齐平
	}
	int nPaperListHeight = (rcClient.Height() - nCurrentTop - nGap - nStaticHeight - nGap) / 2;		//试卷列表控件的高度
	if (m_lPaperCtrl.GetSafeHwnd())
	{
		m_lPaperCtrl.MoveWindow(nCurrentLeft + nPapersListWidth + nGap, nCurrentTop, nIssueListWdith, nListH);
		nCurrentTop = nCurrentTop + nListH + nGap;
	}

	int nBtnWidth = 70;
	int nBtnHeight = 40;
	int	nTipsHeight = 30;
	if (GetDlgItem(IDC_BTN_Start)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_Start)->MoveWindow(nCurrentLeft, nCurrentTop, nBtnWidth, nBtnHeight);
		nCurrentTop += (nBtnHeight + nGap);
	}
	if (GetDlgItem(IDC_BTN_SAVE_PAPERSINPUTDLG)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_SAVE_PAPERSINPUTDLG)->MoveWindow(nCurrentLeft, nCurrentTop, nBtnWidth, nBtnHeight);
		//		nCurrentTop = nCurrentTop + nBtnHeight + nGap;
	}
	if (GetDlgItem(IDC_STATIC_TIPS)->GetSafeHwnd())
	{
		nCurrentTop = cy - nBottomGap - nTipsHeight;
		GetDlgItem(IDC_STATIC_TIPS)->MoveWindow(nCurrentLeft, nCurrentTop, cx - nCurrentLeft - nRightGap, nTipsHeight);
	}

	//Tab控件的位置
	int nTabLeftPos = nLeftGap + nLeftCtrlWidth + nGap;
	int nTabCtrlHeight = rcClient.Height() * 0.62;
	int nTabCtrlWidth = rcClient.Width() - nLeftGap - nLeftCtrlWidth - nGap - nRightGap;
	if (m_pShowPicDlg && m_pShowPicDlg->GetSafeHwnd())
	{
		m_pShowPicDlg->MoveWindow(nTabLeftPos, nTopGap, nTabCtrlWidth, nTabCtrlHeight);
		nCurrentTop = nTopGap + nTabCtrlHeight + nGap;
	}

	if (m_pAnswerShowDlg && m_pAnswerShowDlg->GetSafeHwnd())
	{
		int nH = cy - nCurrentTop - nBottomGap - nTipsHeight - nGap;
		m_pAnswerShowDlg->MoveWindow(nTabLeftPos, nCurrentTop, nTabCtrlWidth, nH);
		nCurrentTop = nTopGap + nTabCtrlHeight + nGap;
	}

// 	int nInfoStaticWidth = 100;			//试卷袋描述信息static控件的宽度
// 	if (GetDlgItem(IDC_STATIC_PapersModel)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_STATIC_PapersModel)->MoveWindow(nTabLeftPos, nCurrentTop, nInfoStaticWidth, nStaticHeight);
// 		//		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
// 	}
// 	if (GetDlgItem(IDC_EDIT_ModelInfo)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_EDIT_ModelInfo)->MoveWindow(nTabLeftPos + nInfoStaticWidth + nGap, nCurrentTop, nInfoStaticWidth * 1.5, nStaticHeight);
// 		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
// 	}
// 	if (GetDlgItem(IDC_STATIC_PapersName)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_STATIC_PapersName)->MoveWindow(nTabLeftPos, nCurrentTop, nInfoStaticWidth, nStaticHeight);
// 		//		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
// 	}
// 	if (GetDlgItem(IDC_EDIT_PapersName)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_EDIT_PapersName)->MoveWindow(nTabLeftPos + nInfoStaticWidth + nGap, nCurrentTop, nInfoStaticWidth * 1.5, nStaticHeight);
// 		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
// 	}
// 	if (GetDlgItem(IDC_STATIC_PapersDesc)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_STATIC_PapersDesc)->MoveWindow(nTabLeftPos, nCurrentTop, nInfoStaticWidth, nStaticHeight);
// 		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
// 	}
// 
// 	int nPaperDescHeight = rcClient.Height() - nCurrentTop - nGap - nBtnHeight - nGap - nTipsHeight - nBottomGap;		//试卷描述信息控件的高度
// 	if (GetDlgItem(IDC_EDIT_PapersDesc)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_EDIT_PapersDesc)->MoveWindow(nTabLeftPos, nCurrentTop, nTabCtrlWidth, nPaperDescHeight);
// 		nCurrentTop = nCurrentTop + nPaperDescHeight + nGap;
// 	}


// 	if (GetDlgItem(IDC_BTN_Start)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_BTN_Start)->MoveWindow(nTabLeftPos, nCurrentTop, nBtnWidth, nBtnHeight);
// 	}
// 	if (GetDlgItem(IDC_BTN_SAVE_PAPERSINPUTDLG)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_BTN_SAVE_PAPERSINPUTDLG)->MoveWindow(nTabLeftPos + nBtnWidth + nGap, nCurrentTop, nBtnWidth, nBtnHeight);
// 		//		nCurrentTop = nCurrentTop + nBtnHeight + nGap;
// 	}
// 	if (GetDlgItem(IDC_STATIC_TIPS)->GetSafeHwnd())
// 	{
// 		GetDlgItem(IDC_STATIC_TIPS)->MoveWindow(nTabLeftPos, nCurrentTop, nTabCtrlWidth, nTipsHeight);
// 	}
#else
	if (GetDlgItem(IDC_STATIC_PapersTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PapersTips)->MoveWindow(nLeftGap, nCurrentTop, nLeftCtrlWidth, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	int nPapersListWidth = static_cast<int>(nLeftCtrlWidth * 0.4);
	int nIssueListWdith = nLeftCtrlWidth - nPapersListWidth - nGap;
	if (m_lPapersCtrl.GetSafeHwnd())
	{
		m_lPapersCtrl.MoveWindow(nLeftGap, nCurrentTop, nPapersListWidth, rcClient.Height() - nCurrentTop);
	}
	if (GetDlgItem(IDC_STATIC_PaperTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PaperTips)->MoveWindow(nLeftGap + nPapersListWidth + nGap, nCurrentTop - nStaticHeight - nGap, nIssueListWdith, nStaticHeight);		//与试卷袋列表提示齐平
//		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	int nPaperListHeight = (rcClient.Height() - nCurrentTop - nGap - nStaticHeight - nGap) / 2;		//试卷列表控件的高度
	if (m_lPaperCtrl.GetSafeHwnd())
	{
		m_lPaperCtrl.MoveWindow(nLeftGap + nPapersListWidth + nGap, nCurrentTop, nIssueListWdith, nPaperListHeight);
		nCurrentTop = nCurrentTop + nPaperListHeight + nGap;
	}

	//Tab控件的位置
	int nTabLeftPos = nLeftGap + nLeftCtrlWidth + nGap;
	int nTabCtrlHeight = rcClient.Height() * 2 / 3;
	int nTabCtrlWidth = rcClient.Width() - nLeftGap - nLeftCtrlWidth - nGap - nRightGap;
	if (m_pShowPicDlg && m_pShowPicDlg->GetSafeHwnd())
	{
		m_pShowPicDlg->MoveWindow(nTabLeftPos, nTopGap, nTabCtrlWidth, nTabCtrlHeight);
		nCurrentTop = nTopGap + nTabCtrlHeight + nGap;
	}

	int nInfoStaticWidth = 100;			//试卷袋描述信息static控件的宽度
	if (GetDlgItem(IDC_STATIC_PapersModel)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PapersModel)->MoveWindow(nTabLeftPos, nCurrentTop, nInfoStaticWidth, nStaticHeight);
//		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	if (GetDlgItem(IDC_EDIT_ModelInfo)->GetSafeHwnd())
	{
		GetDlgItem(IDC_EDIT_ModelInfo)->MoveWindow(nTabLeftPos + nInfoStaticWidth + nGap, nCurrentTop, nInfoStaticWidth * 1.5, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	if (GetDlgItem(IDC_STATIC_PapersName)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PapersName)->MoveWindow(nTabLeftPos, nCurrentTop, nInfoStaticWidth, nStaticHeight);
//		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	if (GetDlgItem(IDC_EDIT_PapersName)->GetSafeHwnd())
	{
		GetDlgItem(IDC_EDIT_PapersName)->MoveWindow(nTabLeftPos + nInfoStaticWidth + nGap, nCurrentTop, nInfoStaticWidth * 1.5, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}
	if (GetDlgItem(IDC_STATIC_PapersDesc)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_PapersDesc)->MoveWindow(nTabLeftPos, nCurrentTop, nInfoStaticWidth, nStaticHeight);
		nCurrentTop = nCurrentTop + nStaticHeight + nGap;
	}

	int nBtnWidth = 70;
	int nBtnHeight = 40;
	int	nTipsHeight = 30;

	int nPaperDescHeight = rcClient.Height() - nCurrentTop - nGap - nBtnHeight - nGap - nTipsHeight - nBottomGap;		//试卷描述信息控件的高度
	if (GetDlgItem(IDC_EDIT_PapersDesc)->GetSafeHwnd())
	{
		GetDlgItem(IDC_EDIT_PapersDesc)->MoveWindow(nTabLeftPos, nCurrentTop, nTabCtrlWidth, nPaperDescHeight);
		nCurrentTop = nCurrentTop + nPaperDescHeight + nGap;
	}
	
	if (GetDlgItem(IDC_BTN_Start)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_Start)->MoveWindow(nTabLeftPos, nCurrentTop, nBtnWidth, nBtnHeight);
	}
	if (GetDlgItem(IDC_BTN_SAVE_PAPERSINPUTDLG)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_SAVE_PAPERSINPUTDLG)->MoveWindow(nTabLeftPos + nBtnWidth + nGap, nCurrentTop, nBtnWidth, nBtnHeight);
//		nCurrentTop = nCurrentTop + nBtnHeight + nGap;
	}
	if (GetDlgItem(IDC_BTN_Test)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_Test)->MoveWindow(nTabLeftPos + (nBtnWidth + nGap) * 2, nCurrentTop, nBtnWidth, nBtnHeight);
		nCurrentTop = nCurrentTop + nBtnHeight + nGap;
	}
	if (GetDlgItem(IDC_STATIC_TIPS)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_TIPS)->MoveWindow(nTabLeftPos, nCurrentTop, nTabCtrlWidth, nTipsHeight);
	}
#endif
}

void CPaperInputDlg::InitParam()
{
	std::string strLog;
	std::string strFile = g_strCurrentPath + "param.dat";
	std::string strUtf8Path = CMyCodeConvert::Gb2312ToUtf8(strFile);
	try
	{
		Poco::AutoPtr<Poco::Util::IniFileConfiguration> pConf(new Poco::Util::IniFileConfiguration(strUtf8Path));

		g_nRecogGrayMin = pConf->getInt("RecogGray.gray_Min", 0);
		g_nRecogGrayMax_White = pConf->getInt("RecogGray.white_Max", 255);
		g_nRecogGrayMin_OMR = pConf->getInt("RecogGray.omr_Min", 0);
		g_RecogGrayMax_OMR = pConf->getInt("RecogGray.omr_Max", 235);

		_dCompThread_Fix_ = pConf->getDouble("RecogOmrSn_Fix.fCompTread", 1.2);
		_dDiffThread_Fix_ = pConf->getDouble("RecogOmrSn_Fix.fDiffThread", 0.2);
		_dDiffExit_Fix_ = pConf->getDouble("RecogOmrSn_Fix.fDiffExit", 0.3);
		_dCompThread_Head_ = pConf->getDouble("RecogOmrSn_Head.fCompTread", 1.2);
		_dDiffThread_Head_ = pConf->getDouble("RecogOmrSn_Head.fDiffThread", 0.085);
		_dDiffExit_Head_ = pConf->getDouble("RecogOmrSn_Head.fDiffExit", 0.15);

		_nThreshold_Recog2_ = pConf->getInt("RecogOmrSn_Fun2.nThreshold_Fun2", 240);

		_dCompThread_3_ = pConf->getDouble("RecogOmrSn_Fun3.fCompTread", 170);
		_dDiffThread_3_ = pConf->getDouble("RecogOmrSn_Fun3.fDiffThread", 20);
		_dDiffExit_3_ = pConf->getDouble("RecogOmrSn_Fun3.fDiffExit", 50);
		_dAnswerSure_ = pConf->getDouble("RecogOmrSn_Fun3.fAnswerSure", 100);

		strLog = "读取识别灰度参数完成";
	}
	catch (Poco::Exception& exc)
	{
		strLog = "读取参数失败，使用默认参数 " + CMyCodeConvert::Utf8ToGb2312(exc.displayText());
		g_nRecogGrayMin = 0;
		g_nRecogGrayMax_White = 255;
		g_nRecogGrayMin_OMR = 0;
		g_RecogGrayMax_OMR = 235;
	}
	g_pLogger->information(strLog);
}

void CPaperInputDlg::OnBnClickedBtnBroswer()
{
	USES_CONVERSION;

	LPITEMIDLIST pidlRoot = NULL;
	SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlRoot);
	BROWSEINFO	bi;
	TCHAR		szPath[MAX_PATH];
	TCHAR		szFolderPath[MAX_PATH];
	ZeroMemory(&bi, sizeof(LPBROWSEINFO));
	bi.pidlRoot = pidlRoot;
	bi.lpszTitle = _T("选择试卷文件夹路径");
	bi.lParam = NULL;
	bi.pszDisplayName = szPath;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	LPITEMIDLIST lpIDList = SHBrowseForFolder(&bi);
	if (!lpIDList)
	{
		return;
	}
	SHGetPathFromIDList(lpIDList, szFolderPath);

	m_strPapersPath = szFolderPath;
	UpdateData(FALSE);
}

bool SortbyNumASC(const std::string& x, const std::string& y)
{
//	USES_CONVERSION;
	char szX[MAX_PATH] = { 0 };
	char szY[MAX_PATH] = { 0 };
	sprintf_s(szX, "%s", x.c_str()/*T2A(x)*/);
	sprintf_s(szY, "%s", y.c_str()/*T2A(y)*/);
	int nLenX = x.length();	// x.GetLength();
	int nLenY = y.length();	// y.GetLength();

//	TRACE(_T("compare: %s, %s\n"), x, y);

	int nFlag = 0;
	while (nLenX && nLenY)
	{
		char szXPart[MAX_PATH] = { 0 };
		char szYPart[MAX_PATH] = { 0 };
		sscanf(szX, "%[A-Za-z]", szXPart);
		sscanf(szY, "%[A-Za-z]", szYPart);
		if (strlen(szXPart) && strlen(szYPart))
		{
			int nResult = stricmp(szXPart, szYPart);
			if (nResult == 0)
			{
				int nX = strlen(szXPart);
				int nY = strlen(szYPart);
				int nXAll = strlen(szX);
				int nYAll = strlen(szY);
				memmove(szX, szX + strlen(szXPart), nXAll - nX);
				memmove(szY, szY + strlen(szYPart), nYAll - nY);
				szX[nXAll - nX] = '\0';
				szY[nYAll - nY] = '\0';
				nLenX = strlen(szX);
				nLenY = strlen(szY);
			}
			else
			{
				return nResult < 0 ? true : false;
			}
		}
		else if (strlen(szXPart))
			return false;
		else if (strlen(szYPart))
			return true;
		else
		{
			sscanf(szX, "%[0-9]", szXPart);
			sscanf(szY, "%[0-9]", szYPart);
			if (strlen(szXPart) && strlen(szYPart))
			{
				int x = atoi(szXPart);
				int y = atoi(szYPart);
				if (x == y)
				{
					if (strlen(szXPart) == strlen(szYPart))
					{
						int nX = strlen(szXPart);
						int nY = strlen(szYPart);
						int nXAll = strlen(szX);
						int nYAll = strlen(szY);
						memmove(szX, szX + strlen(szXPart), nXAll - nX);
						memmove(szY, szY + strlen(szYPart), nYAll - nY);
						szX[nXAll - nX] = '\0';
						szY[nYAll - nY] = '\0';
						nLenX = strlen(szX);
						nLenY = strlen(szY);
					}
					else
					{
						return strlen(szXPart) > strlen(szYPart);		//大小相同，长度越大越靠前
					}
				}
				else
					return x < y;
			}
			else if (strlen(szXPart))
				return false;
			else if (strlen(szYPart))
				return true;
			else
			{
				sscanf(szX, "%[^0-9A-Za-z]", szXPart);
				sscanf(szY, "%[^0-9A-Za-z]", szYPart);
				int nResult = stricmp(szXPart, szYPart);
				if (nResult == 0)
				{
					int nX = strlen(szXPart);
					int nY = strlen(szYPart);
					int nXAll = strlen(szX);
					int nYAll = strlen(szY);
					memmove(szX, szX + strlen(szXPart), nXAll - nX);
					memmove(szY, szY + strlen(szYPart), nYAll - nY);
					szX[nXAll - nX] = '\0';
					szY[nYAll - nY] = '\0';
					nLenX = strlen(szX);
					nLenY = strlen(szY);
				}
				else
				{
					char* p1 = szXPart;
					char* p2 = szYPart;
					while (*p1 != '\0' && *p2 != '\0')
					{
						if (*p1 == '-'&& *p2 != '-')
							return false;
						else if (*p1 != '-' && *p2 == '-')
							return true;
						else if (*p1 == '=' && *p2 != '=')
							return false;
						else if (*p1 != '=' && *p2 == '=')
							return true;
						else if (*p1 == '+' && *p2 != '+')
							return false;
						else if (*p1 != '+' && *p2 == '+')
							return true;
						else if (*p1 > *p2)
							return false;
						else if (*p1 < *p2)
							return true;
						else
						{
							p1++;
							p2++;
						}
					}
					if (*p1 == '\0' && *p2 != '\0')
					{
						if (*p2 == ' ')
							return false;
						else
							return true;
					}
					else if (*p1 != '\0' && *p2 == '\0')
					{
						if (*p1 == ' ')
							return true;
						else
							return false;
					}
					//return nResult < 0?true:false;
				}
			}
		}
	}

	return x.length() < y.length();
}
void CPaperInputDlg::OnBnClickedBtnStart()
{
	UpdateData(TRUE);
	if (m_strPapersPath == _T(""))
	{
		CNewMessageBox	dlg;
		dlg.setShowInfo(2, 1, "请设置试卷袋路径！");
		dlg.DoModal();
		return;
	}
	if (_pCurrExam_->nModel == 0 && !m_pModel)
	{
		CNewMessageBox	dlg;
		dlg.setShowInfo(2, 1, "未设置检测的模板信息！");
		dlg.DoModal();
		return;
	}

	InitParam();

	int nModelPicNums = 0;
	if (m_pModel)
		nModelPicNums = m_pModel->nPicNum;
	else
		nModelPicNums = _nPicNum4Ty_;

	USES_CONVERSION;
	std::string strPaperPath = CMyCodeConvert::Gb2312ToUtf8(T2A(m_strPapersPath));
	try
	{
		Poco::DirectoryIterator it(strPaperPath);
		Poco::DirectoryIterator end;
		while (it != end)
		{
			Poco::Path p(it->path());
			if (it->isDirectory())
			{
				std::wstring strWInput;
				Poco::UnicodeConverter::toUTF16(p.getFileName(), strWInput);
				char szDirName[90] = { 0 };
				sprintf_s(szDirName, "%s", T2A(strWInput.c_str()));
				TRACE("%s\n", szDirName);

				std::vector<std::string> lFileName;
				std::string strSubDirPath = p.toString();
				Poco::DirectoryIterator itSub(strSubDirPath);
				Poco::DirectoryIterator endSub;
				while (itSub != endSub)
				{
					Poco::Path pSubFile(itSub->path());
					if (itSub->isFile())
					{
						std::string strOldFileName = pSubFile.getFileName();

						if (strOldFileName.find("papersInfo.dat") == std::string::npos)
						{
							lFileName.push_back(strOldFileName);
						}
					}
					itSub++;
				}

				if (lFileName.size() % nModelPicNums != 0)
				{
					char szErrorInfo[MAX_PATH] = { 0 };
					sprintf_s(szErrorInfo, "扫描到文件夹%s试卷数量%d,模板要求每考生%d张试卷, 请检查是否有考生试卷缺失", szDirName, lFileName.size(), m_pModel->nPicNum);
					//AfxMessageBox(A2T(szErrorInfo));

					CNewMessageBox	dlg;
					dlg.setShowInfo(2, 1, szErrorInfo);
					dlg.DoModal();
					it++;
					continue;
				}

				//插入试卷袋列表
				char szPapersName[100] = { 0 };
				sprintf_s(szPapersName, "%s", szDirName);
				int nPapersCount = m_lPapersCtrl.GetItemCount();
				m_lPapersCtrl.InsertItem(nPapersCount, NULL);
				m_lPapersCtrl.SetItemText(nPapersCount, 0, (LPCTSTR)A2T(szPapersName));

				//文件复制本地临时目录
				std::string strSubPaperPath = g_strPaperSavePath + p.getFileName();
				char szSubPaperPath[MAX_PATH] = { 0 };
				sprintf_s(szSubPaperPath, "%s%s", g_strPaperSavePath.c_str(), szDirName);
				Poco::File tmpPath(strSubPaperPath);
				tmpPath.createDirectories();

				pPAPERSINFO pPapers = new PAPERSINFO;
				g_fmPapers.lock();
				g_lPapers.push_back(pPapers);
				g_fmPapers.unlock();

				pPapers->nPaperCount = lFileName.size() / nModelPicNums;
				pPapers->strPapersName = szDirName;

				int i = 0;
				pST_PaperInfo pPaper = NULL;
				std::sort(lFileName.begin(), lFileName.end(), SortbyNumASC);
				std::vector<std::string>::iterator itName = lFileName.begin();
				for (; itName != lFileName.end(); itName++)
				{
					TRACE("%s\n", (*itName).c_str());	//(*itName).c_str()

					char szNewName[100] = { 0 };
					sprintf_s(szNewName, "S%d_%s", i / nModelPicNums + 1, (*itName).c_str());

					std::string strNewName = szNewName;
					std::string strNewFilePath = strSubPaperPath + "\\" + strNewName;

					std::string strFileOldPath = strSubDirPath + "\\" + *itName;
					Poco::File oldFile(strFileOldPath);
					oldFile.copyTo(strNewFilePath);

					if (i % nModelPicNums == 0)
					{
						pPaper = new ST_PaperInfo;
						pPapers->lPaper.push_back(pPaper);
						char szStudentInfo[20] = { 0 };
						sprintf_s(szStudentInfo, "S%d", i / nModelPicNums + 1);
						pPaper->strStudentInfo = szStudentInfo;
						pPaper->pModel = m_pModel;
						pPaper->pPapers = pPapers;
						pPaper->pSrcDlg = this;
						pPaper->nIndex = i / nModelPicNums + 1;
					}
					pST_PicInfo pPic = new ST_PicInfo;
					pPaper->lPic.push_back(pPic);

					char szNewFullPath[MAX_PATH] = { 0 };
					sprintf_s(szNewFullPath, "%s\\%s", szSubPaperPath, strNewName.c_str());
					pPic->strPicName = strNewName;
					pPic->strPicPath = CMyCodeConvert::Utf8ToGb2312(strNewFilePath);	// strNewFilePath;
					pPic->pPaper = pPaper;
					i++;

					if (m_pModel)
					{
					#if 1	//判断并调整方向
						static int j = 0;
						if ((i - 1) % nModelPicNums == 0)
							j = 0;
						Mat mtPic = imread(CMyCodeConvert::Utf8ToGb2312(strNewFilePath));
						bool bDoubleScan = m_pModel->vecPaperModel.size() % 2 == 0 ? true : false;
						if (m_pModel->nUsePagination || m_pModel->vecPaperModel.size() > 2)		//对于多页模式，必须是双面扫描，防止掉试卷，即防止空白页替换真试卷
							bDoubleScan = true;
						//COmrRecog omrObj;
						_chkRotationObj.GetRightPicOrientation(mtPic, j, bDoubleScan);

						imwrite(pPic->strPicPath, mtPic);
						j++;
					#endif
					}
				}

				//更新试卷袋数量
				char szPapersCount[20] = { 0 };
				sprintf_s(szPapersCount, "%d", pPapers->lPaper.size());
				m_lPapersCtrl.SetItemText(nPapersCount, 1, (LPCTSTR)A2T(szPapersCount));
				m_lPapersCtrl.SetItemData(nPapersCount, (DWORD_PTR)pPapers);

			#ifdef TEST_PAGINATION
				//判断并调整正反面
				if (m_pModel)
				{
					PAPER_LIST::iterator itPaper = pPapers->lPaper.begin();
					for (; itPaper != pPapers->lPaper.end(); itPaper++)
					{
						pST_PaperInfo pPaper = *itPaper;
						ChkAdjustFirstPic(pPaper);
					}
				}
			#endif
				if (_pCurrExam_->nModel == 0)
				{
					//添加到识别任务列表
					PAPER_LIST::iterator itPaper = pPapers->lPaper.begin();
					for (; itPaper != pPapers->lPaper.end(); itPaper++)
					{
						pRECOGTASK pTask = new RECOGTASK;
						pTask->pPaper = *itPaper;
						g_lRecogTask.push_back(pTask);
					}
				}
			}
			it++;
		}
	}
	catch (Poco::Exception& exc)
	{
		std::string strErr = "获取文件异常，请检查设置信息是否正确！";
		CNewMessageBox dlg;
		dlg.setShowInfo(2, 1, strErr);
		dlg.DoModal();

		strErr.append(exc.message());
		g_pLogger->information(strErr);
	}	
}

bool sortModelFile2(ST_MODELFILE& st1, ST_MODELFILE& st2)
{
	if (st1.strModifyTime > st2.strModifyTime)	return true;
	else if (st1.strModifyTime == st2.strModifyTime)	return (st1.strModelName >= st2.strModelName ? true : false);
	else return false;
}

void CPaperInputDlg::SeachModel()
{
	USES_CONVERSION;
	std::string strModelPath = T2A(g_strCurrentPath + _T("Model"));

	std::string strLog;
	try
	{
		std::list<ST_MODELFILE> lModelFile;
		std::string strUtf8Path = CMyCodeConvert::Gb2312ToUtf8(strModelPath);
		Poco::DirectoryIterator it(strUtf8Path);
		Poco::DirectoryIterator end;
		while (it != end)
		{
			Poco::Path p(it->path());
			if (it->isFile() && p.getExtension() == "mod")
			{
				std::string strModelName = CMyCodeConvert::Utf8ToGb2312(p.getBaseName());

				Poco::DateTime dt(it->getLastModified());
				std::string strLastModifyTime = Poco::format("%04d-%02d-%02d", dt.year(), dt.month(), dt.day());

				ST_MODELFILE stModelFile;
				stModelFile.strModelName = strModelName;
				stModelFile.strModifyTime = strLastModifyTime;
				lModelFile.push_back(stModelFile);
			}
			it++;
		}
		lModelFile.sort(sortModelFile2);

		for(auto modelFile : lModelFile)
		{
			std::string strModelName = modelFile.strModelName;
			m_comboModel.AddString(A2T(strModelName.c_str()));
		}
		strLog = "搜索模板完成";
	}
	catch (Poco::FileException& exc)
	{
		strLog = "搜索模板失败: " + exc.displayText();
	}
	catch (Poco::Exception& exc)
	{
		strLog = "搜索模板失败2: " + exc.displayText();
	}
	g_pLogger->information(strLog);
}

void CPaperInputDlg::SetListCtrlHighLightShow(CXListCtrl& lCtrl, int nItem)
{
	if (nItem < 0) return;

	lCtrl.GetItemColors(nItem, 0, crOldText, crOldBackground);
	for (int i = 0; i < lCtrl.GetColumns(); i++)							//设置高亮显示(手动设置背景颜色)
		lCtrl.SetItemColors(nItem, i, RGB(0, 0, 0), RGB(112, 180, 254));	//70, 70, 255
	lCtrl.Invalidate();
}

void CPaperInputDlg::UnSetListCtrlHighLightShow(CXListCtrl& lCtrl, int nItem)
{
	if (nItem < 0) return;

	for (int i = 0; i < lCtrl.GetColumns(); i++)
		if (!lCtrl.GetModified(nItem, i))
			lCtrl.SetItemColors(nItem, i, crOldText, crOldBackground);
		else
			lCtrl.SetItemColors(nItem, i, RGB(255, 0, 0), crOldBackground);
}

void CPaperInputDlg::OnCbnSelchangeComboModellist()
{
	if (m_ncomboCurrentSel == m_comboModel.GetCurSel())
		return;

	CString strModelName;
	m_comboModel.GetLBText(m_comboModel.GetCurSel(), strModelName);
	CString strModelPath = g_strCurrentPath + _T("Model\\") + strModelName;
	CString strModelFullPath = strModelPath + _T(".mod");
//	UnZipFile(strModelFullPath);		//UnZipModel(strModelFullPath);
	CZipObj zipObj;
	zipObj.setLogger(g_pLogger);
	zipObj.UnZipFile(strModelFullPath);
	if (m_pModel && m_pModel != m_pOldModel)
	{
		delete m_pModel;
		m_pModel = NULL;
	}
	m_pModel = LoadModelFile(strModelPath);
	m_ncomboCurrentSel = m_comboModel.GetCurSel();

	m_nModelPicNums = m_pModel->nPicNum;

//	m_pAnswerShowDlg->InitModel(m_pModel);
}

void CPaperInputDlg::OnNMDblclkListPapers(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	if (pNMItemActivate->iItem < 0) return;

	m_lPaperCtrl.DeleteAllItems();
//	m_lIssuePaperCtrl.DeleteAllItems();

	USES_CONVERSION;
	pPAPERSINFO pPapers = (pPAPERSINFO)m_lPapersCtrl.GetItemData(pNMItemActivate->iItem);
	m_pCurrentPapers = pPapers;

	m_nCurrItemPapers = pNMItemActivate->iItem;
	m_strPapersName = A2T(pPapers->strPapersName.c_str());
	m_strPapersDesc = A2T(pPapers->strPapersDesc.c_str());

	ShowPapers(pPapers);

	InitTmpSubjectBmk();

	CScanTool3Dlg* pDlg = (CScanTool3Dlg*)GetParent();
	if (g_nOperatingMode == 1 || g_bModifySN)
	{
		KillTimer(TIMER_CheckRecogComplete);

		USES_CONVERSION;
		int nCount = m_lPaperCtrl.GetItemCount();
		for (int i = 0; i < nCount; i++)
		{
			pST_PaperInfo pItemPaper = (pST_PaperInfo)(DWORD_PTR)m_lPaperCtrl.GetItemData(i);
			if (pItemPaper)
			{
				if (!pItemPaper->strSN.empty())
					m_lPaperCtrl.SetItemText(i, 1, (LPCTSTR)A2T(pItemPaper->strSN.c_str()));
				else
				{
					m_lPaperCtrl.SetItemText(i, 1, _T("未识别"));
				}
			}
		}

		if (_nScanAnswerModel_ == 0)		//扫描主观题、客观题答案时不进行准考证号异常处理操作
			SetTimer(TIMER_CheckRecogComplete, 100, NULL);
	}

	UpdateData(FALSE);
}


void CPaperInputDlg::OnNMDblclkListPaper(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	if (pNMItemActivate->iItem < 0)
		return;

	if(m_nCurrItemPaperList < m_lPaperCtrl.GetItemCount())
		UnSetListCtrlHighLightShow(m_lPaperCtrl, m_nCurrItemPaperList);

	pST_PaperInfo pPaper = (pST_PaperInfo)m_lPaperCtrl.GetItemData(pNMItemActivate->iItem);
	m_pCurrentShowPaper = pPaper;
	m_nCurrItemPaperList = pNMItemActivate->iItem;

	SetListCtrlHighLightShow(m_lPaperCtrl, m_nCurrItemPaperList);

// 	PaintRecognisedRect(pPaper);
// 
// 	m_nCurrTabSel = 0;
// 
// 	m_tabPicShow.SetCurSel(0);
// 	m_pCurrentPicShow = m_vecPicShow[0];
// 	m_pCurrentPicShow->ShowWindow(SW_SHOW);
// 	for (int i = 0; i < m_vecPicShow.size(); i++)
// 	{
// 		if (i != 0)
// 			m_vecPicShow[i]->ShowWindow(SW_HIDE);
// 	}

	m_pShowPicDlg->setShowPaper(pPaper);
	m_pAnswerShowDlg->InitData(pPaper);

#ifdef Test_Data
	return;
#endif

	//双击为空的准考证号时显示准考证号修改窗口
	CScanTool3Dlg* pDlg = (CScanTool3Dlg*)GetParent();
	if ((/*g_nOperatingMode == 1 *//*|| pDlg->m_bModifySN*/g_bModifySN) && m_pModel && pPaper && \
		(pPaper->strSN.empty() || pPaper->bModifyZKZH || pPaper->bReScan || (_bGetBmk_ && pPaper->nZkzhInBmkStatus != 1) || pPaper->nPicsExchange != 0))
	{
		if (!m_pStudentMgr)
		{
			USES_CONVERSION;
			m_pStudentMgr = new CStudentMgr();
			std::string strDbPath = T2A(g_strCurrentPath + _T("bmk.db"));
			bool bResult = m_pStudentMgr->InitDB(CMyCodeConvert::Gb2312ToUtf8(strDbPath));
		}

	#ifdef TEST_MODIFY_ZKZH_CHIld
		CScanTool3Dlg* pDlg = (CScanTool3Dlg*)AfxGetMainWnd();
		pDlg->SwitchModifyZkzkDlg(m_pModel, m_pCurrentPapers, m_pStudentMgr);
	#else
		CModifyZkzhDlg zkzhDlg(_pModel_, m_pCurrentPapers, m_pStudentMgr);
		zkzhDlg.DoModal();
		ShowPapers(m_pCurrentPapers);
	#endif
	}
}

void CPaperInputDlg::OnBnClickedBtnSave()
{
	UpdateData(TRUE);
	if (m_nCurrItemPapers < 0)
	{
		CNewMessageBox	dlg;
		dlg.setShowInfo(2, 1, "请先选中试卷袋");
		dlg.DoModal();
		return;
	}

	USES_CONVERSION;
	pPAPERSINFO pPapers = (pPAPERSINFO)m_lPapersCtrl.GetItemData(m_nCurrItemPapers);
	if (!pPapers)
	{
		CNewMessageBox	dlg;
		dlg.setShowInfo(2, 1, "没有试卷袋信息");
		dlg.DoModal();
		return;
	}

// 	if (pPapers->lIssue.size() > 0)
// 	{
// 		if (g_nOperatingMode == 2)
// 		{
// 			CNewMessageBox	dlg;
// 			dlg.setShowInfo(2, 1, "存在识别异常试卷，不能上传，请先处理异常试卷");
// 			dlg.DoModal();
// 			return;
// 		}
// 		else
// 		{
// 			CString strMsg = _T("");
// 			strMsg.Format(_T("存在%d份问题试卷，这些试卷需要单独找出扫描，是否确定上传?"), pPapers->lIssue.size());
// 
// 			CNewMessageBox	dlg;
// 			dlg.setShowInfo(2, 2, T2A(strMsg));
// 			dlg.DoModal();
// 			if (dlg.m_nResult != IDYES)
// 				return;
// 			pPapers->nPaperCount = pPapers->lPaper.size();		//修改扫描数量，将问题试卷删除，不算到扫描试卷中。
// 		}
// 	}

	pPapers->strPapersDesc = T2A(m_strPapersDesc);
	pPapers->strPapersName = T2A(m_strPapersName);

	m_lPapersCtrl.SetItemText(m_nCurrItemPapers, 0, (LPCTSTR)A2T(pPapers->strPapersName.c_str()));

#if 1
	CPapersMgr papersMgr;
	char szPapersSavePath[MAX_PATH] = { 0 };
	sprintf_s(szPapersSavePath, "%sPaper\\%s", T2A(g_strCurrentPath), T2A(m_strPapersName));
	papersMgr.setCurrSavePath(szPapersSavePath);
	papersMgr.setExamInfo(_pCurrExam_, m_pModel);
	bool bResult = papersMgr.SavePapers(pPapers);
	if (!bResult)
		return;

	//---------------------------------
	//进入试卷袋压缩后，此试卷袋会自动释放
	g_fmPapers.lock();
	PAPERS_LIST::iterator it = g_lPapers.begin();
	for (; it != g_lPapers.end();)
	{
		if (*it == pPapers)
			it = g_lPapers.erase(it);
		else
			it++;
	}
	g_fmPapers.unlock();
	//---------------------------------

	std::string strZipName = papersMgr.AddPapersCompress(pPapers);

	CString strInfo;
	bool bWarn = false;
	strInfo.Format(_T("正在保存%s..."), A2T(strZipName.c_str()));
	SetStatusShowInfo(strInfo);

	//防止重复保存
	m_lPapersCtrl.SetItemData(m_nCurrItemPapers, NULL);
#else
	clock_t start, end;
	start = clock();

	BOOL	bLogin = FALSE;
	CString strUser = _T("");
	CString strEzs = _T("");
	int nTeacherId = -1;
	int nUserId = -1;
	
	strEzs = pDlg->m_strEzs;
	strUser = pDlg->m_strUserName;
	nTeacherId = pDlg->m_nTeacherId;
	nUserId = pDlg->m_nUserId;
	bLogin = pDlg->m_bLogin;
	

	CPapersInfoSaveDlg dlg(pPapers, m_pModel);
	if (dlg.DoModal() != IDOK)
		return;

	Poco::JSON::Array jsnPaperArry;
	PAPER_LIST::iterator itNomarlPaper = pPapers->lPaper.begin();
	for (int i = 0; itNomarlPaper != pPapers->lPaper.end(); itNomarlPaper++, i++)
	{
		Poco::JSON::Object jsnPaper;
		jsnPaper.set("name", (*itNomarlPaper)->strStudentInfo);
		jsnPaper.set("zkzh", (*itNomarlPaper)->strSN);
		jsnPaper.set("qk", (*itNomarlPaper)->nQKFlag);

		int nIssueFlag = 0;			//0 - 正常试卷，完全机器识别正常的，无人工干预，1 - 正常试卷，扫描员手动修改过，2-准考证号为空，扫描员没有修改，3-扫描员标识了需要重扫的试卷。
		if ((*itNomarlPaper)->strSN.empty() && !(*itNomarlPaper)->bModifyZKZH)
			nIssueFlag = 2;
		if ((*itNomarlPaper)->bModifyZKZH)
			nIssueFlag = 1;
		jsnPaper.set("issueFlag", nIssueFlag);
		//++在上传服务器时无用，只在从Pkg恢复Papers时有用
		jsnPaper.set("modify", (*itNomarlPaper)->bModifyZKZH);	//准考证号修改标识
		jsnPaper.set("reScan", (*itNomarlPaper)->bReScan);		//重扫标识
		jsnPaper.set("IssueList", 0);		//标识此考生属于问题列表，在上传服务器时无用，只在从Pkg恢复Papers时有用
		//--

		Poco::JSON::Array jsnSnDetailArry;
		SNLIST::iterator itSn = (*itNomarlPaper)->lSnResult.begin();
		for (; itSn != (*itNomarlPaper)->lSnResult.end(); itSn++)
		{
			Poco::JSON::Object jsnSnItem;
			jsnSnItem.set("sn", (*itSn)->nItem);
			jsnSnItem.set("val", (*itSn)->nRecogVal);

			Poco::JSON::Object jsnSnPosition;
			RECTLIST::iterator itRect = (*itSn)->lSN.begin();
			for (; itRect != (*itSn)->lSN.end(); itRect++)
			{
					jsnSnPosition.set("x", itRect->rt.x);
					jsnSnPosition.set("y", itRect->rt.y);
					jsnSnPosition.set("w", itRect->rt.width);
					jsnSnPosition.set("h", itRect->rt.height);
			}
			jsnSnItem.set("position", jsnSnPosition);
			jsnSnDetailArry.add(jsnSnItem);
		}
		jsnPaper.set("snDetail", jsnSnDetailArry);

		Poco::JSON::Array jsnOmrArry;
		OMRRESULTLIST::iterator itOmr = (*itNomarlPaper)->lOmrResult.begin();
		for (; itOmr != (*itNomarlPaper)->lOmrResult.end(); itOmr++)
		{
			Poco::JSON::Object jsnOmr;
			jsnOmr.set("th", itOmr->nTH);
			jsnOmr.set("type", itOmr->nSingle + 1);
			jsnOmr.set("value", itOmr->strRecogVal);
			jsnOmr.set("value1", itOmr->strRecogVal1);
			jsnOmr.set("value2", itOmr->strRecogVal2);
			jsnOmr.set("value3", itOmr->strRecogVal3);
			jsnOmr.set("doubt", itOmr->nDoubt);
			Poco::JSON::Array jsnPositionArry;
			RECTLIST::iterator itRect = itOmr->lSelAnswer.begin();
			for (; itRect != itOmr->lSelAnswer.end(); itRect++)
			{
					Poco::JSON::Object jsnItem;
					char szVal[5] = { 0 };
					sprintf_s(szVal, "%c", itRect->nAnswer + 65);
					jsnItem.set("val", szVal);
					jsnItem.set("x", itRect->rt.x);
					jsnItem.set("y", itRect->rt.y);
					jsnItem.set("w", itRect->rt.width);
					jsnItem.set("h", itRect->rt.height);
					jsnPositionArry.add(jsnItem);
			}
			jsnOmr.set("position", jsnPositionArry);
			jsnOmrArry.add(jsnOmr);
		}
		jsnPaper.set("omr", jsnOmrArry);

		Poco::JSON::Array jsnElectOmrArry;
		ELECTOMR_LIST::iterator itElectOmr = (*itNomarlPaper)->lElectOmrResult.begin();
		for (; itElectOmr != (*itNomarlPaper)->lElectOmrResult.end(); itElectOmr++)
		{
			Poco::JSON::Object jsnElectOmr;
			jsnElectOmr.set("paperId", i + 1);
			jsnElectOmr.set("doubt", itElectOmr->nDoubt);
			jsnElectOmr.set("th", itElectOmr->sElectOmrGroupInfo.nGroupID);
			jsnElectOmr.set("allItems", itElectOmr->sElectOmrGroupInfo.nAllCount);
			jsnElectOmr.set("realItem", itElectOmr->sElectOmrGroupInfo.nRealCount);
			jsnElectOmr.set("value", itElectOmr->strRecogResult);
			Poco::JSON::Array jsnPositionArry;
			RECTLIST::iterator itRect = itElectOmr->lItemInfo.begin();
			for (; itRect != itElectOmr->lItemInfo.end(); itRect++)
			{
				Poco::JSON::Object jsnItem;
				char szVal[5] = { 0 };
				sprintf_s(szVal, "%c", itRect->nAnswer + 65);
				jsnItem.set("val", szVal);
				jsnItem.set("x", itRect->rt.x);
				jsnItem.set("y", itRect->rt.y);
				jsnItem.set("w", itRect->rt.width);
				jsnItem.set("h", itRect->rt.height);
				jsnPositionArry.add(jsnItem);
			}
			jsnElectOmr.set("position", jsnPositionArry);
			jsnElectOmrArry.add(jsnElectOmr);
		}
#if 1
		if (jsnElectOmrArry.size() > 0)
			jsnPaper.set("electOmr", jsnElectOmrArry);		//选做题结果
#else
		jsnPaper.set("electOmr", jsnElectOmrArry);		//选做题结果
#endif
		jsnPaperArry.add(jsnPaper);
	}

	if (g_nOperatingMode == 1)		//简单模式时，异常试卷也一起上传，做特殊标识
	{
		PAPER_LIST::iterator itIssuePaper = pPapers->lIssue.begin();
		for (int j = 0; itIssuePaper != pPapers->lIssue.end(); itIssuePaper++, j++)
		{
			Poco::JSON::Object jsnPaper;
			jsnPaper.set("name", (*itIssuePaper)->strStudentInfo);
			jsnPaper.set("zkzh", (*itIssuePaper)->strSN);
			jsnPaper.set("qk", (*itIssuePaper)->nQKFlag);

			int nIssueFlag = 0;			//0 - 正常试卷，完全机器识别正常的，无人工干预，1 - 正常试卷，扫描员手动修改过，2-准考证号为空，扫描员没有修改，3-扫描员标识了需要重扫的试卷。
			if ((*itIssuePaper)->strSN.empty())
				nIssueFlag = 2;
			if ((*itIssuePaper)->bReScan)		//设置重扫权限更大，放后面设置
				nIssueFlag = 3;
			jsnPaper.set("issueFlag", nIssueFlag);
			//++在上传服务器时无用，只在从Pkg恢复Papers时有用
			jsnPaper.set("modify", (*itIssuePaper)->bModifyZKZH);	//准考证号修改标识
			jsnPaper.set("reScan", (*itIssuePaper)->bReScan);		//重扫标识
			jsnPaper.set("IssueList", 1);		//标识此考生属于问题列表，在上传服务器时无用，只在从Pkg恢复Papers时有用
			//--

			Poco::JSON::Array jsnSnDetailArry;
			SNLIST::iterator itSn = (*itIssuePaper)->lSnResult.begin();
			for (; itSn != (*itIssuePaper)->lSnResult.end(); itSn++)
			{
				Poco::JSON::Object jsnSnItem;
				jsnSnItem.set("sn", (*itSn)->nItem);
				jsnSnItem.set("val", (*itSn)->nRecogVal);

				Poco::JSON::Object jsnSnPosition;
				RECTLIST::iterator itRect = (*itSn)->lSN.begin();
				for (; itRect != (*itSn)->lSN.end(); itRect++)
				{
					jsnSnPosition.set("x", itRect->rt.x);
					jsnSnPosition.set("y", itRect->rt.y);
					jsnSnPosition.set("w", itRect->rt.width);
					jsnSnPosition.set("h", itRect->rt.height);
				}
				jsnSnItem.set("position", jsnSnPosition);
				jsnSnDetailArry.add(jsnSnItem);
			}
			jsnPaper.set("snDetail", jsnSnDetailArry);

			Poco::JSON::Array jsnOmrArry;
			OMRRESULTLIST::iterator itOmr = (*itIssuePaper)->lOmrResult.begin();
			for (; itOmr != (*itIssuePaper)->lOmrResult.end(); itOmr++)
			{
				Poco::JSON::Object jsnOmr;
				jsnOmr.set("th", itOmr->nTH);
				jsnOmr.set("type", itOmr->nSingle + 1);
				jsnOmr.set("value", itOmr->strRecogVal);
				jsnOmr.set("value1", itOmr->strRecogVal1);
				jsnOmr.set("value2", itOmr->strRecogVal2);
				jsnOmr.set("value3", itOmr->strRecogVal3);
				jsnOmr.set("doubt", itOmr->nDoubt);
				Poco::JSON::Array jsnPositionArry;
				RECTLIST::iterator itRect = itOmr->lSelAnswer.begin();
				for (; itRect != itOmr->lSelAnswer.end(); itRect++)
				{
					Poco::JSON::Object jsnItem;
					char szVal[5] = { 0 };
					sprintf_s(szVal, "%c", itRect->nAnswer + 65);
					jsnItem.set("val", szVal);
					jsnItem.set("x", itRect->rt.x);
					jsnItem.set("y", itRect->rt.y);
					jsnItem.set("w", itRect->rt.width);
					jsnItem.set("h", itRect->rt.height);
					jsnPositionArry.add(jsnItem);
				}
				jsnOmr.set("position", jsnPositionArry);
				jsnOmrArry.add(jsnOmr);
			}
			jsnPaper.set("omr", jsnOmrArry);

			Poco::JSON::Array jsnElectOmrArry;
			ELECTOMR_LIST::iterator itElectOmr = (*itIssuePaper)->lElectOmrResult.begin();
			for (; itElectOmr != (*itIssuePaper)->lElectOmrResult.end(); itElectOmr++)
			{
				Poco::JSON::Object jsnElectOmr;
				jsnElectOmr.set("paperId", j + 1);
				jsnElectOmr.set("doubt", itElectOmr->nDoubt);
				jsnElectOmr.set("th", itElectOmr->sElectOmrGroupInfo.nGroupID);
				jsnElectOmr.set("allItems", itElectOmr->sElectOmrGroupInfo.nAllCount);
				jsnElectOmr.set("realItem", itElectOmr->sElectOmrGroupInfo.nRealCount);
				jsnElectOmr.set("value", itElectOmr->strRecogResult);
				Poco::JSON::Array jsnPositionArry;
				RECTLIST::iterator itRect = itElectOmr->lItemInfo.begin();
				for (; itRect != itElectOmr->lItemInfo.end(); itRect++)
				{
					Poco::JSON::Object jsnItem;
					char szVal[5] = { 0 };
					sprintf_s(szVal, "%c", itRect->nAnswer + 65);
					jsnItem.set("val", szVal);
					jsnItem.set("x", itRect->rt.x);
					jsnItem.set("y", itRect->rt.y);
					jsnItem.set("w", itRect->rt.width);
					jsnItem.set("h", itRect->rt.height);
					jsnPositionArry.add(jsnItem);
				}
				jsnElectOmr.set("position", jsnPositionArry);
				jsnElectOmrArry.add(jsnElectOmr);
			}
			jsnPaper.set("electOmr", jsnElectOmrArry);		//选做题结果
			jsnPaperArry.add(jsnPaper);
		}
	}

	//写试卷袋信息到文件
	std::string strUploader = CMyCodeConvert::Gb2312ToUtf8(T2A(strUser));
	std::string sEzs = T2A(strEzs);
	Poco::JSON::Object jsnFileData;
	jsnFileData.set("examId", dlg.m_nExamID);
	jsnFileData.set("subjectId", dlg.m_SubjectID);
	jsnFileData.set("uploader", strUploader);
	jsnFileData.set("ezs", sEzs);
	jsnFileData.set("nTeacherId", nTeacherId);
	jsnFileData.set("nUserId", nUserId);
	jsnFileData.set("scanNum", pPapers->nPaperCount);		//扫描的学生数量
	jsnFileData.set("detail", jsnPaperArry);
	jsnFileData.set("desc", CMyCodeConvert::Gb2312ToUtf8(pPapers->strPapersDesc));

	jsnFileData.set("nOmrDoubt", pPapers->nOmrDoubt);
	jsnFileData.set("nOmrNull", pPapers->nOmrNull);
	jsnFileData.set("nSnNull", pPapers->nSnNull);
	jsnFileData.set("RecogMode", g_nOperatingMode);			//识别模式，1-简单模式(遇到问题校验点不停止识别)，2-严格模式
	std::stringstream jsnString;
	jsnFileData.stringify(jsnString, 0);


	std::string strFileData;
#ifdef USES_FILE_ENC
	if(!encString(jsnString.str(), strFileData))
		strFileData = jsnString.str();
#else
	strFileData = jsnString.str();
#endif

	char szExamInfoPath[MAX_PATH] = { 0 }; 
//	sprintf_s(szExamInfoPath, "%s\\%s\\papersInfo.dat", T2A(m_strPapersPath), pPapers->strPapersName.c_str());
	sprintf_s(szExamInfoPath, "%sPaper\\%s\\papersInfo.dat", T2A(g_strCurrentPath), T2A(m_strPapersName));
	ofstream out(szExamInfoPath);
	out << strFileData.c_str();
	out.close();
	//

	//试卷袋压缩
	char szPapersSrcPath[MAX_PATH] = { 0 };
	char szPapersSavePath[MAX_PATH] = { 0 };
	char szZipName[100] = { 0 };
	char szZipBaseName[90] = { 0 };
	if (bLogin)
	{
		Poco::LocalDateTime now;
		char szTime[50] = { 0 };
		sprintf_s(szTime, "%d%02d%02d%02d%02d%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

//		sprintf_s(szPapersSrcPath, "%s\\%s", T2A(m_strPapersPath), pPapers->strPapersName.c_str());
		sprintf_s(szPapersSrcPath, "%sPaper\\%s", T2A(g_strCurrentPath), T2A(m_strPapersName));

		sprintf_s(szPapersSavePath, "%sPaper\\%s_%d-%d_%s_%d", T2A(g_strCurrentPath), T2A(strUser), dlg.m_nExamID, dlg.m_SubjectID, szTime, pPapers->nPaperCount);
		sprintf_s(szZipBaseName, "%s_%d-%d_%s_%d", T2A(strUser), dlg.m_nExamID, dlg.m_SubjectID, szTime, pPapers->nPaperCount);
		sprintf_s(szZipName, "%s_%d-%d_%s_%d%s", T2A(strUser), dlg.m_nExamID, dlg.m_SubjectID, szTime, pPapers->nPaperCount, T2A(PAPERS_EXT_NAME));
// 		sprintf_s(szPapersSavePath, "%sPaper\\%s_%d-%d_%s", T2A(g_strCurrentPath), T2A(strUser), dlg.m_nExamID, dlg.m_SubjectID, szTime);
// 		sprintf_s(szZipBaseName, "%s_%d-%d_%s", T2A(strUser), dlg.m_nExamID, dlg.m_SubjectID, szTime);
// 		sprintf_s(szZipName, "%s_%d-%d_%s%s", T2A(strUser), dlg.m_nExamID, dlg.m_SubjectID, szTime, T2A(PAPERS_EXT_NAME));
	}
	else
	{
//		sprintf_s(szPapersSrcPath, "%s\\%s", T2A(m_strPapersPath), pPapers->strPapersName.c_str());
		sprintf_s(szPapersSrcPath, "%sPaper\\%s", T2A(g_strCurrentPath), T2A(m_strPapersName));
		sprintf_s(szPapersSavePath, "%sPaper\\%s", T2A(g_strCurrentPath), pPapers->strPapersName.c_str());
		sprintf_s(szZipBaseName, "%s", pPapers->strPapersName.c_str());
		sprintf_s(szZipName, "%s%s", pPapers->strPapersName.c_str(), T2A(PAPERS_EXT_NAME));
	}
	CString strInfo;
	bool bWarn = false;
	strInfo.Format(_T("正在保存%s..."), A2T(szZipName));

	//临时目录改名，以便压缩时继续扫描
	std::string strSrcPicDirPath;
	std::string strPicPath = szPapersSrcPath;
	try
	{
		Poco::File tmpPath(CMyCodeConvert::Gb2312ToUtf8(strPicPath));

		if (!tmpPath.exists())
		{
			std::string strErr = Poco::format("文件夹(%s)不存在。不能添加到压缩队列", strPicPath);
			g_pLogger->information(strErr);
			return;
		}

		char szCompressDirPath[MAX_PATH] = { 0 };
		sprintf_s(szCompressDirPath, "%sPaper\\%s_ToCompress", T2A(g_strCurrentPath), szZipBaseName);
		strSrcPicDirPath = szCompressDirPath;
		std::string strUtf8NewPath = CMyCodeConvert::Gb2312ToUtf8(strSrcPicDirPath);

		tmpPath.renameTo(strUtf8NewPath);
	}
	catch (Poco::Exception& exc)
	{
		std::string strLog = "临时文件夹重命名失败(" + exc.message() + "): ";
		strLog.append(strPicPath);
		g_pLogger->information(strLog);
		strSrcPicDirPath = strPicPath;
	}

	pCOMPRESSTASK pTask = new COMPRESSTASK;
//	pTask->bDelSrcDir = false;
	pTask->strCompressFileName = szZipName;
	pTask->strExtName = T2A(PAPERS_EXT_NAME);
	pTask->strSavePath = szPapersSavePath;
	pTask->strSrcFilePath = strSrcPicDirPath;
	pTask->pPapersInfo = pPapers;
	pTask->bReleasePapers = false;			//压缩完后，不用自动释放试卷袋信息，通过外部控制释放
	g_fmCompressLock.lock();
	g_lCompressTask.push_back(pTask);
	g_fmCompressLock.unlock();
#endif
}

LRESULT CPaperInputDlg::RoiLBtnDown(WPARAM wParam, LPARAM lParam)
{
	cv::Point pt = *(cv::Point*)(wParam);
//	ShowRectByPoint(pt, m_pCurrentShowPaper);
	return TRUE;
}

void CPaperInputDlg::ShowRectByPoint(cv::Point pt, pST_PaperInfo pPaper)
{
	if (!pPaper || !pPaper->pModel /*|| pPaper->pModel->vecPaperModel.size() < m_nCurrTabSel*/)
		return;

	// 	if (!pPaper->bIssuePaper)		//当前卷面没有问题点，不进行显示
	// 		return;

	int nFind = -1;
	RECTINFO* pRc = NULL;
	PIC_LIST::iterator itPic = pPaper->lPic.begin();
	for (int i = 0; itPic != pPaper->lPic.end(); itPic++, i++)
	{
// 		if (i == m_nCurrTabSel)
// 			break;
	}

	cv::Rect rt;
	rt.x = pt.x;
	rt.y = pt.y;
	//	GetPosition((*itPic)->lFix, pPaper->pModel->vecPaperModel[m_nCurrTabSel].lFix, rt);
	// 	GetPosition(pPaper->pModel->vecPaperModel[m_nCurrTabSel].lFix, (*itPic)->lFix, rt);
	// 	pt.x = rt.x;
	// 	pt.y = rt.y;
	nFind = GetRectInfoByPoint(pt, *itPic, pRc);
	if (nFind < 0)
		return;

	if (!pRc)
		return;

	CString strInfo;
	strInfo.Format(_T("阀值: %d, 要求比例: %f, 实际: %f"), pRc->nThresholdValue, pRc->fStandardValuePercent, pRc->fRealValuePercent);
	if (pPaper->bIssuePaper)
		SetStatusShowInfo(strInfo, TRUE);
	else
		SetStatusShowInfo(strInfo);
}

int CPaperInputDlg::GetRectInfoByPoint(cv::Point pt, pST_PicInfo pPic, RECTINFO*& pRc)
{
	int  nFind = -1;
	RECTLIST::iterator itIssueRectInfo = pPic->lIssueRect.begin();
	for (int i = 0; itIssueRectInfo != pPic->lIssueRect.end(); itIssueRectInfo++, i++)
	{
		if (itIssueRectInfo->rt.contains(pt))
		{
			nFind = i;
			pRc = &(*itIssueRectInfo);
			break;
		}
	}
	
	if (nFind < 0)
	{
		RECTLIST::iterator itRectInfo = pPic->lNormalRect.begin();
		for (int i = 0; itRectInfo != pPic->lNormalRect.end(); itRectInfo++, i++)
		{
			if (itRectInfo->rt.contains(pt))
			{
				nFind = i;
				pRc = &(*itRectInfo);
				break;
			}
		}
	}
	return nFind;
}

LRESULT CPaperInputDlg::MsgZkzhRecog(WPARAM wParam, LPARAM lParam)
{
	pST_PaperInfo pPaper = (pST_PaperInfo)wParam;
	pPAPERSINFO   pPapers = (pPAPERSINFO)lParam;
	CScanTool3Dlg* pDlg = (CScanTool3Dlg*)GetParent();
	if (g_nOperatingMode != 1 /*&& !pDlg->m_bModifySN*/ && !g_bModifySN)
		return FALSE;

	if (_pCurrExam_->nModel)
		return FALSE;

	CheckZkzhInBmk(pPaper);
	if (pPaper->nZkzhInBmkStatus == -1)
	{
		bool bFind = false;
		for (auto sn : m_vecCHzkzh)
		{
			if (sn == pPaper->strSN)
			{
				bFind = true;
				break;
			}
		}

		if (!bFind) m_vecCHzkzh.push_back(pPaper->strSN);	//重号的考号放入容器中，需要去重
	}

	if (pPapers != m_pCurrentPapers)
		return FALSE;

	USES_CONVERSION;
	int nCount = m_lPaperCtrl.GetItemCount();
	for (int i = 0; i < nCount; i++)
	{
		pST_PaperInfo pItemPaper = (pST_PaperInfo)(DWORD_PTR)m_lPaperCtrl.GetItemData(i);
		if (pItemPaper == pPaper)
		{
// 			m_lPaperCtrl.SetItemText(i, 1, (LPCTSTR)A2T(pPaper->strSN.c_str()));
// 			break;

			m_lPaperCtrl.EnsureVisible(i, FALSE);

			if (!pPaper->strSN.empty())
			{
				m_lPaperCtrl.SetItemText(i, 1, (LPCTSTR)A2T(pPaper->strSN.c_str()));

//				CheckZkzhInBmk(pPaper);
// 				if (pPaper->nZkzhInBmkStatus == -1)
// 				{
// 					bool bFind = false;
// 					for (auto sn : m_vecCHzkzh)
// 					{
// 						if (sn == pPaper->strSN)
// 						{
// 							bFind = true;
// 							break;
// 						}
// 					}
// 
// 					if (!bFind) m_vecCHzkzh.push_back(pPaper->strSN);	//重号的考号放入容器中，需要去重
// 				}

				// 				if (_bGetBmk_ && pPaper->nZkzhInBmkStatus != 1)
				// 					m_lcPicture.SetItemColors(i, 1, RGB(0, 255, 0), RGB(255, 255, 255));
			}
			else
			{
				if (pPaper->strRecogSN4Search.empty())
				{
					m_lPaperCtrl.SetItemText(i, 1, _T("考号识别为空"));
					//					m_lPaperCtrl.SetItemColors(i, 1, RGB(255, 0, 0), RGB(255, 255, 255));
				}
				else
				{
					m_lPaperCtrl.SetItemText(i, 1, _T("考号识别不完全"));
					//					m_lPaperCtrl.SetItemColors(i, 1, RGB(255, 0, 0), RGB(255, 255, 255));
				}
			}
			break;
		}
	}
	return TRUE;
}

void CPaperInputDlg::SetStatusShowInfo(CString strMsg, BOOL bWarn /*= FALSE*/)
{
	if (bWarn)
		m_colorStatus = RGB(255, 0, 0);
	else
		m_colorStatus = RGB(0, 0, 255);
	GetDlgItem(IDC_STATIC_TIPS)->SetWindowText(strMsg);
	TRACE("\n----------------\n");
	TRACE(strMsg);
	TRACE("\n----------------\n");
}

void CPaperInputDlg::SetFontSize(int nSize)
{
	m_fontStatus.DeleteObject();
	m_fontStatus.CreateFont(nSize, 0, 0, 0,
		FW_BOLD, FALSE, FALSE, 0,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS,
		_T("Arial"));
	GetDlgItem(IDC_STATIC_TIPS)->SetFont(&m_fontStatus);
}

HBRUSH CPaperInputDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	UINT CurID = pWnd->GetDlgCtrlID();

	if (IDC_STATIC_TIPS == CurID)
	{
		pDC->SetTextColor(m_colorStatus);

		return hbr;	// hbrsh;
	}
	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return hbr;
}

void CPaperInputDlg::ShowPaperByItem(int nItem)
{
	if (nItem < 0)
		return;

	pST_PaperInfo pPaper = (pST_PaperInfo)m_lPaperCtrl.GetItemData(nItem);

	m_pCurrentShowPaper = pPaper;
// 	if (pPaper->bIssuePaper)
// 		PaintIssueRect(pPaper);
// 	else
//		PaintRecognisedRect(pPaper);
//
// 	m_nCurrTabSel = 0;
// 
// 	m_tabPicShow.SetCurSel(0);
// 	m_pCurrentPicShow = m_vecPicShow[0];
// 	m_pCurrentPicShow->ShowWindow(SW_SHOW);
// 	for (int i = 0; i < m_vecPicShow.size(); i++)
// 	{
// 		if (i != 0)
// 			m_vecPicShow[i]->ShowWindow(SW_HIDE);
// 	}

	m_pShowPicDlg->setShowPaper(pPaper);
	m_pAnswerShowDlg->InitData(pPaper);
}


void CPaperInputDlg::ShowPapers(pPAPERSINFO pPapers)
{
	//显示所有识别完成的准考证号
	USES_CONVERSION;
	//重新显示合格试卷
	m_lPaperCtrl.DeleteAllItems();
	for (auto pPaper : pPapers->lPaper)
	{
		int nCount = m_lPaperCtrl.GetItemCount();
		char szCount[10] = { 0 };
		sprintf_s(szCount, "%d", pPaper->nIndex);	//nCount + 1
		m_lPaperCtrl.InsertItem(nCount, NULL);
		m_lPaperCtrl.SetItemText(nCount, 0, (LPCTSTR)A2T(szCount));
		m_lPaperCtrl.SetItemText(nCount, 1, (LPCTSTR)A2T(pPaper->strSN.c_str()));

		if (pPaper->bModifyZKZH)
			m_lPaperCtrl.SetItemColors(nCount, 1, RGB(0, 0, 255), RGB(255, 255, 255));
		if (pPaper->nZkzhInBmkStatus != 1 && _bGetBmk_)	//不在报名库中、重号
			m_lPaperCtrl.SetItemColors(nCount, 1, RGB(0, 255, 0), RGB(255, 255, 255));
		if (pPaper->nPicsExchange != 0)	//试卷被调换顺序
			m_lPaperCtrl.SetItemColors(nCount, 1, RGB(0, 255, 255), RGB(255, 255, 255));

		m_lPaperCtrl.SetItemData(nCount, (DWORD_PTR)pPaper);
	}
	//异常试卷放到正常试卷后面显示
	for (auto pPaper : pPapers->lIssue)
	{
		int nCount = m_lPaperCtrl.GetItemCount();
		char szCount[10] = { 0 };
		sprintf_s(szCount, "%d", pPaper->nIndex);
		m_lPaperCtrl.InsertItem(nCount, NULL);
		m_lPaperCtrl.SetItemText(nCount, 0, (LPCTSTR)A2T(szCount));
		CString strErrInfo = _T("");
		if (pPaper->strSN.empty())	strErrInfo = _T("考号为空");
		if (pPaper->bReScan)	strErrInfo = _T("删除");	//重扫
		m_lPaperCtrl.SetItemText(nCount, 1, (LPCTSTR)strErrInfo);	//2
		m_lPaperCtrl.SetItemData(nCount, (DWORD_PTR)pPaper);
		m_lPaperCtrl.SetItemColors(nCount, 1, RGB(255, 0, 0), RGB(255, 255, 255));

		CString strTips = _T("异常试卷，将不会上传，也不会参与评卷。 需要单独找出，后面单独扫描");
		m_lPaperCtrl.SetItemToolTipText(nCount, 0, strTips);
		m_lPaperCtrl.SetItemToolTipText(nCount, 1, strTips);
	}
	m_lPaperCtrl.Invalidate();
}

void CPaperInputDlg::OnLvnKeydownListPaper(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	*pResult = 0;
	
	if (m_nCurrItemPaperList < 0) return;

	if (pLVKeyDow->wVKey == VK_UP)
	{
		UnSetListCtrlHighLightShow(m_lPaperCtrl, m_nCurrItemPaperList);

		m_nCurrItemPaperList--;
		if (m_nCurrItemPaperList <= 0)
			m_nCurrItemPaperList = 0;

		SetListCtrlHighLightShow(m_lPaperCtrl, m_nCurrItemPaperList);
		ShowPaperByItem(m_nCurrItemPaperList);
	}
	else if (pLVKeyDow->wVKey == VK_DOWN)
	{
		UnSetListCtrlHighLightShow(m_lPaperCtrl, m_nCurrItemPaperList);

		m_nCurrItemPaperList++;
		if (m_nCurrItemPaperList >= m_lPaperCtrl.GetItemCount() - 1)
			m_nCurrItemPaperList = m_lPaperCtrl.GetItemCount() - 1;

		SetListCtrlHighLightShow(m_lPaperCtrl, m_nCurrItemPaperList);
		ShowPaperByItem(m_nCurrItemPaperList);
	}
}


void CPaperInputDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_CheckRecogComplete)
	{
		if (!m_pCurrentPapers)
		{
			KillTimer(TIMER_CheckRecogComplete);
			return;
		}
		bool bRecogComplete = true;
		bool bNeedShowZkzhDlg = false;
		for (auto p : m_pCurrentPapers->lPaper)
		{
			if (!p->bRecogComplete)
			{
				bRecogComplete = false;
				break;
			}
			if (p->strSN.empty() || (p->nZkzhInBmkStatus != 1 && _bGetBmk_))
				bNeedShowZkzhDlg = true;
		}
		if (bRecogComplete)
		{
			USES_CONVERSION;
			//++重新刷新一遍列表
			int nCount = m_lPaperCtrl.GetItemCount();
			for (int i = 0; i < nCount; i++)
			{
				pST_PaperInfo pItemPaper = (pST_PaperInfo)(DWORD_PTR)m_lPaperCtrl.GetItemData(i);
				if (pItemPaper)
				{
					if (!pItemPaper->strSN.empty())
					{
						m_lPaperCtrl.SetItemText(i, 1, (LPCTSTR)A2T(pItemPaper->strSN.c_str()));
						if (_bGetBmk_ && pItemPaper->nZkzhInBmkStatus != 1)
							m_lPaperCtrl.SetItemColors(i, 1, RGB(0, 255, 0), RGB(255, 255, 255));
						else if (_bGetBmk_ && pItemPaper->nZkzhInBmkStatus == 1)	//考号正常，检测有没有在重号列表中的（防止出现第1份试卷正常后面出现重号的，在前面重号的试卷无法检测的问题）
						{
							if (m_vecCHzkzh.size() > 0)
							{
								for (auto sn : m_vecCHzkzh)
								{
									if (sn == pItemPaper->strSN)
									{
										pItemPaper->nZkzhInBmkStatus = -1;		//重号
										break;
									}
								}
							}
						}
					}
					else
					{
						if (pItemPaper->strRecogSN4Search.empty())
						{
							m_lPaperCtrl.SetItemText(i, 1, _T("考号识别为空"));
							m_lPaperCtrl.SetItemColors(i, 1, RGB(255, 0, 0), RGB(255, 255, 255));
						}
						else
						{
							m_lPaperCtrl.SetItemText(i, 1, _T("考号识别不完全"));
							m_lPaperCtrl.SetItemColors(i, 1, RGB(255, 0, 0), RGB(255, 255, 255));
						}
					}
				}
			}
			//--
			CScanTool3Dlg* pDlg = (CScanTool3Dlg*)GetParent();
			if ((/*g_nOperatingMode == 1*/ /*|| pDlg->m_bModifySN*/g_bModifySN) && bNeedShowZkzhDlg)
			{
				KillTimer(TIMER_CheckRecogComplete);
				if (!m_pStudentMgr)
				{
					m_pStudentMgr = new CStudentMgr();
					std::string strDbPath = T2A(g_strCurrentPath + _T("bmk.db"));
					bool bResult = m_pStudentMgr->InitDB(CMyCodeConvert::Gb2312ToUtf8(strDbPath));
				}

			#ifdef TEST_MODIFY_ZKZH_CHIld
				CScanTool3Dlg* pDlg = (CScanTool3Dlg*)AfxGetMainWnd();
				pDlg->SwitchModifyZkzkDlg(m_pModel, m_pCurrentPapers, m_pStudentMgr);
			#else
				CModifyZkzhDlg zkzhDlg(_pModel_, m_pCurrentPapers, m_pStudentMgr);
				zkzhDlg.DoModal();
				ShowPapers(m_pCurrentPapers);
			#endif
			}
			else
				KillTimer(TIMER_CheckRecogComplete);
		}
	}
	CDialog::OnTimer(nIDEvent);
}

int CPaperInputDlg::CheckZkzhInBmk(std::string strZkzh)
{
	int nResult = 0;	//0--报名库不存在，1--报名库存在，-1--报名库检测到已经扫描
						//	for (auto& obj : g_lBmkStudent)
	for (auto& obj : m_lBmkStudent)
	{
		if (obj.strZkzh == strZkzh)
		{
			if (obj.nScaned == 1)
				nResult = -1;
			else
			{
				nResult = 1;
				obj.nScaned = 1;
			}
			break;
		}
	}

	return nResult;
}

void CPaperInputDlg::CheckZkzhInBmk(pST_PaperInfo pPaper)
{
	if (!pPaper) return;

	int nResult = CheckZkzhInBmk(pPaper->strSN);
	if (nResult == 1)
		pPaper->nZkzhInBmkStatus = 1;
	else if (nResult == -1)
		pPaper->nZkzhInBmkStatus = -1;
	else
		pPaper->nZkzhInBmkStatus = 0;
}

void CPaperInputDlg::InitTmpSubjectBmk()
{
	if (_bGetBmk_)
	{
		m_lBmkStudent.clear();
		m_lBmkStudent = g_lBmkStudent;
	}
}

BOOL CPaperInputDlg::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	pDC->FillRect(rcClient, &CBrush(RGB(255, 255, 255)));	//225, 222, 250
	ReleaseDC(pDC);

	return CDialog::OnEraseBkgnd(pDC);
}

void CPaperInputDlg::ReShowPapers()
{
	ShowPapers(m_pCurrentPapers);
}

void CPaperInputDlg::ReInitData(pMODEL pModel)
{
	if (m_pModel != m_pOldModel)
		SAFE_RELEASE(m_pModel);

	m_pOldModel = pModel;
	m_pModel = pModel;

	m_vecCHzkzh.clear();
	USES_CONVERSION;
	InitUI();
	SeachModel();
	if (NULL != m_pModel)
	{
		for (int i = 0; i < m_comboModel.GetCount(); i++)
		{
			CString strItemName;
			m_comboModel.GetLBText(i, strItemName);
			if (strItemName == A2T(m_pModel->strModelName.c_str()))
			{
				m_comboModel.SetCurSel(i);
				m_ncomboCurrentSel = i;
				break;
			}
		}
	}
	else
		m_comboModel.SetCurSel(0);

	InitTmpSubjectBmk();
}



void CPaperInputDlg::OnNMHoverListPaper(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 1;		//**********	这里如果不响应，同时返回结果值不为1的话，	****************
						//**********	就会产生产生TRACK SELECT，也就是鼠标悬停	****************
						//**********	一段时间后，所在行自动被选中
}


void CPaperInputDlg::OnNMHoverListPapers(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 1;		//**********	这里如果不响应，同时返回结果值不为1的话，	****************
						//**********	就会产生产生TRACK SELECT，也就是鼠标悬停	****************
						//**********	一段时间后，所在行自动被选中
}

void CPaperInputDlg::ChkAdjustFirstPic(pST_PaperInfo pPaper)
{
	pST_PicInfo pPreviousPic = NULL;	//该考生的上一页试卷，用来判断一张试卷的正反面
	Mat mtPreviousPic;					//记录前一页试卷的数据，省去IO读的时间
	clock_t sTime, mT0, mT1, mT2, mT3, eTime;
	sTime = clock();

	std::stringstream ssLog;
	ssLog << "检测考生(" << pPaper->strStudentInfo << ")的每张图像正反面:\n";

	bool bPreviousFirstPic = false;	//检测一张试卷时，上一页试卷是否为正面

	PIC_LIST::iterator itPic = pPaper->lPic.begin();
	for (int i = 0; itPic != pPaper->lPic.end(); i++, itPic++)
	{
		if (i == pPaper->lPic.size() - 1 && floor(i / 2) * 2 == i)	//(*itPic) == (*itEndPic)
		{
			ssLog << "考生试卷的最后一页"<< (*itPic)->strPicName << "(" << i + 1 << "/" << pPaper->lPic.size() << "), 属于最后一张试卷只有单独一面的情况，不需要检测正反面.\n";
			continue;
		}
		if (bPreviousFirstPic)
		{
			bPreviousFirstPic = false;
			ssLog << "图像" << (*itPic)->strPicName << "属于第" << floor(i / 2) << "张试卷的第" << i % 2 << "页(考生总试卷的第" << i << "面)，不需要检测，上一页试卷是正面\n";
			continue;
		}
		mT0 = clock();
		pST_PicInfo pPic = *itPic;
		Mat mtPic = imread(CMyCodeConvert::Utf8ToGb2312(pPic->strPicPath));

		mT1 = clock();
		bool bResult = _chkRotationObj.IsFirstPic(i, mtPic, m_pModel);
		ssLog << _chkRotationObj.GetRecogLog();

		mT2 = clock();
		if (bResult)
		{
			int nFrontPage = floor(i / 2) * 2;
			if (nFrontPage != i)	//说明该纸张的背面经过判断是正面，需要掉转两张图片
			{
				ssLog << "图像" << pPic->strPicName << "检测到属于第" << floor(i / 2) << "张试卷的正面(" << i + 1 << "/" << pPaper->lPic.size() << "), 开始与上一页试卷" << pPreviousPic->strPicName << "调换. " << (int)(mT1 - mT0) << ":" << (int)(mT2 - mT1) << "ms\n";
				//图像重命名
				bool bPicsExchangeSucc = true;
				pST_PicInfo pPic1 = pPic;
				pST_PicInfo pPic2 = pPreviousPic;
				try
				{
					Poco::File fPic1(pPic1->strPicPath);
					fPic1.renameTo(pPic1->strPicPath + "_tmp");

					Poco::File fPic2(pPic2->strPicPath);
					fPic2.renameTo(pPic1->strPicPath);

					fPic1.renameTo(pPic2->strPicPath);
				}
				catch (Poco::Exception &e)
				{
					bPicsExchangeSucc = false;
					std::string strErr = e.displayText();

					TRACE("图像%s与%s调换重命名失败: %s\n", pPic->strPicName.c_str(), pPreviousPic->strPicName.c_str(), strErr.c_str());
					ssLog << "图像" << pPic->strPicName << "与" << pPreviousPic->strPicName << "调换重命名失败: " << strErr << "\n";
				}
				mT3 = clock();

				if (bPicsExchangeSucc)
				{
					ssLog << "图像调换成功，开始重写原图. " << (int)(mT3 - mT2) << "ms\n";
					clock_t sT, eT1, eT2;
					sT = clock();

					bool bDoubleScan = true;	//使用此功能必须是双面扫描。	 m_pModel->vecPaperModel.size() % 2 == 0 ? true : false;
					COmrRecog omrObj;
					int nResult1 = omrObj.GetRightPicOrientation(mtPic, nFrontPage, bDoubleScan);
					eT1 = clock();
					ssLog << "新图像" << pPic1->strPicName << "方向调整: " << nResult1 << "(1:不需要旋转，2：右转90, 3：左转90, 4：右转180). " << (int)(eT1 - sT) << "ms\n";

					std::string strPicPath1 = pPic2->strPicPath;
					if (nResult1 >= 2 && nResult1 <= 4)
					{
						imwrite(strPicPath1, mtPic);
						eT2 = clock();
						ssLog << "新图像" << pPic1->strPicName << "写文件完成. " << (int)(eT2 - eT1) << "ms\n";
					}

					sT = clock();
					int nResult2 = omrObj.GetRightPicOrientation(mtPreviousPic, i, bDoubleScan);
					eT1 = clock();
					ssLog << "新图像" << pPic1->strPicName << "方向调整: " << nResult1 << "(1:不需要旋转，2：右转90, 3：左转90, 4：右转180). " << (int)(eT1 - sT) << "ms\n";

					std::string strPicPath2 = pPic1->strPicPath;
					if (nResult2 >= 2 && nResult2 <= 4)
					{
						imwrite(strPicPath2, mtPreviousPic);
						eT2 = clock();
						ssLog << "新图像" << pPic2->strPicName << "写文件完成. " << (int)(eT2 - eT1) << "ms\n";
					}
				}
			}
			else
			{
				//第1页为正面，第2页可以不需要判断
				bPreviousFirstPic = true;
				ssLog << "图像" << pPic->strPicName << "检测到属于第" << floor(i / 2) << "张试卷的正面(" << i + 1 << "/" << pPaper->lPic.size() << "), 下一页试卷不需要检测. " << (int)(mT1 - mT0) << ":" << (int)(mT2 - mT1) << "ms\n";
			}
		}
		else
		{
			bPreviousFirstPic = false;
			ssLog << "判断" << pPic->strPicName << "属于第" << floor(i / 2) << "张试卷的正面失败(" << i + 1 << "/" << pPaper->lPic.size() << ")，不能确定为正面. " << (int)(mT1 - sTime) << ":" << (int)(mT2 - mT1) << "ms\n";
		}
		pPreviousPic = pPic;
		mtPreviousPic = mtPic;
	}
	eTime = clock();
	ssLog << "判断考生(" << pPaper->strStudentInfo << ")正反面结束(" << (int)(eTime - sTime) << "ms)\n";
	TRACE(ssLog.str().c_str());
	g_pLogger->information(ssLog.str());
}

