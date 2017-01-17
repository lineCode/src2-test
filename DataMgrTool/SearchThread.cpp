#include "stdafx.h"
#include "SearchThread.h"
#include "DataMgrTool.h"
#include "DataMgrToolDlg.h"


CSearchThread::CSearchThread(void* pDlg)
: m_pDlg(pDlg)
{
}


CSearchThread::~CSearchThread()
{
}

void CSearchThread::run()
{
//	_E_StartSearch_.wait();
	
	while (!g_nExitFlag)
	{
		pST_SEARCH pTask = NULL;
		_fmSearchPathList_.lock();
		L_SearchTask::iterator it = _SearchPathList_.begin();
		for (; it != _SearchPathList_.end();)
		{
			pTask = *it;
			it = _SearchPathList_.erase(it);
			break;
		}
		_fmSearchPathList_.unlock();
		if (NULL == pTask)
		{
			Poco::Thread::sleep(200);
			continue;
		}

		HandleTask(pTask->strSearchPath);

		if (_SearchPathList_.size() == 0)
		{
			USES_CONVERSION; char szCompare[10] = { 0 };
			sprintf_s(szCompare, "%d-%d", _pModel_->nExamID, _pModel_->nSubjectID);

			CString strMsg;
			strMsg.Format(_T("%s模板对应的试卷包识别完成\r\n"), A2T(szCompare));
			((CDataMgrToolDlg*)m_pDlg)->showMsg(strMsg);
		}

		delete pTask;
		pTask = NULL;
	}
	
}

void CSearchThread::HandleTask(std::string strPath)
{
	try
	{
//		std::string strPkgPath = CMyCodeConvert::Gb2312ToUtf8(_strPkgSearchPath_);
		std::string strDecompressDir = CMyCodeConvert::Utf8ToGb2312(strPath) + "\\tmpDecompress";

		char szCompare[10] = { 0 };
		sprintf_s(szCompare, "%d-%d", _pModel_->nExamID, _pModel_->nSubjectID);

		Poco::DirectoryIterator it(strPath);
		Poco::DirectoryIterator end;
		while (it != end)
		{
			Poco::Path p(it->path());
			std::string  strPath = CMyCodeConvert::Utf8ToGb2312(p.toString());
			std::string strFileName = p.getFileName();
			std::string strExtion = p.getExtension();
			if (it->isFile() && p.getExtension() == "pkg" && strFileName.find(szCompare) != std::string::npos)
			{
				pDECOMPRESSTASK pDecompressTask = new DECOMPRESSTASK;
				pDecompressTask->nTaskType = 3;
				pDecompressTask->strFilePath = CMyCodeConvert::Utf8ToGb2312(p.toString());
				pDecompressTask->strFileBaseName = CMyCodeConvert::Utf8ToGb2312(p.getBaseName());
				pDecompressTask->strSrcFileName = CMyCodeConvert::Utf8ToGb2312(p.getFileName());
				pDecompressTask->strDecompressDir = strDecompressDir;
				
				_fmDecompress_.lock();
				_nDecompress_++;
				_fmDecompress_.unlock();

				g_fmDecompressLock.lock();
				g_lDecompressTask.push_back(pDecompressTask);
				g_fmDecompressLock.unlock();
			}
			else if (it->isDirectory())
			{
				_fmSearchPathList_.lock();
				pST_SEARCH pTask = new ST_SEARCH;
				pTask->strSearchPath = p.toString();
				_SearchPathList_.push_back(pTask);
				_fmSearchPathList_.unlock();
			}
			it++;
		}
	}
	catch (Poco::Exception& exc)
	{
	}

}
