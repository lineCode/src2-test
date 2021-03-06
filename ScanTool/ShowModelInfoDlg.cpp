// ShowModelInfoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ScanTool.h"
#include "ShowModelInfoDlg.h"
#include "afxdialogex.h"


// CShowModelInfoDlg 对话框

IMPLEMENT_DYNAMIC(CShowModelInfoDlg, CDialog)

CShowModelInfoDlg::CShowModelInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CShowModelInfoDlg::IDD, pParent)
	, m_nNameSize(26), m_nDesSize(24), m_strModelName(_T("")), m_strModelDesc(_T(""))
{
	EnableToolTips(TRUE);
	m_toolTip.Create(this);
	m_toolTip.SetMaxTipWidth(500);
	m_toolTip.Activate(TRUE);
}

CShowModelInfoDlg::~CShowModelInfoDlg()
{
}

void CShowModelInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_ModelName, m_strModelName);
	DDX_Text(pDX, IDC_EDIT_ModelDesc, m_strModelDesc);
}


BEGIN_MESSAGE_MAP(CShowModelInfoDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void CShowModelInfoDlg::InitUI()
{
	SetFontSize(m_nNameSize, m_nDesSize);

	InitCtrlPosition();
}

void CShowModelInfoDlg::InitCtrlPosition()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.right;
	int cy = rcClient.bottom;

	const int nLeftGap = 2;		//左边的空白间隔
	const int nBottomGap = 2;	//下边的空白间隔
	const int nRightGap = 2;	//右边的空白间隔
	const int nTopGap = 2;
	const int nGap = 2;			//普通控件的间隔

	int nCurrentTop = 0;
	int nStaticW = 40;
	int nStaticH = 20;
	if (GetDlgItem(IDC_STATIC_ModelTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_ModelTips)->MoveWindow(nLeftGap, nTopGap, nStaticW, nStaticH);
	}
	if (GetDlgItem(IDC_STATIC_ModelName)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_ModelName)->MoveWindow(nLeftGap + nGap + nStaticW, nTopGap, cx - nLeftGap - nStaticW - nGap, nStaticH + 5);
		nCurrentTop = nTopGap + nStaticH + nGap;
	}
	if (GetDlgItem(IDC_STATIC_DescTips)->GetSafeHwnd())
	{
		GetDlgItem(IDC_STATIC_DescTips)->MoveWindow(nLeftGap, nCurrentTop, nStaticW, nStaticH);
		nCurrentTop = nCurrentTop + nStaticH + nGap;
	}
	int nDescH = cy - nCurrentTop - nBottomGap;
	if (GetDlgItem(IDC_EDIT_ModelDesc)->GetSafeHwnd())
	{
		GetDlgItem(IDC_EDIT_ModelDesc)->MoveWindow(nLeftGap, nCurrentTop, cx - nLeftGap - nRightGap, nDescH);
	}
}


// CShowModelInfoDlg 消息处理程序

BOOL CShowModelInfoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	InitUI();
	m_colorStatus = RGB(50, 20, 255);
	return TRUE;
}

void CShowModelInfoDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	InitCtrlPosition();
//	Invalidate();
}

void CShowModelInfoDlg::SetFontSize(int nNameSize, int nDesSize)
{
	m_fontStatus_Name.DeleteObject();
	m_fontStatus_Name.CreateFont(nNameSize, 0, 0, 0,
							FW_BOLD, FALSE, FALSE, 0,
							DEFAULT_CHARSET,
							OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,
							DEFAULT_QUALITY,
							DEFAULT_PITCH | FF_SWISS,
							_T("Arial"));
	m_fontStatus_Des.DeleteObject();
	m_fontStatus_Des.CreateFont(nDesSize, 0, 0, 0,
								 FW_BOLD, FALSE, FALSE, 0,
								 DEFAULT_CHARSET,
								 OUT_DEFAULT_PRECIS,
								 CLIP_DEFAULT_PRECIS,
								 DEFAULT_QUALITY,
								 DEFAULT_PITCH | FF_SWISS,
								 _T("Arial"));

	GetDlgItem(IDC_STATIC_ModelName)->SetFont(&m_fontStatus_Name);
	GetDlgItem(IDC_EDIT_ModelDesc)->SetFont(&m_fontStatus_Des);
}

