// ExamInfoMgrDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ScanTool3.h"
#include "ExamInfoMgrDlg.h"
#include "afxdialogex.h"
#include "Net_Cmd_Protocol.h"


// CExamInfoMgrDlg 对话框

IMPLEMENT_DYNAMIC(CExamInfoMgrDlg, CDialog)

CExamInfoMgrDlg::CExamInfoMgrDlg(CWnd* pParent /*=NULL*/)
: CDialog(CExamInfoMgrDlg::IDD, pParent)
, m_nMaxShowExamListItem(0), m_nAllExamListItems(0), m_nShowPapersCount(0), m_nCurrShowPaper(1), m_nMaxSubsRow(3), m_nSubjectBtnH(30), m_nDlgMinH(138), m_strShowCurrPaper(_T(""))
, m_nChildDlgGap(10)
{

}

CExamInfoMgrDlg::~CExamInfoMgrDlg()
{
}

void CExamInfoMgrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_Subject, m_comboSubject);
	DDX_Control(pDX, IDC_COMBO_Grade, m_comboGrade);
	DDX_Control(pDX, IDC_COMBO_TK_Type, m_comboTkType);
	DDX_Text(pDX, IDC_STATIC_PaperCount, m_strShowCurrPaper);
	DDX_Control(pDX, IDC_BTN_First, m_bmpBtnFirst);
	DDX_Control(pDX, IDC_BTN_Last, m_bmpBtnLast);
	DDX_Control(pDX, IDC_BTN_Up, m_bmpBtnUp);
	DDX_Control(pDX, IDC_BTN_Down, m_bmpBtnDown);
	DDX_Control(pDX, IDC_BTN_Reflesh, m_bmpBtnReflesh);
}


BOOL CExamInfoMgrDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_bmpBkg.LoadBitmap(IDB_Main_Bk);
	m_bmpBtnReflesh.SetStateBitmap(IDB_Exam_Btn_First_normal, 0, IDB_Exam_Btn_First_Down);
	m_bmpBtnFirst.SetStateBitmap(IDB_Exam_Btn_First_normal, 0, IDB_Exam_Btn_First_Down);
	m_bmpBtnLast.SetStateBitmap(IDB_Exam_Btn_First_normal, 0, IDB_Exam_Btn_First_Down);
	m_bmpBtnUp.SetStateBitmap(IDB_Exam_Btn_First_normal, 0, IDB_Exam_Btn_First_Down);
	m_bmpBtnDown.SetStateBitmap(IDB_Exam_Btn_First_normal, 0, IDB_Exam_Btn_First_Down);
// 	m_bmpBtnFirst.SetStateBitmap(IDB_Exam_SubjectBtn, 0, IDB_Exam_SubjectBtn_Hover);
// 	m_bmpBtnLast.SetStateBitmap(IDB_Exam_SubjectBtn, 0, IDB_Exam_SubjectBtn_Hover);
// 	m_bmpBtnUp.SetStateBitmap(IDB_Exam_SubjectBtn, 0, IDB_Exam_SubjectBtn_Hover);
// 	m_bmpBtnDown.SetStateBitmap(IDB_Exam_SubjectBtn, 0, IDB_Exam_SubjectBtn_Hover);
	InitCtrlPosition();
	InitShowData();

	return TRUE;
}

BOOL CExamInfoMgrDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN)
		{
			return TRUE;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CExamInfoMgrDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO_Subject, &CExamInfoMgrDlg::OnCbnSelchangeComboSubject)
	ON_CBN_SELCHANGE(IDC_COMBO_Grade, &CExamInfoMgrDlg::OnCbnSelchangeComboGrade)
	ON_BN_CLICKED(IDC_BTN_First, &CExamInfoMgrDlg::OnBnClickedBtnFirst)
	ON_BN_CLICKED(IDC_BTN_Last, &CExamInfoMgrDlg::OnBnClickedBtnLast)
	ON_BN_CLICKED(IDC_BTN_Up, &CExamInfoMgrDlg::OnBnClickedBtnUp)
	ON_BN_CLICKED(IDC_BTN_Down, &CExamInfoMgrDlg::OnBnClickedBtnDown)
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_COMBO_TK_Type, &CExamInfoMgrDlg::OnCbnSelchangeComboTkType)
	ON_BN_CLICKED(IDC_BTN_Reflesh, &CExamInfoMgrDlg::OnBnClickedBtnReflesh)
