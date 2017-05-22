#pragma once
#include "global.h"
#include "CommonTWAIN.h"
#include "TwainApp.h"

#define OPENCV_TEST		//��ȡ��ͼ�����ݺ�ʹ��opencvд�ļ�

// CScanThread
#define MSG_START_SCAN	(WM_APP + 201)
#define MSG_SCAN_DONE	(WM_APP + 202)
#define MSG_SCAN_ERR	(WM_APP + 203)

typedef struct _tagScanCtrl_
{
	bool	bShowUI;		//�Ƿ���ʾԭʼTwain����
	int		nScannerId;
	int		nScanPixelType;	//0-�ڰף�1-�Ҷȣ�2-��ɫ
	int		nScanCount;
	int		nScanDuplexenable;	//��˫��ɨ��
	int		nScanSize;		//ֽ������
	int		nScanResolution;	//ɨ��DPI
	int		nAutoCut;	//�Զ��ü�
	int		nRotate;	//�Զ���ƫ

	int		nSaveIndex;	//�ļ�����ʱ����ʼ��������ӵ�1�ſ�ʼ����5�ſ�ʼ
	_tagScanCtrl_()
	{
		nScannerId = 0;
		nScanPixelType = 2;
		nScanCount = 0;
		nScanDuplexenable = 1;
		nScanSize = TWSS_A4LETTER;
		nScanResolution = 200;
		nAutoCut = 1;
		nRotate = 1;
		bShowUI = false;
		nSaveIndex = 1;
	}
}ST_SCANCTRL, *pST_SCANCTRL;

typedef struct _tagScanResult_
{
	bool		bScanOK;
	int			nState;
	IplImage*	pIpl2;
	std::string strResult;
	_tagScanResult_()
	{
		bScanOK = false;
		nState = 0;
		pIpl2 = NULL;
	}
	~_tagScanResult_()
	{
		if (pIpl2)
		{
			cvReleaseImage(&pIpl2);
			pIpl2 = NULL;
		}
	}
}ST_SCAN_RESULT, *pST_SCAN_RESULT;

class CScanThread : public CWinThread, public TwainApp
{
	DECLARE_DYNCREATE(CScanThread)

public:
	CScanThread();           // ��̬������ʹ�õ��ܱ����Ĺ��캯��
	virtual ~CScanThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void	StartScan(WPARAM wParam, LPARAM lParam);

	int		Scan();
	int		GetImgMemory();
	int		GetImgNative();
	void	setStop();
	void	setNotifyDlg(void* pDlg);
	void	setModelInfo(int nModelPicNums, std::string& strSavePath);
	void	resetData();
private:	
	bool	m_bStop;
	void*	m_pDlg;		//��Ϣ֪ͨ����
	int		m_nStartSaveIndex;		//�ļ�����ʱ����ʼ����
	int		m_nScanCount;
	int		m_nModelPicNums;
	std::string		m_strCurrPicSavePath;		//gb2312

	void* SaveFile(IplImage *pIpl);
	std::string ErrCode2Str(int nErr);	//�������ת�ַ���
protected:
	DECLARE_MESSAGE_MAP()

private:
};

