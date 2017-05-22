// ScanReminderDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ScanTool3.h"
#include "ScanReminderDlg.h"
#include "afxdialogex.h"


// CScanReminderDlg �Ի���

IMPLEMENT_DYNAMIC(CScanReminderDlg, CDialog)

CScanReminderDlg::CScanReminderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScanReminderDlg::IDD, pParent)
	, m_nStatusSize(25)
{

}

CScanReminderDlg::~CScanReminderDlg()
{
}

void CScanReminderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BOOL CScanReminderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	InitCtrlPosition();
	SetFontSize(m_nStatusSize);

	return TRUE;
}

BOOL CScanReminderDlg::PreTranslateMessage(MSG* pMsg)
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

BEGIN_MESSAGE_MAP(CScanReminderDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CScanReminderDlg ��Ϣ��������
void CScanReminderDlg::InitCtrlPosition()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	int cx = rcClient.right;
	int cy = rcClient.bottom;

	const int nTopGap = 20;	//�ϱߵļ��
	const int nBottomGap = 10;	//�±ߵļ��
	const int nLeftGap = 20;		//��ߵĿհ׼��
	const int nRightGap = 20;	//�ұߵĿհ׼��
	int nGap = 5;

	int nBaseLeft = nLeftGap + (cx - nLeftGap - nRightGap) * 0.4;
	int nCurrLeft = nBaseLeft;
	int nCurrTop = cy * 0.5;

	if (GetDlgItem(IDC_STATIC_Tip)->GetSafeHwnd())
	{
		int nW = (cx - nLeftGap - nRightGap) * 0.4;
		int nH = 50;
		GetDlgItem(IDC_STATIC_Tip)->MoveWindow(nCurrLeft, nCurrTop, nW, nH);
	}
	Invalidate();
}

void CScanReminderDlg::SetFontSize(int nSize)
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
	GetDlgItem(IDC_STATIC_Tip)->SetFont(&m_fontStatus);
}

void CScanReminderDlg::DrawBorder(CDC *pDC)
{
	CPen *pOldPen = NULL;
	CPen pPen;
	CRect rcClient(0, 0, 0, 0);
	GetClientRect(&rcClient);
	pPen.CreatePen(PS_SOLID, 2, RGB(0, 0, 0));

	pDC->SelectStockObject(NULL_BRUSH);
	pOldPen = pDC->SelectObject(&pPen);
	pDC->Rectangle(&rcClient);
	pDC->SelectObject(pOldPen);
	pPen.Detach();
}

BOOL CScanReminderDlg::OnEraseBkgnd(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(&rcClient);

	pDC->FillRect(rcClient, &CBrush(RGB(255, 255, 255)));	//225, 222, 250
	DrawBorder(pDC);
	ReleaseDC(pDC);

	return CDialog::OnEraseBkgnd(pDC);
}


HBRUSH CScanReminderDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	UINT CurID = pWnd->GetDlgCtrlID();
	if (CurID == IDC_STATIC_Tip)
	{
		//		pDC->SetBkColor(RGB(255, 255, 255));
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}
	return hbr;
}


void CScanReminderDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	InitCtrlPosition();
}