END_MESSAGE_MAP()


// CExamInfoMgrDlg 消息处理程序
void CExamInfoMgrDlg::InitCtrlPosition()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.right;
	int cy = rcClient.bottom;

	const int nTopGap = 40;	//上边的间隔
	const int nBottomGap = 35;	//下边的间隔
	const int nLeftGap = 20;		//左边的空白间隔
	const int nRightGap = 20;	//右边的空白间隔
	int nGap = 5;

	m_rtExamList.left = nLeftGap;
	m_rtExamList.top = nTopGap;
	m_rtExamList.right = cx - nRightGap;
	m_rtExamList.bottom = nTopGap + cy - nBottomGap;

	int nStaticW = 50;
	int nCtrlH = 30;

	int nCurrLeft = nLeftGap;
	int nCurrTop = 5;
	if (GetDlgItem(IDC_STATIC_DlgMgr_Subject)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_DlgMgr_Subject)->MoveWindow(nCurrLeft, nCurrTop, nStaticW, nCtrlH);
		nCurrLeft += (nStaticW + nGap);
	}
	if (GetDlgItem(IDC_COMBO_Subject)->GetSafeHwnd())
	{
		GetDlgItem(IDC_COMBO_Subject)->MoveWindow(nCurrLeft, nCurrTop + 5, nStaticW * 2, nCtrlH);
		nCurrLeft += (nStaticW * 2 + nGap * 3);
	}
	if (GetDlgItem(IDC_STATIC_DlgMgr_Grade)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_DlgMgr_Grade)->MoveWindow(nCurrLeft, nCurrTop, nStaticW, nCtrlH);
		nCurrLeft += (nStaticW + nGap);
	}
	if (GetDlgItem(IDC_COMBO_Grade)->GetSafeHwnd())
	{
		GetDlgItem(IDC_COMBO_Grade)->MoveWindow(nCurrLeft, nCurrTop + 5, nStaticW * 2, nCtrlH);
		nCurrLeft += (nStaticW * 2 + nGap * 3);
	}
	if (GetDlgItem(IDC_STATIC_DlgMgr_TK_Type)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_DlgMgr_TK_Type)->MoveWindow(nCurrLeft, nCurrTop, nStaticW + 10, nCtrlH);
		nCurrLeft += (nStaticW + 10 + nGap);
	}
	if (GetDlgItem(IDC_COMBO_TK_Type)->GetSafeHwnd())
	{
		GetDlgItem(IDC_COMBO_TK_Type)->MoveWindow(nCurrLeft, nCurrTop + 5, nStaticW * 2, nCtrlH);
		nCurrLeft += (nStaticW * 2 + nGap * 3);
	}

	//bottom
	//插入页数索引
	int nSysLinkCount = m_vecBtnIndex.size();
	int nSysLinkW = 10;


	nGap = 5;
	int nBtnW = 45;
	int nBtnH = nBottomGap - nGap - nGap;
