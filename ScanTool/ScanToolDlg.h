
// ScanToolDlg.h : 头文件
//

#pragma once
#include "global.h"
#include "PicShow.h"
#include "MakeModelDlg.h"
#include "RecognizeThread.h"
#include "PaperInputDlg.h"
#include "TwainCpp.h"
#include "DIB.h"
#include "bmp2ipl.h"
#include "ScanCtrlDlg.h"
#include "PapersInfoSaveDlg.h"
#include "SendFileThread.h"
#include "TcpClient.h"
#include "ShowModelInfoDlg.h"
#include "ScanerInfoDlg.h"
#include "CompressThread.h"
#include "ScanThread.h"
#include "ModifyZkzhDlg.h"
#include "StudentMgr.h"
//#include "XListCtrl.h"
// CScanToolDlg 对话框

#define TIMER_CheckRecogComplete	200

class CScanToolDlg : public CDialogEx, public CTwain
{
// 构造
public:
	CScanToolDlg(pMODEL pModel, CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SCANTOOL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	CListCtrl		m_lcPicture;			//图片列表控件
	CXListCtrl		m_lProblemPaper;		//问题试卷列表控件
	CComboBox		m_comboModel;			//模板下拉列表控件
	CTabCtrl		m_tabPicShowCtrl;		//图片显示控件

	CStatusBar		m_wndStatusBar;
	CStatusBarCtrl	m_statusBar;

	int				m_nStatusSize;			//状态栏字体大小
	CFont			m_fontStatus;			//状态栏字体
	COLORREF		m_colorStatus;			//状态栏字体颜色

	CShowModelInfoDlg* m_pShowModelInfoDlg;
	CScanerInfoDlg*		m_pShowScannerInfoDlg;

	pMODEL			m_pModel;				//扫描试卷时的校验模板
	MODELLIST		m_lModel;				//已经加载的模板列表

	int				m_nCurrItemPaperList;	//当前试卷列表选中的项
	int				m_ncomboCurrentSel;		//下拉列表当前选择项


	Poco::Thread* m_pCompressThread;
	CCompressThread* m_pCompressObj;

	Poco::Thread*	m_pRecogThread;
	std::vector<CRecognizeThread*> m_vecRecogThreadObj;
	Poco::Thread*	m_SendFileThread;
	CSendFileThread* m_pSendFileObj;
	Poco::Thread*	m_TcpCmdThread;
	CTcpClient*		m_pTcpCmdObj;

	std::vector<CPicShow*>	m_vecPicShow;	//存储图片显示窗口指针，有多个模板图片时，对应到不同的tab控件页面
	int						m_nCurrTabSel;	//当前Tab控件选择的页面
	CPicShow*				m_pCurrentPicShow;		//当前图片显示控件
	int						m_nModelPicNums;		//模板图片数，即一份模板有多少图片，对应多少试卷

	int				m_nScanCount;			//已经扫描生成的图片数
	std::string		m_strCurrPicSavePath;	//当前扫描图像保存位置
	pPAPERSINFO		m_pPapersInfo;			//当前扫描的考场信息
	pST_PaperInfo	m_pPaper;				//当前扫描的学生信息
	pST_PaperInfo	m_pCurrentShowPaper;	//当前显示的学生信息 与m_pCurrentPicShow一起

	int				m_nDuplex;				//当前扫描是单面扫描(0)还是双面扫描(1)
	int				m_nScanStatus;			//当前扫描状态, 0-未扫描，1-扫描中，2-扫描异常，3-扫描结束

	std::string		m_strCmdServerIP;
	int				m_nCmdPort;

	int				m_nCurrentScanCount;	//当前扫描需要扫描试卷数量

	BOOL			m_bLogin;
	CString			m_strUserName;
	CString			m_strNickName;
	CString			m_strPwd;
	CString			m_strEzs;
	CString			m_strPersonId;			//天喻专用，登录后命名用personID
	int				m_nTeacherId;
	int				m_nUserId;
	BOOL			m_bF1Enable;
	BOOL			m_bF2Enable;
	BOOL			m_bLastPkgSaveOK;		//扫描的上一代试卷是否保存成功
	bool			m_bModifySN;			//扫描完成后，是否将未识别的准考证号进行人工修正
	int				m_nDoubleScan;			//是否是双面扫描	//单双面,0-单面,1-双面

	CStudentMgr*	m_pStudentMgr;			//报名库管理对象
public:
	void	InitUI();
	void	InitTab();
	void	InitConfig();
	void	InitCtrlPosition();
	void	InitParam();
	void	InitFileUpLoadList();
	void	InitCompressList();
	void	ShowPapers(pPAPERSINFO pPapers);

	void	InitShow(pMODEL pModel);
	void	InitScan();
	void	ReleaseScan();

	void	ReleaseUploadFileList();
	BOOL	StartGuardProcess();

	void	SetFontSize(int nSize);
	void	SetStatusShowInfo(CString strMsg, BOOL bWarn = FALSE);	//设置状态栏显示的消息

	void	SearchModel();
//	bool	UnZipModel(CString strZipPath);
//	pMODEL	LoadModelFile(CString strModelPath);			//加载模板文件

	void	ShowPaperByItem(int nItem, int nListCtrl = 0);	//nListCtrl = 0 -->从普通试卷列表显示图像，1-->从问题列表显示图像
	void	ShowRectByPoint(cv::Point pt, pST_PaperInfo pPaper);
	LRESULT RoiLBtnDown(WPARAM wParam, LPARAM lParam);		//鼠标左键按下的通知
	LRESULT MsgRecogErr(WPARAM wParam, LPARAM lParam);
	LRESULT MsgZkzhRecog(WPARAM wParam, LPARAM lParam);		//准考证号识别完成时的通知
	LRESULT MsgPkg2PapersOk(WPARAM wParam, LPARAM lParam);
	void	PaintRecognisedRect(pST_PaperInfo pPaper);
	int		PaintIssueRect(pST_PaperInfo pPaper);

	int		GetRectInfoByPoint(cv::Point pt, pST_PicInfo pPic, RECTINFO*& pRc);

	BOOL	ScanSrcInit();
// 	BOOL	InitTwain(HWND hWnd);
// 
// 	HWND	m_hMessageWnd;
// 	HINSTANCE m_hTwainDLL;
// 	TW_IDENTITY m_AppId;

	BOOL m_bTwainInit;

	CArray<TW_IDENTITY, TW_IDENTITY> m_scanSourceArry;
	void CopyImage(HANDLE hBitmap, TW_IMAGEINFO& info);
	void SetImage(HANDLE hBitmap, int bits);
	void ScanDone(int nStatus);

	int		CheckOrientation4Fix(cv::Mat& matSrc, int n);	//定点模式下的方向
	int		CheckOrientation4Head(cv::Mat& matSrc, int n);	//同步头模式下的方向
	int		CheckOrientation(cv::Mat& matSrc, int n, bool bDoubleScan);
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedBtnScan();
	afx_msg void OnBnClickedBtnScanmodule();
	afx_msg void OnCbnSelchangeComboModel();
	afx_msg void OnBnClickedBtnInputpaper();
	afx_msg void OnTcnSelchangeTabPicshow(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDestroy();
	afx_msg void OnNMDblclkListPicture(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedBtnUploadpapers();
	afx_msg void OnBnClickedBtnLogin();
	afx_msg void OnBnClickedBtnModelmgr();
	afx_msg void OnNMHoverListPicture(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownListPicture(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedBtnUploadmgr();
	afx_msg void OnBnClickedBtnScanall();
	afx_msg void OnClose();
	afx_msg void OnBnClickedBtnReback();
	afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnNMDblclkListPaper(NMHDR *pNMHDR, LRESULT *pResult);
};