HBRUSH CShowModelInfoDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	UINT CurID = pWnd->GetDlgCtrlID();

	if (IDC_STATIC_ModelName == CurID)
	{
		pDC->SetTextColor(m_colorStatus);

		return hbr;	// hbrsh;
	}
	if (IDC_EDIT_ModelDesc == CurID)
	{
		pDC->SetTextColor(m_colorStatus);

		return hbr;	// hbrsh;
	}
	return hbr;
}

void CShowModelInfoDlg::ShowModelInfo(pMODEL pModel, int nFlag /*= 0*/)
{
	if (!pModel)
	{
		m_strModelName = _T("");
		m_strModelDesc = _T("");
		UpdateData(FALSE);
		return;
	}
	
	USES_CONVERSION;
	m_strModelName = A2T(pModel->strModelName.c_str());
	m_strModelDesc = A2T(pModel->strModelDesc.c_str());


	m_toolTip.AddTool(GetDlgItem(IDC_STATIC_ModelName), m_strModelName);

	if (nFlag > 0)
	{
		USES_CONVERSION;
		CString strExamInfo = _T("");
		
		char szPicNum[20] = { 0 };
		sprintf_s(szPicNum, "模板页数: %d\r\n", pModel->nPicNum);

		char szHeadVal[10] = { 0 };
		if (pModel->nHasHead)
			strcpy_s(szHeadVal, "有");
		else
			strcpy_s(szHeadVal, "无");
		char szHead[30] = { 0 };
		sprintf_s(szHead, "同步头: %s\r\n", szHeadVal);

		char szExamInfo[30] = { 0 };
		sprintf_s(szExamInfo, "考试ID: %d\r\n科目ID: %d\r\n", pModel->nExamID, pModel->nSubjectID);
		strExamInfo = szPicNum;

		char szElectOmrVal[10] = { 0 };
		if (pModel->nHasElectOmr)
			strcpy_s(szElectOmrVal, "有");
		else
			strcpy_s(szElectOmrVal, "无");
		char szElectOmr[30] = { 0 };
		sprintf_s(szElectOmr, "选做题: %s\r\n", szElectOmrVal);

		char szPaperType[30] = { 0 };
		if (pModel->nScanSize == 1)
			sprintf_s(szPaperType, "纸张类型: A4\r\n");
		else if (pModel->nScanSize == 2)
			sprintf_s(szPaperType, "纸张类型: A3\r\n");
		else
			sprintf_s(szPaperType, "纸张类型: 定制类型\r\n");
		char szScanType[30] = { 0 };
		if (pModel->nScanType == 1)
			sprintf_s(szScanType, "扫描类型: 灰度扫描\r\n");
		else
			sprintf_s(szScanType, "扫描类型: 彩色扫描\r\n");
		char szScanDpi[30] = { 0 };
		sprintf_s(szScanDpi, "扫描分辨率: %ddpi\r\n", pModel->nScanDpi);

		strExamInfo.Append(A2T(szHead));
		strExamInfo.Append(A2T(szExamInfo));
		strExamInfo.Append(A2T(szElectOmr));
		strExamInfo.Append(A2T("\r\n"));
		strExamInfo.Append(A2T(szPaperType));
		strExamInfo.Append(A2T(szScanType));
		strExamInfo.Append(A2T(szScanDpi));
		m_strModelDesc.Append(A2T("\r\n"));
		m_strModelDesc.Append(strExamInfo);
	}

	UpdateData(FALSE);
}

BOOL CShowModelInfoDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
		case WM_MOUSEMOVE:
			m_toolTip.RelayEvent(pMsg);
			break;
		case WM_KEYDOWN:
			{
				if (pMsg->wParam == VK_ESCAPE)
				{
					return TRUE;
				}
				if (pMsg->wParam == VK_RETURN)
				{
					return TRUE;
				}
				return TRUE;
			}
			break;
	}
	return CDialog::PreTranslateMessage(pMsg);
}
