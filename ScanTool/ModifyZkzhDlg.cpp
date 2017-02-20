// ModifyZkzhDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ScanTool.h"
#include "ModifyZkzhDlg.h"
#include "afxdialogex.h"


// CModifyZkzhDlg �Ի���

IMPLEMENT_DYNAMIC(CModifyZkzhDlg, CDialog)

CModifyZkzhDlg::CModifyZkzhDlg(pMODEL pModel, pPAPERSINFO pPapersInfo, CWnd* pParent /*=NULL*/)
	: CDialog(CModifyZkzhDlg::IDD, pParent)
	, m_pCurrentPicShow(NULL), m_pModel(pModel), m_pPapers(pPapersInfo), m_nModelPicNums(1), m_nCurrTabSel(0), m_pCurrentShowPaper(NULL)
	, m_nCurrentSelItem(0)
{

}

CModifyZkzhDlg::~CModifyZkzhDlg()
{
	std::vector<CPicShow*>::iterator itPic = m_vecPicShow.begin();
	for (; itPic != m_vecPicShow.end();)
	{
		CPicShow* pModelPicShow = *itPic;
		SAFE_RELEASE(pModelPicShow);
		itPic = m_vecPicShow.erase(itPic);
	}
}

void CModifyZkzhDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_Zkzh, m_lcZkzh);
	DDX_Control(pDX, IDC_TAB_ZkzhPic, m_tabPicShowCtrl);
	DDX_Text(pDX, IDC_EDIT_Zkzh, m_strCurZkzh);
}


BOOL CModifyZkzhDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (NULL != m_pModel)
	{
		m_nModelPicNums = m_pModel->nPicNum;
	}
	InitUI();
	InitData();
	ShowPaperByItem(m_nCurrentSelItem);

	return TRUE;
}

BEGIN_MESSAGE_MAP(CModifyZkzhDlg, CDialog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_ZkzhPic, &CModifyZkzhDlg::OnTcnSelchangeTabZkzhpic)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_Zkzh, &CModifyZkzhDlg::OnNMDblclkListZkzh)
	ON_NOTIFY(NM_HOVER, IDC_LIST_Zkzh, &CModifyZkzhDlg::OnNMHoverListZkzh)
	ON_REGISTERED_MESSAGE(WM_XLISTCTRL_EDIT_END, OnEditEnd)
	ON_BN_CLICKED(IDC_BTN_SAVE, &CModifyZkzhDlg::OnBnClickedBtnSave)
END_MESSAGE_MAP()


// CModifyZkzhDlg ��Ϣ��������

void CModifyZkzhDlg::InitUI()
{
	m_lcZkzh.SetExtendedStyle(m_lcZkzh.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_SHOWSELALWAYS);
	m_lcZkzh.InsertColumn(0, _T("˳��"), LVCFMT_CENTER, 40);
	m_lcZkzh.InsertColumn(1, _T("ѧ����ʶ"), LVCFMT_CENTER, 150);
	m_lcZkzh.InsertColumn(2, _T("�༭ ׼��֤��"), LVCFMT_CENTER, 150);

	InitTab();
	InitCtrlPosition();
}

