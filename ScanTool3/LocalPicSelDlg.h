#pragma once
#include <vector>
#include "TipBaseDlg.h"
#include "ModelInfoDlg.h"
#include "BmpButton.h"

// CLocalPicSelDlg �Ի���

class CLocalPicSelDlg : public CTipBaseDlg
{
	DECLARE_DYNAMIC(CLocalPicSelDlg)

public:
	CLocalPicSelDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CLocalPicSelDlg();

// �Ի�������
	enum { IDD = IDD_LOCALPICSELDLG };

	std::vector<MODELPICPATH> m_vecPath;

	CBmpButton		m_bmpBtnClose;
	CListCtrl	m_listPath;
private:
	int			m_nCurrentItem;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMClickListPic(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedBtnDel();
	afx_msg void OnBnClickedBtnOk();
	afx_msg void OnBnClickedBtnClose();
};