//	nCurrLeft = m_rtExamList.left + m_rtExamList.Width() / 2 - (nBtnW + nGap) * 2;
	nCurrLeft = m_rtExamList.left + m_rtExamList.Width() / 2 - (nBtnW + nGap) * 2 - (nSysLinkW + nGap) * nSysLinkCount / 2;
	nCurrTop = cy - nBottomGap + nGap;

	if (GetDlgItem(IDC_BTN_Reflesh)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_Reflesh)->MoveWindow(nCurrLeft - nBtnW - nGap, nCurrTop, nBtnW, nBtnH);
	}

	if (GetDlgItem(IDC_BTN_First)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_First)->MoveWindow(nCurrLeft, nCurrTop, nBtnW, nBtnH);
		nCurrLeft += (nBtnW + nGap);
	}
	if (GetDlgItem(IDC_BTN_Up)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_Up)->MoveWindow(nCurrLeft, nCurrTop, nBtnW, nBtnH);
		nCurrLeft += (nBtnW + nGap);
	}
	for (int i = 0; i < m_vecBtnIndex.size(); i++)
	{
		if (m_vecBtnIndex[i] && m_vecBtnIndex[i]->GetSafeHwnd())
		{
			m_vecBtnIndex[i]->MoveWindow(nCurrLeft, nCurrTop, nSysLinkW, nBtnH);
			nCurrLeft += (nSysLinkW + nGap);
		}
	}

	if (GetDlgItem(IDC_BTN_Down)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_Down)->MoveWindow(nCurrLeft, nCurrTop, nBtnW, nBtnH);
		nCurrLeft += (nBtnW + nGap);
	}
	if (GetDlgItem(IDC_BTN_Last)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_Last)->MoveWindow(nCurrLeft, nCurrTop, nBtnW, nBtnH);
		nCurrLeft += (nBtnW + nGap);
	}
	if (GetDlgItem(IDC_STATIC_PaperCount)->GetSafeHwnd())
	{
		int nW = nBtnW * 3;
		GetDlgItem(IDC_STATIC_PaperCount)->MoveWindow(nCurrLeft, nCurrTop, nW, nBtnH);
		nCurrLeft += (nW + nGap);
	}
//	Invalidate();
}

void CExamInfoMgrDlg::InitSearchData()
{
	std::vector<std::string> vecSub;
	vecSub.push_back("全部");
	std::vector<std::string> vecGrade;
	vecGrade.push_back("全部");
	std::vector<std::string> vecTkType;
	vecTkType.push_back("全部");

	std::vector<std::string> vecGradeTmp;

	//获取列表中所有科目名称信息、年级信息
	int nAllExamItems = 0;
	for (auto examObj : g_lExamList)
	{
		pEXAMINFO pExam = examObj;
		for (auto subObj : examObj->lSubjects)
		{
			pEXAM_SUBJECT pSub = subObj;
			bool bFind = false;
			for (auto strSubName : vecSub)
			{
				if (pSub->strSubjName == strSubName)
				{
					bFind = true;
					break;
				}
			}
			if (!bFind && pSub->strSubjName != "")
				vecSub.push_back(pSub->strSubjName);
		}

		bool bFindGrade = false;
		for (auto strGrade : vecGradeTmp)
		{
			if (pExam->strGradeName == strGrade)
			{
				bFindGrade = true;
				break;
			}
		}
		if (!bFindGrade && pExam->strGradeName != "")
			vecGradeTmp.push_back(pExam->strGradeName);

		if (pExam->nModel == 0)
		{
			bool bFind = false;
			for (auto strTkType : vecTkType)
			{
				if (strTkType == "网阅")
				{
					bFind = true;
					break;
				}
			}
			if (!bFind)
				vecTkType.push_back("网阅");
		}
		else
		{
			bool bFind = false;
			for (auto strTkType : vecTkType)
			{
				if (strTkType == "手阅")
				{
					bFind = true;
					break;
				}
			}
			if (!bFind)
				vecTkType.push_back("手阅");
		}

		nAllExamItems += pExam->lSubjects.size();
	}
	m_nAllExamListItems = nAllExamItems;

	//++年级信息排名
	std::sort(vecGradeTmp.begin(), vecGradeTmp.end(), SortStringByDown);
	for (auto itm : vecGradeTmp)
		vecGrade.push_back(itm);
	//--

	m_comboSubject.ResetContent();
	m_comboGrade.ResetContent();
	m_comboTkType.ResetContent();

	USES_CONVERSION;
	for (auto strSubName : vecSub)
		m_comboSubject.AddString(A2T(strSubName.c_str()));
	for (auto strGrade : vecGrade)
		m_comboGrade.AddString(A2T(strGrade.c_str()));
	for (auto strTkType : vecTkType)
		m_comboTkType.AddString(A2T(strTkType.c_str()));

	m_comboSubject.AdjustDroppedWidth();
	m_comboGrade.AdjustDroppedWidth();
	m_comboTkType.AdjustDroppedWidth();

	m_comboSubject.SetCurSel(0);
	m_comboGrade.SetCurSel(0);
	m_comboTkType.SetCurSel(0);
}