void CModifyZkzhDlg::InitTab()
{
	if (m_pModel)
	{
		std::vector<CPicShow*>::iterator itPic = m_vecPicShow.begin();
		for (; itPic != m_vecPicShow.end();)
//		for (auto itPic : m_vecPicShow)
		{
			CPicShow* pModelPicShow = *itPic;
			if (pModelPicShow)
			{
				delete pModelPicShow;
				pModelPicShow = NULL;
			}
			itPic = m_vecPicShow.erase(itPic);
		}
	}
	m_tabPicShowCtrl.DeleteAllItems();

	USES_CONVERSION;
	CRect rtTab;
	m_tabPicShowCtrl.GetClientRect(&rtTab);
	for (int i = 0; i < m_nModelPicNums; i++)
	{
		char szTabHeadName[20] = { 0 };
		sprintf_s(szTabHeadName, "��%dҳ", i + 1);

		m_tabPicShowCtrl.InsertItem(i, A2T(szTabHeadName));

		CPicShow* pPicShow = new CPicShow(this);
		pPicShow->Create(CPicShow::IDD, &m_tabPicShowCtrl);
		pPicShow->ShowWindow(SW_HIDE);
		pPicShow->MoveWindow(&rtTab);
		m_vecPicShow.push_back(pPicShow);
	}
	m_tabPicShowCtrl.SetCurSel(0);
	if (m_vecPicShow.size())
	{
		m_vecPicShow[0]->ShowWindow(SW_SHOW);
		m_pCurrentPicShow = m_vecPicShow[0];
	}

	if (m_tabPicShowCtrl.GetSafeHwnd())
	{
		CRect rtTab;
		m_tabPicShowCtrl.GetClientRect(&rtTab);
		int nTabHead_H = 24;		//tab�ؼ�ͷ�ĸ߶�
		CRect rtPic = rtTab;
		rtPic.top = rtPic.top + nTabHead_H;
		rtPic.left += 2;
		rtPic.right -= 4;
		rtPic.bottom -= 4;
		for (int i = 0; i < m_vecPicShow.size(); i++)
			m_vecPicShow[i]->MoveWindow(&rtPic);
	}
}

void CModifyZkzhDlg::InitCtrlPosition()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.right;
	int cy = rcClient.bottom;

	int nTopGap = 5;	//�ϱߵļ��������������
	const int nLeftGap = 5;		//��ߵĿհ׼��
	const int nBottomGap = 2;	//�±ߵĿհ׼��
	const int nRightGap = 2;	//�ұߵĿհ׼��
	const int nGap = 2;			//��ͨ�ؼ��ļ��

	int nListCtrlWidth = 350;	//ͼƬ�б��ؼ�����
	int nStaticTip = 15;		//�б���ʾstatic�ؼ��߶�
	int nBtnH = 30;				//��ť�߶�
	int nBtnW = (nListCtrlWidth - nGap) * 0.3;
	int nBottomH = 100;			//���²��ֵĸ߶ȣ����ڷ��ð�ť��
	int nCurrentTop = nTopGap;
	int nCurrentLeft = nLeftGap;
	if (GetDlgItem(IDC_STATIC_Zkzh_S1)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_Zkzh_S1)->MoveWindow(nCurrentLeft, nCurrentTop, nListCtrlWidth, nStaticTip);
		nCurrentTop += (nStaticTip + nGap);
	}
	int nZkzhLCH = cy - nTopGap - nStaticTip - nGap - nStaticTip - nGap - nBtnH - nBottomH - nBottomGap;
	if (m_lcZkzh.GetSafeHwnd())
	{
		m_lcZkzh.MoveWindow(nCurrentLeft, nCurrentTop, nListCtrlWidth, nZkzhLCH);
//		nCurrentLeft += (nListCtrlWidth + nGap);
		nCurrentTop += (nZkzhLCH + nGap);
	}
	if (GetDlgItem(IDC_STATIC_Zkzh_S2)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_Zkzh_S2)->MoveWindow(nCurrentLeft, nCurrentTop, nListCtrlWidth, nStaticTip);
		nCurrentTop += (nStaticTip + nGap);
	}
	if (GetDlgItem(IDC_STATIC_Zkzh_S3)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_Zkzh_S3)->MoveWindow(nCurrentLeft, nCurrentTop, 35, nBtnH);
		nCurrentLeft += (35 + nGap);
	}
	if (GetDlgItem(IDC_EDIT_Zkzh)->GetSafeHwnd())
	{
		GetDlgItem(IDC_EDIT_Zkzh)->MoveWindow(nCurrentLeft, nCurrentTop, nListCtrlWidth - nGap - 35 - nGap - nBtnW, nBtnH);
		nCurrentLeft += (nListCtrlWidth - nGap - 35 - nGap - nBtnW + nGap);
	}
	if (GetDlgItem(IDC_BTN_SAVE)->GetSafeHwnd())
	{
		GetDlgItem(IDC_BTN_SAVE)->MoveWindow(nCurrentLeft, nCurrentTop, nBtnW, nBtnH);
		nCurrentLeft = nLeftGap + nListCtrlWidth + nGap;
	}

	//tab
	nCurrentTop = nTopGap;
	int nPicShowTabCtrlWidth = cx - nLeftGap - nRightGap - nListCtrlWidth - nGap - nGap;
	int nPicShowTabCtrlHigh = cy - nTopGap - nBottomH - nBottomGap;
	if (m_tabPicShowCtrl.GetSafeHwnd())
	{
		m_tabPicShowCtrl.MoveWindow(nCurrentLeft, nTopGap, nPicShowTabCtrlWidth, nPicShowTabCtrlHigh);

		CRect rtTab;
		m_tabPicShowCtrl.GetClientRect(&rtTab);
		int nTabHead_H = 24;		//tab�ؼ�ͷ�ĸ߶�
		CRect rtPic = rtTab;
		rtPic.top = rtPic.top + nTabHead_H;
		rtPic.left += 2;
		rtPic.right -= 4;
		rtPic.bottom -= 4;
		for (int i = 0; i < m_vecPicShow.size(); i++)
			m_vecPicShow[i]->MoveWindow(&rtPic);
	}
}

