
// ScanTool2.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CScanTool2App: 
// �йش����ʵ�֣������ ScanTool2.cpp
//

class CScanTool3App : public CWinApp
{
public:
	CScanTool3App();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CScanTool3App theApp;