void CExamInfoMgrDlg::InitShowData()
{
	InitSearchData();
	GetSearchResultExamList();
	ReleaseDlgData();
	m_nCurrShowPaper = 1;
	ShowExamList(m_lExamList, m_nCurrShowPaper);
}

void CExamInfoMgrDlg::ReleaseData()
{
	EXAM_LIST::iterator itExam = m_lExamList.begin();
	for (; itExam != m_lExamList.end();)
	{
		pEXAMINFO pExam = *itExam;
		itExam = m_lExamList.erase(itExam);
		SAFE_RELEASE(pExam);
	}
}

void CExamInfoMgrDlg::GetSearchResultExamList()
{
	//++科目过滤
	USES_CONVERSION;
	CString strCurSub = _T("");
	CString strCurGrade = _T("");
	CString strCurTkType = _T("");
	m_comboSubject.GetLBText(m_comboSubject.GetCurSel(), strCurSub);
	m_comboGrade.GetLBText(m_comboGrade.GetCurSel(), strCurGrade);
	m_comboTkType.GetLBText(m_comboTkType.GetCurSel(), strCurTkType);
	std::string strCurrSubject = T2A(strCurSub);
	std::string strCurrGrade = T2A(strCurGrade);
	int nMode = -1;
	if (strCurTkType == _T("全部"))
		nMode = -1;
	else if (strCurTkType == _T("网阅"))
		nMode = 0;
	else if (strCurTkType == _T("手阅"))
		nMode = 1;
	//--
	
	EXAM_LIST::iterator itExam = m_lExamList.begin();
	for (; itExam != m_lExamList.end();)
	{
		pEXAMINFO pExam = *itExam;
		itExam = m_lExamList.erase(itExam);
		SAFE_RELEASE(pExam);
	}

	TRACE("全部考试有 %d个\n", g_lExamList.size());

	for (auto examObj : g_lExamList)
	{
		pEXAMINFO pExam = examObj;
		if ((pExam->strGradeName == strCurrGrade || strCurrGrade == "全部") && (pExam->nModel == nMode || nMode == -1))
		{
			pEXAMINFO pShowExam = NULL;
			bool bFind = false;
			for (auto subObj : examObj->lSubjects)
			{
				pEXAM_SUBJECT pSub = subObj;
				if (pSub->strSubjName == strCurrSubject || strCurrSubject == "全部")
				{
					if (!pShowExam)
					{
						pShowExam = new EXAMINFO();
						pShowExam->nExamID		= pExam->nExamID;
						pShowExam->nModel		= pExam->nModel;
//						pShowExam->nExamGrade	= pExam->nExamGrade;
						pShowExam->nExamState	= pExam->nExamState;
						pShowExam->strExamID	= pExam->strExamID;
						pShowExam->strExamName	= pExam->strExamName;
						pShowExam->strExamTypeName = pExam->strExamTypeName;
						pShowExam->strGradeName = pExam->strGradeName;
						pShowExam->strExamTime	= pExam->strExamTime;
						pShowExam->strPersonID	= pExam->strPersonID;
					}
					pEXAM_SUBJECT pShowSub = new EXAM_SUBJECT();
//					memcpy(pShowSub, pSub, sizeof(EXAM_SUBJECT));
					pShowSub->nSubjID = pSub->nSubjID;
					pShowSub->strModelName = pSub->strModelName;
					pShowSub->strSubjName = pSub->strSubjName;
					pShowExam->lSubjects.push_back(pShowSub);
				}
			}
			if (pShowExam)
				m_lExamList.push_back(pShowExam);
		}
	}

	GetAllShowPaperCount();
}