void CModifyZkzhDlg::InitData()
{
	if (NULL == m_pPapers)
		return;

	USES_CONVERSION;
	for (auto pPaper : m_pPapers->lPaper)
	{
		if (pPaper->strSN.empty())
		{
			//���ӽ��Ծ��б��ؼ�
			int nCount = m_lcZkzh.GetItemCount();
			char szCount[10] = { 0 };
			sprintf_s(szCount, "%d", nCount + 1);
			m_lcZkzh.InsertItem(nCount, NULL);

			m_lcZkzh.SetItemText(nCount, 0, (LPCTSTR)A2T(szCount));
			m_lcZkzh.SetItemText(nCount, 1, (LPCTSTR)A2T(pPaper->strStudentInfo.c_str()));
			m_lcZkzh.SetItemText(nCount, 2, (LPCTSTR)A2T(pPaper->strSN.c_str()));
			m_lcZkzh.SetItemData(nCount, (DWORD_PTR)pPaper);
			m_lcZkzh.SetEdit(nCount, 2);
		}
	}
}

void CModifyZkzhDlg::ShowPaperByItem(int nItem)
{
	if (nItem < 0)
		return;
	if (nItem >= m_lcZkzh.GetItemCount())
		return;

	pST_PaperInfo pPaper = (pST_PaperInfo)m_lcZkzh.GetItemData(nItem);

	m_pCurrentShowPaper = pPaper;
	ShowPaperZkzhPosition(pPaper);
// 	m_lcZkzh.SetItemState(nItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
// 	m_lcZkzh.SetSelectionMark(nItem);
// 	m_lcZkzh.SetFocus();
// 	m_lcZkzh.SetItemState(nItem, LVIS_DROPHILITED, LVIS_DROPHILITED);		//������ʾһ�У�ʧȥ�����Ҳһֱ��ʾ

	
	m_lcZkzh.GetItemColors(nItem, 0, crOldText, crOldBackground);
	for (int i = 0; i < m_lcZkzh.GetColumns(); i++)							//���ø�����ʾ(�ֶ����ñ�����ɫ)
		m_lcZkzh.SetItemColors(nItem, i, RGB(0, 0, 0), RGB(70, 70, 255));

	USES_CONVERSION;
	m_nCurrTabSel = 0;
	m_strCurZkzh = pPaper->strSN.c_str();
	GetDlgItem(IDC_STATIC_Zkzh_S3)->SetWindowText(A2T(pPaper->strStudentInfo.c_str()));
	GetDlgItem(IDC_EDIT_Zkzh)->SetFocus();

	m_tabPicShowCtrl.SetCurSel(0);
	m_pCurrentPicShow = m_vecPicShow[0];
	m_pCurrentPicShow->ShowWindow(SW_SHOW);
	for (int i = 0; i < m_vecPicShow.size(); i++)
	{
		if (i != 0)
			m_vecPicShow[i]->ShowWindow(SW_HIDE);
	}
	UpdateData(FALSE);
	m_lcZkzh.Invalidate();
}

void CModifyZkzhDlg::ShowPaperZkzhPosition(pST_PaperInfo pPaper)
{
	if (NULL == pPaper)
		return;

	PIC_LIST::iterator itPic = pPaper->lPic.begin();
	for (int i = 0; itPic != pPaper->lPic.end(); itPic++, i++)
	{
		cv::Mat matSrc = cv::imread((*itPic)->strPicPath);
#ifdef PIC_RECTIFY_TEST
		cv::Mat dst;
		cv::Mat rotMat;
		PicRectify(matSrc, dst, rotMat);
		cv::Mat matImg;
		if (dst.channels() == 1)
			cvtColor(dst, matImg, CV_GRAY2BGR);
		else
			matImg = dst;
		// #ifdef WarpAffine_TEST
		// 		cv::Mat	inverseMat(2, 3, CV_32FC1);
		// 		PicTransfer(i, matImg, (*itPic)->lFix, pPaper->pModel->vecPaperModel[i]->lFix, inverseMat);
		// #endif
#else
		cv::Mat matImg = matSrc;
#endif

#ifdef WarpAffine_TEST
		cv::Mat	inverseMat(2, 3, CV_32FC1);
		if (pPaper->pModel)
			PicTransfer(i, matImg, (*itPic)->lFix, pPaper->pModel->vecPaperModel[i]->lFix, inverseMat);
#endif

#ifdef Test_ShowOriPosition
		cv::Mat	inverseMat(2, 3, CV_32FC1);
		GetInverseMat((*itPic)->lFix, pPaper->pModel->vecPaperModel[i]->lFix, inverseMat);
#endif

		cv::Point pt(0, 0);
		if (pPaper->pModel)
			pt = pPaper->pModel->vecPaperModel[i]->rcSNTracker.rt.tl() - cv::Point(100, 100);

		m_vecPicShow[i]->ShowPic(matImg, pt);
	}
}

BOOL CModifyZkzhDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)	//pMsg->wParam == VK_ESCAPE
		{
			return TRUE;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void CModifyZkzhDlg::OnTcnSelchangeTabZkzhpic(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int nIndex = m_tabPicShowCtrl.GetCurSel();
	m_nCurrTabSel = nIndex;

	m_pCurrentPicShow = m_vecPicShow[nIndex];
	m_pCurrentPicShow->ShowWindow(SW_SHOW);
	for (int i = 0; i < m_vecPicShow.size(); i++)
	{
		if (i != nIndex)
			m_vecPicShow[i]->ShowWindow(SW_HIDE);
	}
}


void CModifyZkzhDlg::OnNMDblclkListZkzh(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem < 0)
		return;

	for (int i = 0; i < m_lcZkzh.GetColumns(); i++)
		if (!m_lcZkzh.GetModified(m_nCurrentSelItem, i))
			m_lcZkzh.SetItemColors(m_nCurrentSelItem, i, crOldText, crOldBackground);
		else
			m_lcZkzh.SetItemColors(m_nCurrentSelItem, i, RGB(255, 0, 0), crOldBackground);

//	m_lcZkzh.SetItemState(m_nCurrentSelItem, 0, LVIS_DROPHILITED);
	m_nCurrentSelItem = pNMItemActivate->iItem;
	ShowPaperByItem(pNMItemActivate->iItem);

	*pResult = 0;
}


void CModifyZkzhDlg::OnNMHoverListZkzh(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 1;		//**********	�����������Ӧ��ͬʱ���ؽ��ֵ��Ϊ1�Ļ���	****************
						//**********	�ͻ��������TRACK SELECT��Ҳ���������ͣ	****************
						//**********	һ��ʱ����������Զ���ѡ��
}