void CExamInfoMgrDlg::GetAllShowPaperCount()
{
	int nGap = 5;
	int nExamDlg_H = 45;				//考试信息列表的高度
	int nShowCount = 1;
	int nTmpTop = m_rtExamList.top;
	TRACE("显示的考试列表有%d个\n", m_lExamList.size());
	int i = 0;
	for (auto examObj : m_lExamList)
	{
		pEXAMINFO pExam = examObj;

		int nSubs = pExam->lSubjects.size();
		int nMaxCountInH = ceil((float)nSubs / (float)m_nMaxSubsRow);	//最多有几行按钮
		nExamDlg_H = nMaxCountInH * m_nSubjectBtnH + (nMaxCountInH - 1) * nGap + 30;
		if (nExamDlg_H < m_nDlgMinH) nExamDlg_H = m_nDlgMinH;

		i++;
		if (nTmpTop + nExamDlg_H > m_rtExamList.Height())
		{
			nShowCount++;
			nTmpTop = m_rtExamList.top;
			TRACE("分页%d, 在第%d个考试\n", nShowCount, i);
		}

		nTmpTop += (nExamDlg_H + m_nChildDlgGap);
	}
	m_nShowPapersCount = nShowCount;
}

int CExamInfoMgrDlg::GetStartExamIndex(int n)
{
	int nGap = 5;
	int nExamDlg_H = 45;				//考试信息列表的高度

	int nResult = 0;
	int nShowCount = 1;
	int nTmpTop = m_rtExamList.top;
	EXAM_LIST::iterator it = m_lExamList.begin();
	for (int i = 1; it != m_lExamList.end(); i++, it++)
	{
		pEXAMINFO pExam = *it;

		int nSubs = pExam->lSubjects.size();
		int nMaxCountInH = ceil((float)nSubs / (float)m_nMaxSubsRow);	//最多有几行按钮
		nExamDlg_H = nMaxCountInH * m_nSubjectBtnH + (nMaxCountInH - 1) * nGap + 30;
		if (nExamDlg_H < m_nDlgMinH) nExamDlg_H = m_nDlgMinH;

		if (nTmpTop + nExamDlg_H > m_rtExamList.Height())
		{
			nShowCount++;
			nTmpTop = m_rtExamList.top;
		}

		nTmpTop += (nExamDlg_H + m_nChildDlgGap);

		if (nShowCount == n)
		{
			nResult = i;
			break;
		}
	}
	return nResult;
}

void CExamInfoMgrDlg::DrawBorder(CDC *pDC)
{
	CPen *pOldPen = NULL;
	CPen pPen;
	CRect rcClient(0, 0, 0, 0);
	GetClientRect(&rcClient);
	pPen.CreatePen(PS_SOLID, 1, RGB(118, 190, 254));

	pDC->SelectStockObject(NULL_BRUSH);
	pOldPen = pDC->SelectObject(&pPen);
	pDC->Rectangle(&rcClient);
	pDC->SelectObject(pOldPen);
	pPen.Detach();
	ReleaseDC(pDC);
}

void CExamInfoMgrDlg::ShowExamList(EXAM_LIST lExam, int nStartShow)
{
	if (nStartShow > m_nAllExamListItems) return;
	if (nStartShow <= 0) return;

	ReleaseDlgData();
	if (lExam.size() <= 0) return;

	int nGap = 5;
	int nExamDlg_H = 45;				//考试信息列表的高度
	int nRealW = m_rtExamList.Width();	//考试信息列表的宽度
	int nRealH = m_rtExamList.Height();	//实际有效的显示考试列表窗口的高度
	
	int nCurrLeft = m_rtExamList.left;
	int nCurrTop = m_rtExamList.top;

	TRACE("起始显示列表中第%d个\n", nStartShow);

	int nCount = 0;
	for (auto examObj : lExam)
	{
		pEXAMINFO pExam = examObj;

		nCount++;
		if (nCount >= nStartShow)
		{
			int nSubs = pExam->lSubjects.size();
			int nMaxCountInH = ceil((float)nSubs / (float)m_nMaxSubsRow);	//最多有几行按钮
			nExamDlg_H = nMaxCountInH * m_nSubjectBtnH + (nMaxCountInH - 1) * nGap + 30;
			if (nExamDlg_H < m_nDlgMinH) nExamDlg_H = m_nDlgMinH;

			if (nCurrTop + nExamDlg_H > m_rtExamList.Height())
				break;

			CSingleExamDlg* pExamDlg = new CSingleExamDlg(this);
			pExamDlg->Create(CSingleExamDlg::IDD, this);
			pExamDlg->ShowWindow(SW_HIDE);
			pExamDlg->MoveWindow(nCurrLeft, nCurrTop, nRealW, nExamDlg_H);
			nCurrTop += (nExamDlg_H + m_nChildDlgGap);

			pExamDlg->m_nMaxSubsRow		= m_nMaxSubsRow;
			pExamDlg->m_nSubjectBtnH	= m_nSubjectBtnH;

			pExamDlg->SetExamInfo(pExam);
			pExamDlg->ShowWindow(SW_SHOW);
			m_vecExamInfoDlg.push_back(pExamDlg);
		}
	}
	m_nAllExamListItems = lExam.size();

	m_strShowCurrPaper.Format(_T("当前页: %d/%d"), m_nCurrShowPaper, m_nShowPapersCount);
	UpdateData(FALSE);
	Invalidate();
//	GetDlgItem(IDC_STATIC_PaperCount)->Invalidate();
}


void CExamInfoMgrDlg::ReleaseDlgData()
{
	int nCoutn = m_vecExamInfoDlg.size();
	for (int i = 0; i < m_vecExamInfoDlg.size(); i++)
	{
		CSingleExamDlg* pDlg = m_vecExamInfoDlg[i];
		pDlg->DestroyWindow();
		SAFE_RELEASE(pDlg);
	}
	m_vecExamInfoDlg.clear();
	for (int i = 0; i < m_vecBtnIndex.size(); i++)
	{
		CLinkCtrl * pLinkCtrl = m_vecBtnIndex[i];
		SAFE_RELEASE(pLinkCtrl);
		m_vecBtnIndex[i] = NULL;
	}
	m_vecBtnIndex.clear();
}

void CExamInfoMgrDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	InitCtrlPosition();
	GetAllShowPaperCount();
	m_nCurrShowPaper = 1;
	ShowExamList(m_lExamList, m_nCurrShowPaper);
	Invalidate();
}


BOOL CExamInfoMgrDlg::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	CDialog::OnEraseBkgnd(pDC);

	int iX, iY;
	CDC memDC;
	CRect rectClient;
	BITMAP bmp;

	iX = iY = 0;
	GetClientRect(&rectClient);

	if (memDC.CreateCompatibleDC(pDC))
	{
		CBitmap *pOldBmp = memDC.SelectObject(&m_bmpBkg);
		m_bmpBkg.GetBitmap(&bmp);
		pDC->SetStretchBltMode(COLORONCOLOR);
		pDC->StretchBlt(iX, iY, rectClient.Width(), rectClient.Height(), &memDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
		memDC.SelectObject(pOldBmp);
	}
	memDC.DeleteDC();


//	pDC->FillRect(rcClient, &CBrush(RGB(255, 255, 255)));	//225, 222, 250
	DrawBorder(pDC);

	return TRUE;
}


void CExamInfoMgrDlg::OnDestroy()
{
	CDialog::OnDestroy();

	ReleaseData();
	ReleaseDlgData();
}


void CExamInfoMgrDlg::OnCbnSelchangeComboSubject()
{
	if (m_comboSubject.GetCurSel() < 0)
		return;
	GetSearchResultExamList();
	m_nCurrShowPaper = 1;
	ShowExamList(m_lExamList, m_nCurrShowPaper);
}


void CExamInfoMgrDlg::OnCbnSelchangeComboGrade()
{
	if (m_comboGrade.GetCurSel() < 0)
		return;

	GetSearchResultExamList();
	m_nCurrShowPaper = 1;
	ShowExamList(m_lExamList, m_nCurrShowPaper);
}


void CExamInfoMgrDlg::OnBnClickedBtnFirst()
{
	if (m_lExamList.size() == 0)
		GetSearchResultExamList();

	m_nCurrShowPaper = 1;
	int nCurrStartShowExamListItem = GetStartExamIndex(m_nCurrShowPaper);
	ShowExamList(m_lExamList, nCurrStartShowExamListItem);
}