LRESULT CModifyZkzhDlg::OnEditEnd(WPARAM nItem, LPARAM nSubItem)
{
	if (nItem >= 0 && nSubItem >= 0)
	{
		USES_CONVERSION;
		CString strText = m_lcZkzh.GetItemText(nItem, nSubItem);
		pST_PaperInfo pPaper = (pST_PaperInfo)m_lcZkzh.GetItemData(nItem);
		pPaper->strSN = T2A(strText);
		m_strCurZkzh = strText;
		GetDlgItem(IDC_STATIC_Zkzh_S3)->SetWindowText(A2T(pPaper->strStudentInfo.c_str()));

//		m_lcZkzh.SetItemState(nItem, 0, LVIS_DROPHILITED);
		for (int i = 0; i < m_lcZkzh.GetColumns(); i++)						//ȡ��������ʾ(�ֶ����ñ�����ɫ)
			if(!m_lcZkzh.GetModified(m_nCurrentSelItem, i))
				m_lcZkzh.SetItemColors(m_nCurrentSelItem, i, crOldText, crOldBackground);
			else
				m_lcZkzh.SetItemColors(m_nCurrentSelItem, i, RGB(255, 0, 0), crOldBackground);

		COLORREF crText, crBackground;
		m_lcZkzh.GetItemColors(nItem, nSubItem, crText, crBackground);
		if (m_lcZkzh.GetModified(nItem, nSubItem))
		{
			// subitem was modified - color it red
			m_lcZkzh.SetItemText(nItem, nSubItem, strText,
							   RGB(255, 0, 0), crBackground);
//			crOldText = RGB(255, 0, 0);
		}
		else
		{
			// subitem not modified - color it black -
			// note that once modified, a subitem will remain
			// marked as modified
			m_lcZkzh.SetItemText(nItem, nSubItem, strText,
								 RGB(0, 0, 0), crBackground);
//			crOldText = RGB(0, 0, 0);
		}
		m_nCurrentSelItem = nItem;
		UpdateData(FALSE);
		m_lcZkzh.Invalidate();
// 		POINT pt;
// 		pt.x = 0;
// 		pt.y = 0;
// 		if (nItem < m_lcZkzh.GetItemCount() - 1)	//������һ��
// 		{
// 			m_lcZkzh.SetItemState(nItem, 0, LVIS_DROPHILITED);
// 			m_nCurrentSelItem = nItem + 1;
// 			ShowPaperByItem(m_nCurrentSelItem);
// 		}
	}
	return 0;
}



void CModifyZkzhDlg::OnBnClickedBtnSave()
{
	UpdateData(TRUE);
	USES_CONVERSION;
	std::string strOldVal = m_pCurrentShowPaper->strSN;
	m_pCurrentShowPaper->strSN = T2A(m_strCurZkzh);
	CString strText = A2T(m_pCurrentShowPaper->strSN.c_str());

//	m_lcZkzh.SetItemState(m_nCurrentSelItem, 0, LVIS_DROPHILITED);
	for (int i = 0; i < m_lcZkzh.GetColumns(); i++)						//ȡ��������ʾ(�ֶ����ñ�����ɫ)
		if (!m_lcZkzh.GetModified(m_nCurrentSelItem, i))
			m_lcZkzh.SetItemColors(m_nCurrentSelItem, i, crOldText, crOldBackground);
		else
			m_lcZkzh.SetItemColors(m_nCurrentSelItem, i, RGB(255, 0, 0), crOldBackground);

	COLORREF crText, crBackground;
	m_lcZkzh.GetItemColors(m_nCurrentSelItem, 2, crText, crBackground);
	if (strOldVal != m_pCurrentShowPaper->strSN)
	{
		m_lcZkzh.SetItemText(m_nCurrentSelItem, 2, strText,
							 RGB(255, 0, 0), crBackground);
		m_lcZkzh.SetModified(m_nCurrentSelItem, 2, TRUE);
	}
	else
	{
		m_lcZkzh.SetItemText(m_nCurrentSelItem, 2, strText,
							 RGB(0, 0, 0), crBackground);
	}

	if (m_nCurrentSelItem < m_lcZkzh.GetColumns())
		m_nCurrentSelItem = m_nCurrentSelItem + 1;
	else
		m_nCurrentSelItem = 0;
	ShowPaperByItem(m_nCurrentSelItem);
}