void CExamInfoMgrDlg::OnBnClickedBtnLast()
{
	if (m_lExamList.size() == 0)
		GetSearchResultExamList();

	m_nCurrShowPaper = m_nShowPapersCount;
	int nCurrStartShowExamListItem = GetStartExamIndex(m_nCurrShowPaper);
	ShowExamList(m_lExamList, nCurrStartShowExamListItem);
}


void CExamInfoMgrDlg::OnBnClickedBtnUp()
{
	m_nCurrShowPaper--;
	if (m_nCurrShowPaper < 1) m_nCurrShowPaper = 1;
	int nCurrStartShowExamListItem = GetStartExamIndex(m_nCurrShowPaper);
	ShowExamList(m_lExamList, nCurrStartShowExamListItem);
}


void CExamInfoMgrDlg::OnBnClickedBtnDown()
{
	m_nCurrShowPaper++;
	if (m_nCurrShowPaper > m_nShowPapersCount) m_nCurrShowPaper = m_nShowPapersCount;
	int nCurrStartShowExamListItem = GetStartExamIndex(m_nCurrShowPaper);
	ShowExamList(m_lExamList, nCurrStartShowExamListItem);
}

HBRUSH CExamInfoMgrDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	UINT CurID = pWnd->GetDlgCtrlID();
	if (CurID == IDC_STATIC_DlgMgr_Subject || CurID == IDC_STATIC_DlgMgr_Grade || CurID == IDC_STATIC_PaperCount || CurID == IDC_STATIC_DlgMgr_TK_Type)
	{
		//		pDC->SetBkColor(RGB(255, 255, 255));
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	
	return hbr;
}

void CExamInfoMgrDlg::OnCbnSelchangeComboTkType()
{
	if (m_comboTkType.GetCurSel() < 0)
		return;

	GetSearchResultExamList();
	m_nCurrShowPaper = 1;
	ShowExamList(m_lExamList, m_nCurrShowPaper);
}


void CExamInfoMgrDlg::OnBnClickedBtnReflesh()
{
	USES_CONVERSION;
	if (!_bLogin_)
		return ;

	int nResult = 0;

	ST_EXAM_INFO stExamInfo;
	ZeroMemory(&stExamInfo, sizeof(ST_EXAM_INFO));
	if (_bHandModel_)
	{
#if 1
		std::string strTmp = _strPersonID_ + "###" + _strSchoolID_;
		strcpy(stExamInfo.szEzs, strTmp.c_str());
#else
		strcpy(stExamInfo.szEzs, T2A(m_strPersonId));
#endif
	}
	else
		strcpy(stExamInfo.szEzs, _strEzs_.c_str());

	pTCP_TASK pTcpTask = new TCP_TASK;
	pTcpTask->usCmd = USER_GETEXAMINFO;
	pTcpTask->nPkgLen = sizeof(ST_EXAM_INFO);
	memcpy(pTcpTask->szSendBuf, (char*)&stExamInfo, sizeof(ST_EXAM_INFO));
	g_fmTcpTaskLock.lock();
	g_lTcpTask.push_back(pTcpTask);
	g_fmTcpTaskLock.unlock();

	g_eGetExamList.reset();


	EXAM_LIST::iterator itExam = g_lExamList.begin();
	for (; itExam != g_lExamList.end();)
	{
		pEXAMINFO pExam = *itExam;
		itExam = g_lExamList.erase(itExam);
		SAFE_RELEASE(pExam);
	}
	EXAM_LIST::iterator itExam2 = m_lExamList.begin();
	for (; itExam2 != m_lExamList.end();)
	{
		pEXAMINFO pExam = *itExam2;
		itExam2 = m_lExamList.erase(itExam2);
		SAFE_RELEASE(pExam);
	}
	m_nCurrShowPaper = 1;
	ShowExamList(m_lExamList, m_nCurrShowPaper);

	try
	{
		g_eGetExamList.wait(10000);
	}
	catch (Poco::TimeoutException &e)
	{
		TRACE("获取报名库超时\n");
	}
	InitShowData();

	return ;
}
