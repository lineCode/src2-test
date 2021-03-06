#include "stdafx.h"
#include "RecognizeThread.h"
#include "DataMgrTool.h"
#include "DataMgrToolDlg.h"

using namespace cv;
CRecognizeThread::CRecognizeThread()
{
	m_nContract = 100;
	m_nBright = 0;
}

CRecognizeThread::~CRecognizeThread()
{
	if (m_vecContours.size())
		m_vecContours.clear();
	g_Log.LogOut("RecognizeThread exit.");
	TRACE("RecognizeThread exit1.\n");
}

void CRecognizeThread::run()
{
	g_Log.LogOut("RecognizeThread start...");
	TRACE("RecognizeThread start...\n");
	eExit.reset();

	while (!g_nExitFlag)
	{
		pRECOGTASK pTask = NULL;
		g_fmRecog.lock();
		RECOGTASKLIST::iterator it = g_lRecogTask.begin();
		for (; it != g_lRecogTask.end();)
		{
			pTask = *it;
			it = g_lRecogTask.erase(it);
			break;
		}
		g_fmRecog.unlock();
		if (NULL == pTask)
		{
			Poco::Thread::sleep(500);
			continue;
		}

		HandleTask(pTask);

		_fmRecog_.lock();
		_nRecog_++;
		_fmRecog_.unlock();

		delete pTask;
		pTask = NULL;
	}

	_mapModelLock.lock();
	std::map<pMODEL, pMODELINFO>::iterator it = _mapModel.begin();
	for (; it != _mapModel.end();)
	{
		pMODELINFO pModelInfo = NULL;
		pModelInfo = it->second;
		if (pModelInfo)
		{
			delete pModelInfo;
			pModelInfo = NULL;
		}
		it = _mapModel.erase(it);
	}
	_mapModelLock.unlock();

	eExit.set();
	TRACE("RecognizeThread exit 0\n");
}

bool CRecognizeThread::HandleTask(pRECOGTASK pTask)
{
	//进行任务识别
	pMODELINFO pModelInfo = NULL;
	_mapModelLock.lock();
	std::map<pMODEL, pMODELINFO>::iterator it = _mapModel.find(pTask->pPaper->pModel);		//pTask->pModel
	if (it == _mapModel.end())
	{
		pModelInfo = new MODELINFO;
		_mapModel.insert(std::map<pMODEL, pMODELINFO>::value_type(pTask->pPaper->pModel, pModelInfo));	//pTask->pModel

		std::string strLog = "加载模板";
		g_Log.LogOut(strLog);

		pModelInfo->pModel = pTask->pPaper->pModel;		//pTask->pModel;
		bool bResult = LoadModel(pModelInfo);
		if (!bResult)
		{
			_mapModelLock.unlock();
			return bResult;
		}
	}
	else
		pModelInfo = it->second;
	_mapModelLock.unlock();

	PaperRecognise(pTask->pPaper, pModelInfo);

	return true;
}

bool CRecognizeThread::LoadModel(pMODELINFO pModelInfo)
{
	USES_CONVERSION;
#if 0
	for (int i = 0; i < pModelInfo->pModel->nPicNum; i++)
	{
		std::string strModelPicPath = g_strModelSavePath + "\\" + pModelInfo->pModel->strModelName + "\\" + pModelInfo->pModel->vecPaperModel[i]->strModelPicName;

		cv::Mat matSrc;
		try
		{
			matSrc = cv::imread(strModelPicPath);
		}
		catch (cv::Exception& exc)
		{
			std::string strLog = "打开文件失败: " + exc.msg;
			g_Log.LogOut(strLog);
			return false;
		}
	#ifdef PIC_RECTIFY_TEST
		Mat dst;
		Mat rotMat;
		PicRectify(matSrc, dst, rotMat);
		Mat matImg;
		if (dst.channels() == 1)
			cvtColor(dst, matImg, CV_GRAY2BGR);
		else
			matImg = dst;
	#else
		Mat matImg = matSrc;
	#endif
		pModelInfo->vecMatSrc.push_back(matImg);
	}
#endif
	return true;
}

void CRecognizeThread::PaperRecognise(pST_PaperInfo pPaper, pMODELINFO pModelInfo)
{
	std::string strRecogLog;
	PIC_LIST::iterator itPic = pPaper->lPic.begin();
	for (int i = 0; itPic != pPaper->lPic.end(); itPic++, i++)
	{
		clock_t start_pic, end_pic;
		start_pic = clock();

		if ((*itPic)->bRecoged)		//已经识别过，不再识别
			continue;

		(*itPic)->bRecoged = true;

		if (i >= pModelInfo->pModel->vecPaperModel.size())
			continue;

		int nCount = pModelInfo->pModel->vecPaperModel[i]->lH_Head.size() + pModelInfo->pModel->vecPaperModel[i]->lV_Head.size() + pModelInfo->pModel->vecPaperModel[i]->lABModel.size()
			+ pModelInfo->pModel->vecPaperModel[i]->lCourse.size() + pModelInfo->pModel->vecPaperModel[i]->lQK_CP.size() + pModelInfo->pModel->vecPaperModel[i]->lWJ_CP.size() + pModelInfo->pModel->vecPaperModel[i]->lGray.size()
			+ pModelInfo->pModel->vecPaperModel[i]->lWhite.size() + pModelInfo->pModel->vecPaperModel[i]->lSNInfo.size() + pModelInfo->pModel->vecPaperModel[i]->lOMR2.size()
			+ pModelInfo->pModel->vecPaperModel[i]->lElectOmr.size();
		if (!nCount)	//如果当前模板试卷没有校验点就不需要进行试卷打开操作，直接下一张试卷
			continue;

		TRACE("start recog: %s\n", (*itPic)->strPicName.c_str());

		std::string strPicFileName = (*itPic)->strPicName;
		Mat matCompSrcPic;
		bool bOpenSucc = false;
		for (int i = 0; i < 3; i++)
		{
			if (!bOpenSucc)
			{
				try
				{
					matCompSrcPic = imread((*itPic)->strPicPath);			//imread((*itPic)->strPicPath);
					bOpenSucc = true;
					break;
				}
				catch (cv::Exception& exc)
				{
					Sleep(500);
				}
			}
		}
		if (!bOpenSucc)
		{
			std::string strLog = "几次打开文件都失败2: " + (*itPic)->strPicPath;
			g_Log.LogOut(strLog);
			continue;
		}
		
#ifdef PIC_RECTIFY_TEST	//图像旋转纠正测试
		Mat matDst;
		Mat matCompPic;
		Mat rotMat;
		PicRectify(matCompSrcPic, matDst, rotMat);
		if (matDst.channels() == 1)
			cvtColor(matDst, matCompPic, CV_GRAY2BGR);
		else
			matCompPic = matDst;
#else
		Mat matCompPic = matCompSrcPic;
#endif

		clock_t end1_pic = clock();

		bool bFind = false;
		int nPic = i;
		
		if (g_nRecogWithContract)	//恢复默认值
		{
			m_nContract = 100;
			m_nBright = 0;
		}
		
		std::string strChkPicRotationLog, strRecogFixLog, strRecogCourseLog, strRecogQKLog, strRecogWJLog, strRecogGrayLog, strRecogWhiteLog, strRecogSNLog, strRecogOmrLog, strRecogElectOmrLog;
		clock_t sTimeTmp, eTimeTmp;
		int nTime[15] = { 0 };		//每步操作耗时
		sTimeTmp = clock();

		pPAPERSINFO pCurrentPapers = static_cast<pPAPERSINFO>(pPaper->pPapers);
		if (g_nRecogMode == 1)		//pCurrentPapers->nRecogMode == 1
		{
			if (g_nRecogChkRotation)
			{
				ChkPicRotation(nPic, matCompPic, *itPic, pModelInfo, strChkPicRotationLog);
				eTimeTmp = clock();
				nTime[0] = eTimeTmp - sTimeTmp;
				sTimeTmp = clock();
			}
			bool bResult = RecogFixCP(nPic, matCompPic, *itPic, pModelInfo, g_nRecogMode, strRecogFixLog);
			eTimeTmp = clock();
			nTime[1] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
		#ifdef WarpAffine_TEST
			cv::Mat	inverseMat(2, 3, CV_32FC1);
			bResult = PicTransfer(nPic, matCompPic, (*itPic)->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, inverseMat);
			eTimeTmp = clock();
			nTime[2] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
		#endif
			bResult = RecogHHead(nPic, matCompPic, *itPic, pModelInfo);
			eTimeTmp = clock();
			nTime[3] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogVHead(nPic, matCompPic, *itPic, pModelInfo);
			eTimeTmp = clock();
			nTime[4] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogABModel(nPic, matCompPic, *itPic, pModelInfo);
			eTimeTmp = clock();
			nTime[5] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogCourse(nPic, matCompPic, *itPic, pModelInfo, g_nRecogMode, strRecogCourseLog);
			eTimeTmp = clock();
			nTime[6] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogQKCP(nPic, matCompPic, *itPic, pModelInfo, g_nRecogMode, strRecogQKLog);
			eTimeTmp = clock();
			nTime[7] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogWJCP(nPic, matCompPic, *itPic, pModelInfo, g_nRecogMode, strRecogWJLog);
			eTimeTmp = clock();
			nTime[8] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogGrayCP(nPic, matCompPic, *itPic, pModelInfo, g_nRecogMode, strRecogGrayLog);
			eTimeTmp = clock();
			nTime[9] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogWhiteCP(nPic, matCompPic, *itPic, pModelInfo, g_nRecogMode, strRecogWhiteLog);
			eTimeTmp = clock();
			nTime[10] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogSN(nPic, matCompPic, *itPic, pModelInfo, strRecogSNLog);
			eTimeTmp = clock();
			nTime[11] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogOMR(nPic, matCompPic, *itPic, pModelInfo, strRecogOmrLog);
			eTimeTmp = clock();
			nTime[12] = eTimeTmp - sTimeTmp;
			sTimeTmp = clock();
			bResult = RecogElectOmr(nPic, matCompPic, *itPic, pModelInfo, strRecogElectOmrLog);
			eTimeTmp = clock();
			nTime[13] = eTimeTmp - sTimeTmp;
		}
		else
		{
			if (g_nRecogChkRotation)
				ChkPicRotation(nPic, matCompPic, *itPic, pModelInfo, strChkPicRotationLog);
			bool bResult = RecogFixCP(nPic, matCompPic, *itPic, pModelInfo, 2, strRecogFixLog);
		#ifdef WarpAffine_TEST
			cv::Mat	inverseMat(2, 3, CV_32FC1);
			if (bResult) bResult = PicTransfer(nPic, matCompPic, (*itPic)->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, inverseMat);
		#endif
			if (bResult) bResult = RecogHHead(nPic, matCompPic, *itPic, pModelInfo);
			if (bResult) bResult = RecogVHead(nPic, matCompPic, *itPic, pModelInfo);
			if (bResult) bResult = RecogABModel(nPic, matCompPic, *itPic, pModelInfo);
			if (bResult) bResult = RecogCourse(nPic, matCompPic, *itPic, pModelInfo, 2, strRecogCourseLog);
			if (bResult) bResult = RecogQKCP(nPic, matCompPic, *itPic, pModelInfo, 2, strRecogQKLog);
			if (bResult) bResult = RecogWJCP(nPic, matCompPic, *itPic, pModelInfo, 2, strRecogWJLog);
			if (bResult) bResult = RecogGrayCP(nPic, matCompPic, *itPic, pModelInfo, 2, strRecogGrayLog);
			if (bResult) bResult = RecogWhiteCP(nPic, matCompPic, *itPic, pModelInfo, 2, strRecogWhiteLog);
			if (bResult) bResult = RecogSN(nPic, matCompPic, *itPic, pModelInfo, strRecogSNLog);
			if (bResult) bResult = RecogOMR(nPic, matCompPic, *itPic, pModelInfo, strRecogOmrLog);
			if (bResult) bResult = RecogElectOmr(nPic, matCompPic, *itPic, pModelInfo, strRecogElectOmrLog);
			if (!bResult) bFind = true;
			if (bFind)
			{
				HandleWithErrPaper(pPaper);
				break;									//找到这张试卷有问题点，不进行下一张试卷的检测
			}
		}
		
		end_pic = clock();
		TRACE("试卷 %s 打开时间: %d, 识别总时间: %d\n", strPicFileName.c_str(), end1_pic - start_pic, end_pic - start_pic);

		std::string strTimeTmp;
		for (int i = 0; i < 14; i++)
			strTimeTmp.append(Poco::format("%d:", nTime[i]));

		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "试卷 %s 打开时间: %d, time2 = %s, 识别总时间: %d.", strPicFileName.c_str(), end1_pic - start_pic, strTimeTmp.c_str(), end_pic - start_pic);
		strRecogLog.append(strPicFileName + "详细信息: ");
		strRecogLog.append(szLog);

		//strRecogLog.append(strRecogFixLog + "\n" + strRecogCourseLog + "\n" + strRecogQKLog + "\n" + strRecogWJLog + "\n" + strRecogGrayLog + "\n" + strRecogWhiteLog + "\n" + strRecogSNLog + "\n" + strRecogOmrLog + "\n" + strRecogElectOmrLog);
		
		strRecogLog.append("\n" + strChkPicRotationLog);
		if (strRecogFixLog != "")
			strRecogLog.append("\n" + strRecogFixLog);
		if (strRecogCourseLog != "")
			strRecogLog.append("\n" + strRecogCourseLog);
		if (strRecogQKLog != "")
			strRecogLog.append("\n" + strRecogQKLog);
		if (strRecogWJLog != "")
			strRecogLog.append("\n" + strRecogWJLog);
		if (strRecogGrayLog != "")
			strRecogLog.append("\n" + strRecogGrayLog);
		if (strRecogWhiteLog != "")
			strRecogLog.append("\n" + strRecogWhiteLog);
		strRecogLog.append("\n" + strRecogSNLog + "\n" + strRecogOmrLog);
		if (strRecogElectOmrLog != "")
			strRecogLog.append("\n" + strRecogElectOmrLog);
	}
	g_Log.LogOut(strRecogLog);

	pPAPERSINFO pPapers = static_cast<pPAPERSINFO>(pPaper->pPapers);
#if 1	//test log
	if (g_nFastRecogModel != 1)
	{
		std::string strPaperLog = "试卷(" + pPaper->strStudentInfo + ")[" + pPapers->strPapersName + "]识别结果: \n";

		int nNullCount = 0;
		int nDoubtCount = 0;
		int nEqualCount = 0;

		char szSN[100] = { 0 };
		sprintf_s(szSN, "SN(%s), ", pPaper->strSN.c_str());
		strPaperLog.append(szSN);

		OMRRESULTLIST::iterator itOmr = pPaper->lOmrResult.begin();
		for (; itOmr != pPaper->lOmrResult.end(); itOmr++)
		{
			//数量统计
			if (itOmr->nDoubt == 0)
				nEqualCount++;
			else if (itOmr->nDoubt == 1)
				nDoubtCount++;
			else
				nNullCount++;

			char szSingle[10] = { 0 };
			if (itOmr->nSingle == 0)
				strcpy_s(szSingle, "单");
			else
				strcpy_s(szSingle, "多");

			int nPrintOmrVal = 0;
		#ifdef PrintAllOmrVal
			nPrintOmrVal = 1;
		#else
			nPrintOmrVal = itOmr->nDoubt;
		#endif
			//		char szItemInfo[2000] = { 0 };
			std::string strItemLog;
			if (nPrintOmrVal)	//itOmr->nDoubt
			{
				RECTLIST::iterator itRect = itOmr->lSelAnswer.begin();
				for (; itRect != itOmr->lSelAnswer.end(); itRect++)
				{
					char szTmp[100] = { 0 };
					sprintf_s(szTmp, "%c,识别=%.3f(R/S=%.1f/%.1f),Succ:%d. ", itRect->nAnswer + 65, \
							  itRect->fRealValuePercent, itRect->fRealValue, itRect->fStandardValue, itRect->fRealValuePercent > itRect->fStandardValuePercent);
					// 				sprintf_s(szTmp, "选项=%c, 识别实际比例=%.3f, val=%.2f, 识别标准val=%.2f, 是否成功:%d\t", itRect->nAnswer + 65, \
					// 					itRect->fRealValuePercent, itRect->fRealValue, itRect->fStandardValue, itRect->fRealValuePercent > itRect->fStandardValuePercent);
					strItemLog.append(szTmp);
				}
				strItemLog.append("\n");

				RECTLIST::iterator itRect2 = itOmr->lSelAnswer.begin();
				for (; itRect2 != itOmr->lSelAnswer.end(); itRect2++)
				{
					char szTmp[200] = { 0 };
					sprintf_s(szTmp, "%c,密度=%.3f(%.3f/%.3f), ", itRect2->nAnswer + 65, \
							  itRect2->fRealDensity / itRect2->fStandardDensity, itRect2->fRealDensity, itRect2->fStandardDensity);
					strItemLog.append(szTmp);
				}
				strItemLog.append("\n");
				RECTLIST::iterator itRect3 = itOmr->lSelAnswer.begin();
				for (; itRect3 != itOmr->lSelAnswer.end(); itRect3++)
				{
					char szTmp[200] = { 0 };
					sprintf_s(szTmp, "%c,灰度=%.3f(%.3f-%.3f), ", itRect3->nAnswer + 65, \
							  itRect3->fRealMeanGray - itRect3->fStandardMeanGray, itRect3->fRealMeanGray, itRect3->fStandardMeanGray);
					strItemLog.append(szTmp);
				}
				strItemLog.append("\n");
				RECTLIST::iterator itRect4 = itOmr->lSelAnswer.begin();
				for (; itRect4 != itOmr->lSelAnswer.end(); itRect4++)
				{
					char szTmp[200] = { 0 };
					sprintf_s(szTmp, "%c,标准差=%.3f(%.3f-%.3f), ", itRect4->nAnswer + 65, \
							  itRect4->fRealStddev - itRect4->fStandardStddev, itRect4->fRealStddev, itRect4->fStandardStddev);
					strItemLog.append(szTmp);
				}

				char szGrayTmp[200] = { 0 };
				sprintf_s(szGrayTmp, "\n空白灰度差=%.3f(%.3f-%.3f)", itOmr->fWhiteAreaGray - itOmr->fWhiteAreaGrayModel, itOmr->fWhiteAreaGray, itOmr->fWhiteAreaGrayModel);
				strItemLog.append(szGrayTmp);
			}

			//++++++++	test	++++++++
			std::vector<pRECTINFO> vecItemsDensityDesc;
			std::vector<ST_ITEM_DIFF> vecOmrItemDensityDiff;
			calcOmrDensityDiffVal(itOmr->lSelAnswer, vecItemsDensityDesc, vecOmrItemDensityDiff);
			strItemLog.append("\n[");
			for (int i = 0; i < vecOmrItemDensityDiff.size(); i++)
			{
				char szTmp[40] = { 0 };
				sprintf_s(szTmp, "%s:%.5f ", vecOmrItemDensityDiff[i].szVal, vecOmrItemDensityDiff[i].fDiff);
				strItemLog.append(szTmp);
			}
			strItemLog.append("]");

	#ifdef Test_RecogFirst_NoThreshord
			float fDensityThreshold = 0.0;
			strItemLog.append("密度选中阀值:[");
			for (int i = 0; i < vecOmrItemDensityDiff.size(); i++)
			{
				char szTmp[40] = { 0 };
				sprintf_s(szTmp, "%s:%.5f ", vecOmrItemDensityDiff[i].szVal, _dDiffThread_Fix_ + fDensityThreshold * 0.5);
				strItemLog.append(szTmp);
				//			if ((vecOmrItemDensityDiff[i].fDiff >= _dDiffThread_Fix_ + fDensityThreshold * 0.5))
				fDensityThreshold += vecOmrItemDensityDiff[i].fDiff;
			}
			strItemLog.append("]");

			//test日志
			float fDensityMeanPer = 0.0;
			float fDensityMeanPer2 = 0.0;
			for (int i = 0; i < vecItemsDensityDesc.size(); i++)
			{
				fDensityMeanPer += vecItemsDensityDesc[i]->fRealValuePercent;
			}
			fDensityMeanPer = fDensityMeanPer / vecItemsDensityDesc.size();

			for (int i = 0; i < vecOmrItemDensityDiff.size(); i++)
				fDensityMeanPer2 += vecOmrItemDensityDiff[i].fDiff;
			fDensityMeanPer2 = fDensityMeanPer2 / vecOmrItemDensityDiff.size();

			char szTmp2[100] = { 0 };
			sprintf_s(szTmp2, "密度平均值:%.3f, 密度差平均值值:%.3f, ", fDensityMeanPer, fDensityMeanPer2);
			strItemLog.append(szTmp2);

			float fDensityThreshold2 = 0.0;
			strItemLog.append("\n[");
			for (int i = 0; i < vecOmrItemDensityDiff.size(); i++)
			{
				char szTmp[40] = { 0 };
				float fGrayThresholdGray = fDensityMeanPer2 - vecOmrItemDensityDiff[i].fDiff;
				float fGrayMean1 = vecItemsDensityDesc[i]->fRealValuePercent - fDensityMeanPer;
				float fGrayMean12 = -(vecItemsDensityDesc[i + 1]->fRealValuePercent - fDensityMeanPer);
				sprintf_s(szTmp, "%s:%.5f ", vecOmrItemDensityDiff[i].szVal, _dDiffThread_Fix_ + vecOmrItemDensityDiff[i].fDiff * 0.5 + fGrayThresholdGray * 0.5 + fDensityThreshold2);	//_dDiffThread_Fix_ + fGrayThresholdGray + fDensityThreshold2
				strItemLog.append(szTmp);

				fDensityThreshold2 += (_dDiffThread_Fix_ + vecOmrItemDensityDiff[i].fDiff * 0.5 + fGrayThresholdGray * 0.5 + fDensityThreshold2) / 2;	//_dDiffThread_Fix_ + fGrayThresholdGray + fDensityThreshold2
			}
			strItemLog.append("]\n");

			strItemLog.append("与密度平均值差值:[");
			for (int i = 0; i < vecItemsDensityDesc.size(); i++)
			{
				char szTmp[40] = { 0 };
				sprintf_s(szTmp, "%c:%.5f ", vecItemsDensityDesc[i]->nAnswer + 65, vecItemsDensityDesc[i]->fRealValuePercent - fDensityMeanPer);
				strItemLog.append(szTmp);
			}
			strItemLog.append("]");
	#endif

			std::vector<pRECTINFO> vecItemsGrayDesc;
			std::vector<ST_ITEM_DIFF> vecOmrItemGrayDiff;
			calcOmrGrayDiffVal(itOmr->lSelAnswer, vecItemsGrayDesc, vecOmrItemGrayDiff);

			float fMeanGrayDiff = 0.0;
			strItemLog.append("\n[");
			for (int i = 0; i < vecOmrItemGrayDiff.size(); i++)
			{
				char szTmp[40] = { 0 };
				sprintf_s(szTmp, "%s:%.3f ", vecOmrItemGrayDiff[i].szVal, vecOmrItemGrayDiff[i].fDiff);
				strItemLog.append(szTmp);

				fMeanGrayDiff += vecOmrItemGrayDiff[i].fDiff;
			}
			strItemLog.append("]");
			fMeanGrayDiff = fMeanGrayDiff / vecOmrItemGrayDiff.size();

			float fMeanModelGrayDiff = 0.0;
			for (int i = 0; i < vecItemsGrayDesc.size(); i++)
			{
				fMeanModelGrayDiff += (vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray);
			}
			fMeanModelGrayDiff = fMeanModelGrayDiff / vecItemsGrayDesc.size();
			char szTmp1[80] = { 0 };
			sprintf_s(szTmp1, "平均灰度差:%.3f, 与模板的平均灰度差:%.3f\n", fMeanGrayDiff, fMeanModelGrayDiff);
			strItemLog.append(szTmp1);
			strItemLog.append("灰度选中的阀值[");
			float fThreld = 0.0;
			float fGrayDiffLast = 0.0;		//对上一次判断选中的选项对下一个选项选中判断的增益
			for (int i = 0; i < vecOmrItemGrayDiff.size(); i++)
			{
				float fGrayThresholdGray = vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray - fMeanGrayDiff;
				char szTmp[40] = { 0 };
				sprintf_s(szTmp, "%s:%.3f ", vecOmrItemGrayDiff[i].szVal, _dDiffThread_3_ + fGrayThresholdGray + fGrayDiffLast);
				strItemLog.append(szTmp);
				fGrayDiffLast += abs(fGrayThresholdGray) / 2;
			}
			strItemLog.append("]");
			//--------------------------

			char szOmrItem[3060] = { 0 };
			if (nPrintOmrVal)	//itOmr->nDoubt
				sprintf_s(szOmrItem, "%d(%s):%s[%s -- %s -- %s] Doubt(%d)\t==>%s\n", itOmr->nTH, szSingle, itOmr->strRecogVal.c_str(), itOmr->strRecogVal1.c_str(), itOmr->strRecogVal2.c_str(), itOmr->strRecogVal3.c_str(), itOmr->nDoubt, strItemLog.c_str());	//szItemInfo
	// 		else
	// 			sprintf_s(szOmrItem, "%d(%s):%s[%s -- %s -- %s] Doubt(%d)\n", itOmr->nTH, szSingle, itOmr->strRecogVal.c_str(), itOmr->strRecogVal1.c_str(), itOmr->strRecogVal2.c_str(), itOmr->strRecogVal3.c_str(), itOmr->nDoubt);

			strPaperLog.append(szOmrItem);
		}
		int nCount = pPaper->lOmrResult.size();
		if (nCount)
		{
			std::string strCorrectPerInfo = Poco::format("%s总数[%u],空值%d(%.2f%%),怀疑%d(%.2f%%),无怀疑%d(%.2f%%)", pPaper->strStudentInfo, pPaper->lOmrResult.size(), nNullCount, (double)nNullCount / nCount * 100, \
														 nDoubtCount, (double)nDoubtCount / nCount * 100, nEqualCount, (double)nEqualCount / nCount * 100);

			strPaperLog.append(strCorrectPerInfo);
			//g_Log.LogOut(strCorrectPerInfo);
		}
		g_Log.LogOut(strPaperLog);
	}
#endif

//	pPAPERSINFO pPapers = static_cast<pPAPERSINFO>(pPaper->pPapers);
	pPapers->fmRecogCompleteOK.lock();
	pPapers->nRecogPics++;
	pPapers->fmRecogCompleteOK.unlock();
	if (pPapers->nRecogPics == (pPapers->lPaper.size() + pPapers->lIssue.size()))
	{
		calcStatistics(pPapers);

		(static_cast<CDialog*>(pPaper->pSrcDlg))->SendMessage(MSG_RECOG_COMPLETE, (WPARAM)pPaper, (LPARAM)pPapers);		//PostMessageW
		
// 		USES_CONVERSION;
// 		CString strMsg;
// 		if (pPapers->lIssue.size() == 0)
// 			strMsg.Format(_T("%s识别完成\r\n"), A2T(pPapers->strPapersName.c_str()));
// 		else
// 			strMsg.Format(_T("%s识别出问题试卷\r\n"), A2T(pPapers->strPapersName.c_str()));
// 		(static_cast<CDataMgrToolDlg*>(pPaper->pSrcDlg))->showMsg(strMsg);


		_fmRecogPapers_.lock();
		_nRecogPapers_++;
		_fmRecogPapers_.unlock();

		if (pPapers->nSendEzs == 1)
		{
			pSEND_HTTP_TASK pRecogResultTask = new SEND_HTTP_TASK;
			pRecogResultTask->nTaskType = 7;
			pRecogResultTask->pPapers = pPapers;
			pRecogResultTask->bSendOmr = pPapers->bRecogOmr;
			pRecogResultTask->bSendZkzh = pPapers->bRecogZkzh;
			pRecogResultTask->bSendElectOmr = pPapers->bRecogElectOmr;
			g_fmHttpSend.lock();
			g_lHttpSend.push_back(pRecogResultTask);
			g_fmHttpSend.unlock();
			return;
		}
		else if (pPapers->nSendEzs == 2)
		{
			//删除源文件夹
			try
			{
				Poco::File srcFileDir(CMyCodeConvert::Gb2312ToUtf8(pPapers->strPapersPath));
				if (srcFileDir.exists())
					srcFileDir.remove(true);
			}
			catch (Poco::Exception& exc)
			{
				std::string strErr = "删除文件夹(" + pPapers->strPapersPath + ")失败: " + exc.message();
				g_Log.LogOutError(strErr);
			}

			g_fmPapers.lock();			//释放试卷袋列表
			PAPERS_LIST::iterator itPapers = g_lPapers.begin();
			for (; itPapers != g_lPapers.end(); itPapers++)
			{
				pPAPERSINFO pPapersTask = *itPapers;
				if (pPapersTask == pPapers)
				{
					itPapers = g_lPapers.erase(itPapers);
					SAFE_RELEASE(pPapersTask);
					break;
				}
			}
			g_fmPapers.unlock();
			return;
		}
		else if (pPapers->nSendEzs == 3)	//在主窗口显示试卷袋
		{
			return;
		}

		//写文件
		SavePapersInfo(pPapers);
		
		//压缩
		std::string strNewPapersSaveDir;
		strNewPapersSaveDir = (static_cast<CDataMgrToolDlg*>(pPaper->pSrcDlg))->m_strPkgPath + "\\newPkg";

		int nModelOmrCount = 0;
		for (int k = 0; k < _pModel_->vecPaperModel.size(); k++)
			nModelOmrCount += _pModel_->vecPaperModel[k]->lOMR2.size();
		int nPapersCount = pPapers->lPaper.size() + pPapers->lIssue.size();
		int nOmrCount = nModelOmrCount * nPapersCount;
		if (_dDoubtPer_ > 0 && ((float)pPapers->nOmrDoubt / nOmrCount * 100 > _dDoubtPer_))
			strNewPapersSaveDir = (static_cast<CDataMgrToolDlg*>(pPaper->pSrcDlg))->m_strPkgPath + "\\newPkg_HighError";

		try
		{
			Poco::File fileDir(CMyCodeConvert::Gb2312ToUtf8(strNewPapersSaveDir));
			if (!fileDir.exists())
				fileDir.createDirectories();
		}
		catch (Poco::Exception& exc)
		{
		}

		std::string strNewPapersSavePath = strNewPapersSaveDir + "\\" + pPapers->strPapersName;
		pCOMPRESSTASK pTask = new COMPRESSTASK;
		pTask->strCompressFileName = pPapers->strSrcPapersFileName;
		pTask->strExtName = ".pkg";
		pTask->strSavePath = strNewPapersSavePath;
		pTask->strSrcFilePath = pPapers->strPapersPath;
		pTask->pPapers = pPapers;
		g_fmCompressLock.lock();
		g_lCompressTask.push_back(pTask);
		g_fmCompressLock.unlock();
	}
}

inline bool CRecognizeThread::Recog(int nPic, RECTINFO& rc, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo)
{
	Mat matCompRoi;
	Rect rt = rc.rt;
	bool bResult = false;
	try
	{
		if (rt.x < 0) rt.x = 0;
		if (rt.y < 0) rt.y = 0;
		if (rt.br().x > matCompPic.cols)
		{
			rt.width = matCompPic.cols - rt.x;
		}
		if (rt.br().y > matCompPic.rows)
		{
			rt.height = matCompPic.rows - rt.y;
		}

 		matCompRoi = matCompPic(rt);

		Mat imag_src, img_comp;
		if (matCompRoi.channels() == 3)
			cv::cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

		Mat matCompRoi2 = matCompRoi.clone();
		if ((rc.eCPType == OMR || rc.eCPType == SN) && g_nRecogWithContract)
		{
			AutoContractBright(matCompRoi);
		}

		cv::GaussianBlur(matCompRoi, matCompRoi, cv::Size(rc.nGaussKernel, rc.nGaussKernel), 0, 0);	//_nGauseKernel_
		SharpenImage(matCompRoi, matCompRoi, rc.nSharpKernel);

		const int channels[1] = { 0 };
		const float* ranges[1];
		const int histSize[1] = { 1 };
		float hranges[2];
#if 1	//for test

		if (rc.eCPType != WHITE_CP)
		{
			hranges[0] = g_nRecogGrayMin;
			hranges[1] = static_cast<float>(rc.nThresholdValue);
			ranges[0] = hranges;
		}
		else
		{
			hranges[0] = static_cast<float>(rc.nThresholdValue);
			hranges[1] = g_nRecogGrayMax_White;	//255		//256时可统计完全空白的点，即RGB值为255的完全空白点;255时只能统计到RGB为254的值，255的值统计不到
			ranges[0] = hranges;
		}
#else
		if (rc.eCPType != WHITE_CP && rc.eCPType != OMR)
		{
			hranges[0] = g_nRecogGrayMin;
			hranges[1] = static_cast<float>(rc.nThresholdValue);
			ranges[0] = hranges;
		}
		else if (rc.eCPType == OMR)
		{
			hranges[0] = g_nRecogGrayMin_OMR;
			hranges[1] = g_RecogGrayMax_OMR;
			ranges[0] = hranges;
		}
		else
		{
			hranges[0] = static_cast<float>(rc.nThresholdValue);
			hranges[1] = g_nRecogGrayMax_White;	//255		//256时可统计完全空白的点，即RGB值为255的完全空白点;255时只能统计到RGB为254的值，255的值统计不到
			ranges[0] = hranges;
		}
#endif
		MatND src_hist, comp_hist;
		cv::calcHist(&matCompRoi, 1, channels, Mat(), comp_hist, 1, histSize, ranges, false);

		double maxValComp = 0;
		double minValComp = 0;

		//找到直方图中的最大值和最小值
		cv::minMaxLoc(comp_hist, &minValComp, &maxValComp, 0, 0);
		int nSize = comp_hist.rows;

		// 设置最大峰值为图像高度的90%
		int hpt = static_cast<int>(0.9 * 256);		//histSize
		for (int h = 0; h < nSize; h++)	//histSize
		{
// 			float binValSrc = src_hist.at<float>(h);
// 			int intensitySrc = static_cast<int>(binValSrc*hpt / maxValSrc);
//			TRACE("h = %d. binValSrc = %f. intensitySrc = %d\n", h, binValSrc, intensitySrc);

			float binValComp = comp_hist.at<float>(h);
			int intensityComp = static_cast<int>(binValComp*hpt / maxValComp);

			rc.fRealValue = binValComp;
			rc.fRealArea = rc.rt.area();
			rc.fRealDensity = rc.fRealValue / rc.fRealArea;
			

			float fStandardCompare = (rc.fRealArea / rc.fStandardArea) * rc.fStandardValue;
			if (binValComp == 0 && rc.fStandardValue == 0)
				rc.fRealValuePercent = 1.0;
			else if (rc.fStandardValue == 0)
				rc.fRealValuePercent = 1.0;
			else
				rc.fRealValuePercent = binValComp / rc.fStandardValue;
			bResult = true;
			break;
		}

	#if 1		//第三种OMR识别方法测试
		#if 1
			MatND mean;
			MatND stddev;
			meanStdDev(matCompRoi, mean, stddev);	//matCompRoi

			IplImage *src;
			src = &IplImage(mean);
			rc.fRealMeanGray = cvGetReal2D(src, 0, 0);

			IplImage *src2;
			src2 = &IplImage(stddev);
			rc.fRealStddev = cvGetReal2D(src2, 0, 0);
		#else
			int nMaxVal = 256;
			MatND src_hist2;
			const int histSize2[1] = { nMaxVal };	//rc.nThresholdValue - g_nRecogGrayMin		256
			const float* ranges2[1];
			float hranges2[2];
			hranges2[0] = 0;
			hranges2[1] = nMaxVal - 1;			//255
			ranges2[0] = hranges2;
			cv::calcHist(&matCompRoi, 1, channels, Mat(), src_hist2, 1, histSize2, ranges2, true, false);
			int nCount = 0;
			int nArea = 0;
			for (int i = 0; i < nMaxVal; i++)	//256
			{
				nArea += src_hist2.at<float>(i);
				nCount += i * src_hist2.at<float>(i);
			}
			rc.fRealMeanGray = (float)nCount / nArea;
		#endif
	#else
		MatND src_hist2;
		const int histSize2[1] = { 256 };	//rc.nThresholdValue - g_nRecogGrayMin		256
		const float* ranges2[1];
		float hranges2[2];
		hranges2[0] = 0;
		hranges2[1] = 255;			//255
		ranges2[0] = hranges2;
		cv::calcHist(&matCompRoi, 1, channels, Mat(), src_hist2, 1, histSize2, ranges2, true, false);
		int nCount = 0;
		for (int i = 0; i < 256; i++)	//256
		{
			nCount += i * src_hist2.at<float>(i);
		}
		rc.fRealMeanGray = nCount / rc.fRealArea;
	#endif
	}
	catch (cv::Exception &exc)
	{
		char szLog[300] = { 0 };
		sprintf_s(szLog, "CRecognizeThread::Recog error. detail: %s\n", exc.msg);
		g_Log.LogOut(szLog);
		TRACE(szLog);
	}
	
	return bResult;
}

bool CRecognizeThread::ChkPicRotation(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, std::string& strLog)
{
	TRACE("ChkPicRotation: %s\n", pPic->strPicName.c_str());

	bool bDoubleScan = pModelInfo->pModel->vecPaperModel.size() % 2 == 0 ? true : false;

	int nResult = _chkRotationObj.GetRightPicOrientation(matCompPic, nPic, bDoubleScan);
	std::string strDirection;
	switch (nResult)
	{
		case 1: strDirection = "正向，不需要旋转"; break;
		case 2: strDirection = "右旋90(模板图像旋转)"; break;
		case 3: strDirection = "左旋90(模板图像旋转)"; break;
		case 4: strDirection = "右旋180"; break;
	}
	pPic->nRecogRotation = nResult;
	strLog.append("图片" + pPic->strPicName + "方向判断结果：" + strDirection);
	//g_Log.LogOut(strLog);

	return true;
}

bool CRecognizeThread::RecogFixCP(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, int nRecogMode, std::string& strLog)
{
	bool bResult = true;
// 	if (pModelInfo->pModel->nHasHead != 0)	//有同步头的，不需要进行定点识别
// 		return bResult;

	TRACE("recog Fix: %s\n", pPic->strPicName.c_str());

	clock_t start, end;
	start = clock();
	//strLog = Poco::format("图片%s\n", pPic->strPicName);

	RECTLIST::iterator itCP = pModelInfo->pModel->vecPaperModel[nPic]->lSelFixRoi.begin();
	for (int i = 0; itCP != pModelInfo->pModel->vecPaperModel[nPic]->lSelFixRoi.end(); i++, itCP++)
	{
		RECTINFO rc = *itCP;

		std::vector<Rect>RectCompList;
		try
		{
		#if 1
			float fModelW = pModelInfo->pModel->vecPaperModel[nPic]->nPicW;
			float fModelH = pModelInfo->pModel->vecPaperModel[nPic]->nPicH;
			int nRealW = matCompPic.cols;
			int nRealH = matCompPic.rows;

			cv::Rect rtTmp;
			rtTmp.x = rc.rt.x * nRealW / fModelW;
			rtTmp.y = rc.rt.y * nRealH / fModelH;
			rtTmp.width = rc.rt.width * nRealW / fModelW;
			rtTmp.height = rc.rt.height * nRealH / fModelH;

			if (rtTmp.x < 0) rtTmp.x = 0;
			if (rtTmp.y < 0) rtTmp.y = 0;
			if (rtTmp.br().x > matCompPic.cols)
			{
				rtTmp.width = matCompPic.cols - rtTmp.x;
			}
			if (rtTmp.br().y > matCompPic.rows)
			{
				rtTmp.height = matCompPic.rows - rtTmp.y;
			}
			cv::Mat matCompRoi;
			matCompRoi = matCompPic(rtTmp);
		#else
			if (rc.rt.x < 0) rc.rt.x = 0;
			if (rc.rt.y < 0) rc.rt.y = 0;
			if (rc.rt.br().x > matCompPic.cols)
			{
				rc.rt.width = matCompPic.cols - rc.rt.x;
			}
			if (rc.rt.br().y > matCompPic.rows)
			{
				rc.rt.height = matCompPic.rows - rc.rt.y;
			}
			cv::Mat matCompRoi;
			matCompRoi = matCompPic(rc.rt);
		#endif
			if(matCompRoi.channels() == 3)
				cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

			GaussianBlur(matCompRoi, matCompRoi, cv::Size(rc.nGaussKernel, rc.nGaussKernel), 0, 0);	//cv::Size(_nGauseKernel_, _nGauseKernel_)
			SharpenImage(matCompRoi, matCompRoi, rc.nSharpKernel);

			int nRealThreshold = 150;
			RECTLIST::iterator itFix = pModelInfo->pModel->vecPaperModel[nPic]->lFix.begin();
			for (int j = 0; itFix != pModelInfo->pModel->vecPaperModel[nPic]->lFix.end(); j++, itFix++)
			{
				if (j == i)
				{
					nRealThreshold = itFix->nThresholdValue;
					break;
				}
			}

#ifdef USES_GETTHRESHOLD_ZTFB
			const int channels[1] = { 0 };
			const int histSize[1] = { nRealThreshold };
			float hranges[2] = { 0, nRealThreshold };
			const float* ranges[1];
			ranges[0] = hranges;
			MatND hist;
			calcHist(&matCompRoi, 1, channels, Mat(), hist, 1, histSize, ranges);	//histSize, ranges

			int nSum = 0;
			int nDevSum = 0;
			int nCount = 0;
			for (int h = 0; h < hist.rows; h++)	//histSize
			{
				float binVal = hist.at<float>(h);

				nCount += static_cast<int>(binVal);
				nSum += h*binVal;
			}
			int nThreshold = nRealThreshold;
			if (nCount > 0)
			{
				float fMean = (float)nSum / nCount;		//均值

				for (int h = 0; h < hist.rows; h++)	//histSize
				{
					float binVal = hist.at<float>(h);

					nDevSum += pow(h - fMean, 2)*binVal;
				}
				float fStdev = sqrt(nDevSum / nCount);
				nThreshold = fMean + 2 * fStdev;
				if (fStdev > fMean)
					nThreshold = fMean + fStdev;
			}		

			if (nThreshold > nRealThreshold) nThreshold = nRealThreshold;
			threshold(matCompRoi, matCompRoi, nThreshold, 255, THRESH_BINARY);
#else
			threshold(matCompRoi, matCompRoi, 60, 255, THRESH_BINARY);
#endif
			//去除干扰信息，先膨胀后腐蚀还原, 可去除一些线条干扰
			Mat element_Anticlutter = getStructuringElement(MORPH_RECT, Size(_nAnticlutterKernel_, _nAnticlutterKernel_));	//Size(6, 6)	普通空白框可识别		Size(3, 3)
			dilate(matCompRoi, matCompRoi, element_Anticlutter);
			erode(matCompRoi, matCompRoi, element_Anticlutter);

			cv::Canny(matCompRoi, matCompRoi, 0, rc.nCannyKernel, 5);	//_nCannyKernel_
			Mat element = getStructuringElement(MORPH_RECT, Size(rc.nDilateKernel, rc.nDilateKernel));	//Size(6, 6)	普通空白框可识别	Size(_nDilateKernel_, _nDilateKernel_)
			dilate(matCompRoi, matCompRoi, element);

#if 1
			//		std::vector<std::vector<cv::Point> > vecContours;		//轮廓信息存储
			//		m_vecContours.clear();
			//		std::vector<cv::Mat> vecContours;
			//		cv::findContours(matCompRoi.clone(), vecContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);	//hsvRe.clone()		//CV_RETR_EXTERNAL	//CV_CHAIN_APPROX_SIMPLE

			// 		vector<Mat> vecContours;
			// 		Mat hierarchy;
			// 		findContours(matCompRoi, vecContours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
			// 
			// 		std::vector<Rect>RectCompList;
			// 		for (int i = 0; i < vecContours.size(); i++)
			// 		{
			// 			cv::Mat matTmp = cv::Mat(vecContours[i]);
			// 			Rect rm = cv::boundingRect(matTmp);
			// 
			// 			// 			Rect rm = cv::boundingRect(vecContours[i]);
			// 			// 			vecContours[i].release();
			// 
			// 			RectCompList.push_back(rm);
			// 			matTmp.release();
			// 		}

			IplImage ipl_img(matCompRoi);

			//the parm. for cvFindContours  
			CvMemStorage* storage = cvCreateMemStorage(0);
			CvSeq* contour = 0;

			//提取轮廓  
			cvFindContours(&ipl_img, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

			for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++/*更新迭代索引*/)
			{
				CvRect aRect = cvBoundingRect(contour, 0);
				Rect rm = aRect;
				rm = rm + rtTmp.tl();	// rc.rt.tl();
				RectCompList.push_back(rm);
			}
			cvReleaseMemStorage(&storage);
#else
			//		std::vector<std::vector<cv::Point> > vecContours;		//轮廓信息存储
			m_vecContours.clear();
			//		std::vector<cv::Mat> vecContours;
			cv::findContours(matCompRoi.clone(), m_vecContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);	//hsvRe.clone()		//CV_RETR_EXTERNAL	//CV_CHAIN_APPROX_SIMPLE

			//		bool bResult = false;
			std::vector<Rect>RectCompList;
			for (int i = 0; i < m_vecContours.size(); i++)
			{
				Rect rm = cv::boundingRect(cv::Mat(m_vecContours[i]));
				//			Rect rm = cv::boundingRect(cv::Mat(vecContours[i]));

				// 			std::vector<cv::Point> v;
				// 			m_vecContours[i].swap(v);
				// 			if (rm.width < 10 || rm.height < 7 || rm.width > 70 || rm.height > 50 || rm.area() < 70)
				// 			{
				// 				TRACE("抛弃不符合条件的矩形块,(%d, %d, %d, %d)\n", rm.x, rm.y, rm.width, rm.height);
				// 				continue;
				// 			}
				RectCompList.push_back(rm);

				//			m_vecContours[i].clear();
				//			bResult = true;
			}
#endif
		}
		catch (cv::Exception& exc)
		{
			std::string strLog2 = Poco::format("识别定点(%d)异常: %s\n", i, exc.msg);
			strLog.append(strLog2);
			TRACE(strLog2.c_str());

			pPic->bFindIssue = true;
			pPic->lIssueRect.push_back(rc);
			if (nRecogMode == 2)
			{
				bResult = false;						//找到问题点
				break;
			}
		}

		std::string strLog2;	//临时日志，记录矩形具体识别结果
		bool bFindRect = false;
		if(RectCompList.size() == 0)
			bFindRect = true;
		else
		{
			std::sort(RectCompList.begin(), RectCompList.end(), SortByArea);
			Rect& rtFix = RectCompList[0];

			RECTINFO rcFix;
			RECTLIST::iterator itFix = pModelInfo->pModel->vecPaperModel[nPic]->lFix.begin();
			for (int j = 0; itFix != pModelInfo->pModel->vecPaperModel[nPic]->lFix.end(); j++, itFix++)
			{
				if (j == i)
				{
					rcFix = *itFix;
					break;
				}
			}

			bool bOnlyOne = false;		//只有一个矩形，需要判断面积和灰度，但是比例可以降低
			bool bFind = false;

			//通过灰度值来判断
			for (int k = 0; k < RectCompList.size(); k++)
			{
				RECTINFO rcTmp = rcFix;
				rcTmp.rt = RectCompList[k];

				//根据定点左上点与右下点位置判断是否在试卷的边线上，如果在，则可能是折角或者边上有损坏
				cv::Point pt1 = RectCompList[k].tl();
				cv::Point pt2 = RectCompList[k].br();
				int nDiff = 4;	//与图像边界的距离间隔在这个值之内，认为属于边界线上
				if (pt1.x < nDiff || pt1.y < nDiff || matCompPic.cols - pt2.x < nDiff || matCompPic.rows - pt2.y < nDiff)
				{
					TRACE("矩形(%d,%d,%d,%d)位置距离边线太近，可能是折角或损坏\n", RectCompList[k].x, RectCompList[k].y, RectCompList[k].width, RectCompList[k].height);
					continue;
				}

				Recog(nPic, rcTmp, matCompPic, pPic, pModelInfo);
				float fArea = rcTmp.fRealArea / rcTmp.fStandardArea;
				float fDensity = rcTmp.fRealDensity / rcTmp.fStandardDensity;
				float fWper = (float)rcTmp.rt.width / rcFix.rt.width;			//查找的矩形的宽度与模板对应定点的宽度之比
				float fHper = (float)rcTmp.rt.height / rcFix.rt.height;			//查找的矩形的宽度与模板对应定点的高度之比
				std::string strTmpLog = Poco::format("第%d个矩形:area=%f, Density=%f\t", k, (double)fArea, (double)fDensity);
				strLog2.append(strTmpLog);
				if ((bOnlyOne && fArea > 0.4 && fArea < 2.5 && fDensity > rcTmp.fStandardValuePercent * 0.9 && fWper < 2.0 && fWper > 0.4 && fHper < 2.0 && fHper > 0.4) || \
					(fArea > 0.5 && fArea < 2.0 && fDensity > rcTmp.fStandardValuePercent && fWper < 2.0 && fWper > 0.4 && fHper < 2.0 && fHper > 0.4))	//fArea > 0.7 && fArea < 1.5 && fDensity > 0.6
				{
					bFind = true;
					rtFix = RectCompList[k];
					break;
				}
			}
			
			if (!bFind)
				bFindRect = true;
			else
			{
				RECTINFO rcFixInfo = rc;
				rcFixInfo.nTH = i;			//这是属于模板上定点列表的第几个
				rcFixInfo.rt = rtFix;
				pPic->lFix.push_back(rcFixInfo);
				TRACE("定点矩形: (%d,%d,%d,%d)\n", rtFix.x, rtFix.y, rtFix.width, rtFix.height);
			}
		}
		if (bFindRect)
		{
			std::string strLog3 = Poco::format("识别定点(%d)失败 -- %s\n", i, strLog2);
			strLog.append(strLog3);
			bResult = false;						//找到问题点
			pPic->bFindIssue = true;
			pPic->lIssueRect.push_back(rc);
			if (nRecogMode == 2)
				break;
		}
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别定点失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		TRACE(szLog);
	}
	end = clock();
#ifdef PrintRunTime
	std::string strTime = Poco::format("识别定点时间: %dms\n", (int)(end - start));
	strLog.append(strTime);
#endif
	//g_Log.LogOut(strLog);
	return bResult;
}

bool CRecognizeThread::RecogHHead(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo)
{
	bool bResult = true;
	if (pModelInfo->pModel->nHasHead == 0)
		return true;

	TRACE("recog HHead: %s\n", pPic->strPicName.c_str());

	std::string strErrDesc;
	m_vecH_Head.clear();
	RECTLIST::iterator itRoi = pModelInfo->pModel->vecPaperModel[nPic]->lSelHTracker.begin();
	for (; itRoi != pModelInfo->pModel->vecPaperModel[nPic]->lSelHTracker.end(); itRoi++)
	{
		RECTINFO rc = *itRoi;
#if 1
		std::vector<Rect>RectCompList;
		try
		{
			if (rc.rt.x < 0) rc.rt.x = 0;
			if (rc.rt.y < 0) rc.rt.y = 0;
			if (rc.rt.br().x > matCompPic.cols)
			{
				rc.rt.width = matCompPic.cols - rc.rt.x;
			}
			if (rc.rt.br().y > matCompPic.rows)
			{
				rc.rt.height = matCompPic.rows - rc.rt.y;
			}

			Mat matCompRoi;
			matCompRoi = matCompPic(rc.rt);

			if (matCompRoi.channels() == 3)
				cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

			GaussianBlur(matCompRoi, matCompRoi, cv::Size(rc.nGaussKernel, rc.nGaussKernel), 0, 0);	//cv::Size(_nGauseKernel_, _nGauseKernel_)
			SharpenImage(matCompRoi, matCompRoi, rc.nSharpKernel);

#ifdef USES_GETTHRESHOLD_ZTFB
			const int channels[1] = { 0 };
			const int histSize[1] = { 150 };
			float hranges[2] = { 0, 150 };
			const float* ranges[1];
			ranges[0] = hranges;
			MatND hist;
			calcHist(&matCompRoi, 1, channels, Mat(), hist, 1, histSize, ranges);	//histSize, ranges

			int nSum = 0;
			int nDevSum = 0;
			int nCount = 0;
			for (int h = 0; h < hist.rows; h++)	//histSize
			{
				float binVal = hist.at<float>(h);

				nCount += static_cast<int>(binVal);
				nSum += h*binVal;
			}
			int nThreshold = 150;
			if (nCount > 0)
			{
				float fMean = (float)nSum / nCount;		//均值

				for (int h = 0; h < hist.rows; h++)	//histSize
				{
					float binVal = hist.at<float>(h);

					nDevSum += pow(h - fMean, 2)*binVal;
				}
				float fStdev = sqrt(nDevSum / nCount);
				nThreshold = fMean + 2 * fStdev;
				if (fStdev > fMean)
					nThreshold = fMean + fStdev;
			}
			if (nThreshold > 150) nThreshold = 150;
			threshold(matCompRoi, matCompRoi, nThreshold, 255, THRESH_BINARY);
#else
			threshold(matCompRoi, matCompRoi, 60, 255, THRESH_BINARY);
#endif
			//去除干扰信息，先膨胀后腐蚀还原, 可去除一些线条干扰
			Mat element_Anticlutter = getStructuringElement(MORPH_RECT, Size(_nAnticlutterKernel_, _nAnticlutterKernel_));	//Size(6, 6)	普通空白框可识别		Size(3, 3)
			dilate(matCompRoi, matCompRoi, element_Anticlutter);
			erode(matCompRoi, matCompRoi, element_Anticlutter);

			cv::Canny(matCompRoi, matCompRoi, 0, rc.nCannyKernel, 5);	//_nCannyKernel_
			Mat element = getStructuringElement(MORPH_RECT, Size(rc.nDilateKernel, rc.nDilateKernel));	//Size(6, 6)	普通空白框可识别		Size(3, 3)
			dilate(matCompRoi, matCompRoi, element);
			IplImage ipl_img(matCompRoi);

			//the parm. for cvFindContours  
			CvMemStorage* storage = cvCreateMemStorage(0);
			CvSeq* contour = 0;

			//提取轮廓  
			cvFindContours(&ipl_img, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

#if 1
			//模板图像的水平同步头平均长宽
			RECTLIST::iterator itBegin = pModelInfo->pModel->vecPaperModel[nPic]->lH_Head.begin();
			RECTINFO rcFist = *itBegin;
			RECTINFO rcSecond = *(++itBegin);
			
			int nMid_minW, nMid_maxW, nMid_minH, nMid_maxH;
			int nHead_minW, nHead_maxW, nHead_minH, nHead_maxH;
#if 0	//test
			float fOffset = 0.2;
			nMid_minW = rcSecond.rt.width * (1 - fOffset);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
			nMid_maxW = rcSecond.rt.width * (1 + fOffset);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
			nMid_minH = rcSecond.rt.height * (1 - fOffset);		//同上
			nMid_maxH = rcSecond.rt.height * (1 + fOffset);		//同上

			nHead_minW = rcFist.rt.width * (1 - fOffset);		//两端同步头(第一个或最后一个)宽度与两端中间同步头宽度的偏差不超过模板同步头宽度的0.2
			nHead_maxW = rcFist.rt.width * (1 + fOffset);		//同上
			nHead_minH = rcFist.rt.height * (1 - fOffset);		//同上
			nHead_maxH = rcFist.rt.height * (1 + fOffset);		//同上
#else
			if (pModelInfo->pModel->nType == 1)
			{
				int nMid_modelW = rcSecond.rt.width;
				int nMid_modelH = rcSecond.rt.height;
				int nMidInterW, nMidInterH, nHeadInterW, nHeadInterH;
				nMidInterW = 3;
				nMidInterH = 3;
				nHeadInterW = 4;
				nHeadInterH = 4;
				nMid_minW = nMid_modelW - nMidInterW;
				nMid_maxW = nMid_modelW + nMidInterW;
				nMid_minH = nMid_modelH - nMidInterH;
				nMid_maxH = nMid_modelH + nMidInterH;

				nHead_minW = rcFist.rt.width - nHeadInterW;
				nHead_maxW = rcFist.rt.width + nHeadInterW;
				nHead_minH = rcFist.rt.height - nHeadInterH;
				nHead_maxH = rcFist.rt.height + nHeadInterH;

// 				float fOffset = 0.1;
// 				float fPer_W, fPer_H;	//模板第二个点与第一个点的宽、高的比例，用于最小值控制
// 				fPer_W = 0.5;
// 				fPer_H = 0.25;
// 				int nMid_modelW = rcSecond.rt.width + 2;		//加2是因为制卷模板框框没有经过查边框运算，经过查边框后，外框会包含整个矩形，需要加上上下各1个单位的线宽
// 				int nMid_modelH = rcSecond.rt.height + 2;
// 				if (nMid_modelW < rcFist.rt.width * fPer_W + 0.5)	nMid_modelW = rcFist.rt.width * fPer_W + 0.5;
// 				if (nMid_modelH < rcFist.rt.height * fPer_H + 0.5)	nMid_modelH = rcFist.rt.height * fPer_H + 0.5;
// 				nMid_minW = nMid_modelW * (1 - fOffset);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
// 				nMid_maxW = nMid_modelW * (1 + fOffset * 4) + 0.5;		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
// 				nMid_minH = nMid_modelH * (1 - fOffset);				//同上
// 				nMid_maxH = nMid_modelH * (1 + fOffset * 4) + 0.5;		//同上
// 
// 				nHead_minW = rcFist.rt.width * (1 - fOffset);		//两端同步头(第一个或最后一个)宽度与两端中间同步头宽度的偏差不超过模板同步头宽度的0.2
// 				nHead_maxW = rcFist.rt.width * (1 + fOffset * 4) + 0.5;		//同上
// 				nHead_minH = rcFist.rt.height * (1 - fOffset);				//同上
// 				nHead_maxH = rcFist.rt.height * (1 + fOffset * 4) + 0.5;	//同上
			}
			else
			{
				float fOffset_mid = 0.4;
				nMid_minW = rcSecond.rt.width * (1 - fOffset_mid);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
				nMid_maxW = rcSecond.rt.width * (1 + fOffset_mid);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
				nMid_minH = rcSecond.rt.height * (1 - fOffset_mid);		//同上
				nMid_maxH = rcSecond.rt.height * (1 + fOffset_mid);		//同上

				float fOffset_Head = 0.3;
				nHead_minW = rcFist.rt.width * (1 - fOffset_Head);		//两端同步头(第一个或最后一个)宽度与两端中间同步头宽度的偏差不超过模板同步头宽度的0.2
				nHead_maxW = rcFist.rt.width * (1 + fOffset_Head);		//同上
				nHead_minH = rcFist.rt.height * (1 - fOffset_Head);		//同上
				nHead_maxH = rcFist.rt.height * (1 + fOffset_Head);		//同上
			}
#endif

			int nYSum = 0;
			for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++/*更新迭代索引*/)
			{
				CvRect aRect = cvBoundingRect(contour, 0);
				Rect rm = aRect;
				rm.x = rm.x + rc.rt.x;
				rm.y = rm.y + rc.rt.y;
				
				if (rm.width < nMid_minW || rm.height < nMid_minH || rm.width > nMid_maxW || rm.height > nMid_maxH)
				{
					if (!(rm.width > nHead_minW && rm.width < nHead_maxW && rm.height > nHead_minH && rm.height < nHead_maxH))	//排除第一个或最后一个大的同步头
					{
						TRACE("过滤水平同步头(%d,%d,%d,%d), 要求范围W:[%d,%d], H[%d,%d], 参考大小(%d,%d)\n", rm.x, rm.y, rm.width, rm.height, nMid_minW, nMid_maxW, nMid_minH, nMid_maxH, rcSecond.rt.width, rcSecond.rt.height);
						continue;
					}
					else
					{
						TRACE("首尾水平同步头(即定位点同步头)(%d,%d,%d,%d)\n", rm.x, rm.y, rm.width, rm.height);
					}
				}
				RectCompList.push_back(rm);
				nYSum += rm.y;
			}
			cvReleaseMemStorage(&storage);
//			int nYMean = nYSum / RectCompList.size();
#else
			for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++/*更新迭代索引*/)
			{
				CvRect aRect = cvBoundingRect(contour, 0);
				Rect rm = aRect;
				rm.x = rm.x + rc.rt.x;
				rm.y = rm.y + rc.rt.y;
				if (rm.width < 10 || rm.height < 7 || rm.width > 70 || rm.height > 50 || rm.area() < 70)
				{
					continue;
				}
				RectCompList.push_back(rm);
			}
#endif
		}
		catch (cv::Exception& exc)
		{
			std::string strLog = "识别水平同步头异常: " + exc.msg;
			g_Log.LogOut(strLog);
			TRACE(strLog.c_str());

			bResult = false;
			break;
		}
		if (RectCompList.size() == 0)
		{
			bResult = false;
			strErrDesc = "水平同步头数量为0.";
		}
		else
		{
			//二次过滤
			std::sort(RectCompList.begin(), RectCompList.end(), SortByPositionX2);
			std::vector<Rect>::iterator itHead = RectCompList.begin();
			for (int i = 0; itHead != RectCompList.end(); i++)
			{
				RECTINFO rcHead1;
				RECTLIST::iterator itModelHead = pModelInfo->pModel->vecPaperModel[nPic]->lH_Head.begin();
				for (int j = 0; itModelHead != pModelInfo->pModel->vecPaperModel[nPic]->lH_Head.end(); j++, itModelHead++)
				{
					if (j == i)
					{
						rcHead1 = *itModelHead;
						break;
					}
				}
				RECTINFO rcTmp = rcHead1;
				rcTmp.rt = RectCompList[i];
				Recog(nPic, rcTmp, matCompPic, pPic, pModelInfo);
				if (rcTmp.fRealArea / rcTmp.fStandardArea < 0.75 || rcTmp.fRealDensity / rcTmp.fStandardDensity < 0.8)
				{
					itHead = RectCompList.erase(itHead);
					i = i - 1;
				}
				else
					itHead++;
			}
			for (int i = 0; i < RectCompList.size(); i++)
			{
				RECTINFO rcHead;
				rcHead.rt = RectCompList[i];
				
				rcHead.nGaussKernel = rc.nGaussKernel;
				rcHead.nSharpKernel = rc.nSharpKernel;
				rcHead.nCannyKernel = rc.nCannyKernel;
				rcHead.nDilateKernel = rc.nDilateKernel;

				rcHead.eCPType = H_HEAD;
				m_vecH_Head.push_back(rcHead);
				//++ for test
				pPic->lNormalRect.push_back(rcHead);
				//--
			}
			std::sort(m_vecH_Head.begin(), m_vecH_Head.end(), SortByPositionX);
		}
		if(m_vecH_Head.size() != pModelInfo->pModel->vecPaperModel[nPic]->lH_Head.size())
		{
			bResult = false;
			pPic->bFindIssue = true;
			for(int i = 0; i < m_vecH_Head.size(); i++)
				pPic->lIssueRect.push_back(m_vecH_Head[i]);

			strErrDesc = Poco::format("水平同步头数量为%u, 与模板水平同步头数量(%u)不一致", m_vecH_Head.size(), pModelInfo->pModel->vecPaperModel[nPic]->lH_Head.size());
		}
#else
		GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic].lFix, rc.rt);
		bool bFindRect = Recog(nPic, rc, matCompPic, pPic, pModelInfo);

		if (bFindRect)
		{
			bResult = false;						//找到问题点
			pPic->bFindIssue = true;
			pPic->lIssueRect.push_back(rc);
		}
		else
			pPic->lNormalRect.push_back(rc);
#endif
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别水平同步头失败, 原因: %s, 图片名: %s\n", strErrDesc.c_str(), pPic->strPicName.c_str());
		g_Log.LogOut(szLog);
		TRACE(szLog);
	}
	return bResult;
}

bool CRecognizeThread::RecogVHead(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo)
{
	bool bResult = true;
	if (pModelInfo->pModel->nHasHead == 0)
		return true;

	TRACE("recog VHead: %s\n", pPic->strPicName.c_str());

	std::string strErrDesc;
	m_vecV_Head.clear();
	RECTLIST::iterator itRoi = pModelInfo->pModel->vecPaperModel[nPic]->lSelVTracker.begin();
	for (; itRoi != pModelInfo->pModel->vecPaperModel[nPic]->lSelVTracker.end(); itRoi++)
	{
		RECTINFO rc = *itRoi;
#if 1
		std::vector<Rect>RectCompList;
		try
		{
			if (rc.rt.x < 0) rc.rt.x = 0;
			if (rc.rt.y < 0) rc.rt.y = 0;
			if (rc.rt.br().x > matCompPic.cols)
			{
				rc.rt.width = matCompPic.cols - rc.rt.x;
			}
			if (rc.rt.br().y > matCompPic.rows)
			{
				rc.rt.height = matCompPic.rows - rc.rt.y;
			}

			Mat matCompRoi;
			matCompRoi = matCompPic(rc.rt);

			if (matCompRoi.channels() == 3)
				cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

			GaussianBlur(matCompRoi, matCompRoi, cv::Size(rc.nGaussKernel, rc.nGaussKernel), 0, 0);	//_nGauseKernel_
			SharpenImage(matCompRoi, matCompRoi, rc.nSharpKernel);

#ifdef USES_GETTHRESHOLD_ZTFB
			const int channels[1] = { 0 };
			const int histSize[1] = { 150 };
			float hranges[2] = { 0, 150 };
			const float* ranges[1];
			ranges[0] = hranges;
			MatND hist;
			calcHist(&matCompRoi, 1, channels, Mat(), hist, 1, histSize, ranges);	//histSize, ranges

			int nSum = 0;
			int nDevSum = 0;
			int nCount = 0;
			for (int h = 0; h < hist.rows; h++)	//histSize
			{
				float binVal = hist.at<float>(h);

				nCount += static_cast<int>(binVal);
				nSum += h*binVal;
			}
			int nThreshold = 150;
			if (nCount > 0)
			{
				float fMean = (float)nSum / nCount;		//均值

				for (int h = 0; h < hist.rows; h++)	//histSize
				{
					float binVal = hist.at<float>(h);

					nDevSum += pow(h - fMean, 2)*binVal;
				}
				float fStdev = sqrt(nDevSum / nCount);
				nThreshold = fMean + 2 * fStdev;
				if (fStdev > fMean)
					nThreshold = fMean + fStdev;
			}
			if (nThreshold > 150) nThreshold = 150;
			threshold(matCompRoi, matCompRoi, nThreshold, 255, THRESH_BINARY);
#else
			threshold(matCompRoi, matCompRoi, 60, 255, THRESH_BINARY);
#endif
			//去除干扰信息，先膨胀后腐蚀还原, 可去除一些线条干扰
			Mat element_Anticlutter = getStructuringElement(MORPH_RECT, Size(_nAnticlutterKernel_, _nAnticlutterKernel_));	//Size(6, 6)	普通空白框可识别		Size(3, 3)
			dilate(matCompRoi, matCompRoi, element_Anticlutter);
			erode(matCompRoi, matCompRoi, element_Anticlutter);

			cv::Canny(matCompRoi, matCompRoi, 0, rc.nCannyKernel, 5);	//_nCannyKernel_
			Mat element = getStructuringElement(MORPH_RECT, Size(rc.nDilateKernel, rc.nDilateKernel));	//Size(6, 6)	普通空白框可识别		Size(3, 3)
			dilate(matCompRoi, matCompRoi, element);
			IplImage ipl_img(matCompRoi);

			//the parm. for cvFindContours  
			CvMemStorage* storage = cvCreateMemStorage(0);
			CvSeq* contour = 0;

			//提取轮廓  
			cvFindContours(&ipl_img, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

#if 1
			//模板图像的水平同步头平均长宽
			RECTLIST::iterator itBegin = pModelInfo->pModel->vecPaperModel[nPic]->lV_Head.begin();
			RECTINFO rcFist = *itBegin;
			RECTINFO rcSecond = *(++itBegin);

			int nMid_minW, nMid_maxW, nMid_minH, nMid_maxH;
			int nHead_minW, nHead_maxW, nHead_minH, nHead_maxH;
#if 0	//test
			float fOffset = 0.2;
			nMid_minW = rcSecond.rt.width * (1 - fOffset);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
			nMid_maxW = rcSecond.rt.width * (1 + fOffset);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
			nMid_minH = rcSecond.rt.height * (1 - fOffset);		//同上
			nMid_maxH = rcSecond.rt.height * (1 + fOffset);		//同上

			nHead_minW = rcFist.rt.width * (1 - fOffset);		//两端同步头(第一个或最后一个)宽度与两端中间同步头宽度的偏差不超过模板同步头宽度的0.2
			nHead_maxW = rcFist.rt.width * (1 + fOffset);		//同上
			nHead_minH = rcFist.rt.height * (1 - fOffset);		//同上
			nHead_maxH = rcFist.rt.height * (1 + fOffset);		//同上
#else
			if (pModelInfo->pModel->nType == 1)
			{
				int nMid_modelW = rcSecond.rt.width;
				int nMid_modelH = rcSecond.rt.height;
				int nMidInterW, nMidInterH, nHeadInterW, nHeadInterH;
				nMidInterW = 3;
				nMidInterH = 3;
				nHeadInterW = 4;
				nHeadInterH = 4;
				nMid_minW = nMid_modelW - nMidInterW;
				nMid_maxW = nMid_modelW + nMidInterW;
				nMid_minH = nMid_modelH - nMidInterH;
				nMid_maxH = nMid_modelH + nMidInterH;

				nHead_minW = rcFist.rt.width - nHeadInterW;
				nHead_maxW = rcFist.rt.width + nHeadInterW;
				nHead_minH = rcFist.rt.height - nHeadInterH;
				nHead_maxH = rcFist.rt.height + nHeadInterH;

// 				float fOffset = 0.1;
// 				float fPer_W, fPer_H;	//模板第二个点与第一个点的宽、高的比例，用于最小值控制
// 				fPer_W = 0.5;
// 				fPer_H = 0.25;
// 				int nMid_modelW = rcSecond.rt.width + 2;		//加2是因为制卷模板框框没有经过查边框运算，经过查边框后，外框会包含整个矩形，需要加上上下各1个单位的线宽
// 				int nMid_modelH = rcSecond.rt.height + 2;
// 				if (nMid_modelW < rcFist.rt.width * fPer_W + 0.5)	nMid_modelW = rcFist.rt.width * fPer_W + 0.5;
// 				if (nMid_modelH < rcFist.rt.height * fPer_H + 0.5)	nMid_modelH = rcFist.rt.height * fPer_H + 0.5;
// 				nMid_minW = nMid_modelW * (1 - fOffset);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
// 				nMid_maxW = nMid_modelW * (1 + fOffset * 4) + 0.5;		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
// 				nMid_minH = nMid_modelH * (1 - fOffset);				//同上
// 				nMid_maxH = nMid_modelH * (1 + fOffset * 4) + 0.5;		//同上
// 
// 				nHead_minW = rcFist.rt.width * (1 - fOffset);		//两端同步头(第一个或最后一个)宽度与两端中间同步头宽度的偏差不超过模板同步头宽度的0.2
// 				nHead_maxW = rcFist.rt.width * (1 + fOffset * 4) + 0.5;		//同上
// 				nHead_minH = rcFist.rt.height * (1 - fOffset);				//同上
// 				nHead_maxH = rcFist.rt.height * (1 + fOffset * 4) + 0.5;	//同上
			}
			else
			{
				float fOffset_mid = 0.4;
				nMid_minW = rcSecond.rt.width * (1 - fOffset_mid);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
				nMid_maxW = rcSecond.rt.width * (1 + fOffset_mid);		//中间同步头宽度与模板中间同步头宽度的偏差不超过模板同步头宽度的0.2
				nMid_minH = rcSecond.rt.height * (1 - fOffset_mid);		//同上
				nMid_maxH = rcSecond.rt.height * (1 + fOffset_mid);		//同上

				float fOffset_Head = 0.3;
				nHead_minW = rcFist.rt.width * (1 - fOffset_Head);		//两端同步头(第一个或最后一个)宽度与两端中间同步头宽度的偏差不超过模板同步头宽度的0.2
				nHead_maxW = rcFist.rt.width * (1 + fOffset_Head);		//同上
				nHead_minH = rcFist.rt.height * (1 - fOffset_Head);		//同上
				nHead_maxH = rcFist.rt.height * (1 + fOffset_Head);		//同上
			}
#endif
			int nXMidSum = 0;	//x轴中线坐标总和
			for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++/*更新迭代索引*/)
			{
				CvRect aRect = cvBoundingRect(contour, 0);
				Rect rm = aRect;
				rm.x = rm.x + rc.rt.x;
				rm.y = rm.y + rc.rt.y;

				if (rm.width < nMid_minW || rm.height < nMid_minH || rm.width > nMid_maxW || rm.height > nMid_maxH)
				{
					if (!(rm.width > nHead_minW && rm.width < nHead_maxW && rm.height > nHead_minH && rm.height < nHead_maxH))	//排除第一个或最后一个大的同步头
					{
						TRACE("过滤垂直同步头(%d,%d,%d,%d), 要求范围W:[%d,%d], H[%d,%d], 参考大小(%d,%d)\n", rm.x, rm.y, rm.width, rm.height, nMid_minW, nMid_maxW, nMid_minH, nMid_maxH, rcSecond.rt.width, rcSecond.rt.height);
						continue;
					}
					else
					{
						TRACE("首尾垂直同步头(即定位点同步头)(%d,%d,%d,%d)\n", rm.x, rm.y, rm.width, rm.height);
					}
				}
				RectCompList.push_back(rm);
				nXMidSum += (rm.x + rm.width / 2);
			}
			cvReleaseMemStorage(&storage);
			int nXMean = nXMidSum / RectCompList.size();
#else
			for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++/*更新迭代索引*/)
			{
				CvRect aRect = cvBoundingRect(contour, 0);
				Rect rm = aRect;
				rm.x = rm.x + rc.rt.x;
				rm.y = rm.y + rc.rt.y;
				if (rm.width < 10 || rm.height < 7 || rm.width > 70 || rm.height > 50 || rm.area() < 70)
				{
					continue;
				}
				RectCompList.push_back(rm);
			}
#endif
		}
		catch (cv::Exception& exc)
		{
			std::string strLog = "识别垂直同步头异常: " + exc.msg;
			g_Log.LogOut(strLog);
			TRACE(strLog.c_str());

			bResult = false;
			break;
		}
		if (RectCompList.size() == 0)
		{
			bResult = false;
			strErrDesc = "垂直同步头数量为0.";
		}
		else
		{
			//二次过滤
			std::sort(RectCompList.begin(), RectCompList.end(), SortByPositionY2);
			std::vector<Rect>::iterator itHead = RectCompList.begin();
			for (int i = 0; itHead != RectCompList.end(); i++)
			{
				RECTINFO rcHead1;
				RECTLIST::iterator itModelHead = pModelInfo->pModel->vecPaperModel[nPic]->lV_Head.begin();
				for (int j = 0; itModelHead != pModelInfo->pModel->vecPaperModel[nPic]->lV_Head.end(); j++, itModelHead++)
				{
					if (j == i)
					{
						rcHead1 = *itModelHead;
						break;
					}
				}
				RECTINFO rcTmp = rcHead1;
				rcTmp.rt = RectCompList[i];
				Recog(nPic, rcTmp, matCompPic, pPic, pModelInfo);
				if (rcTmp.fRealArea / rcTmp.fStandardArea < 0.7 || rcTmp.fRealDensity / rcTmp.fStandardDensity < 0.8)
				{
					itHead = RectCompList.erase(itHead);
					i = i - 1;
				}
				else
					itHead++;
			}
			for (int i = 0; i < RectCompList.size(); i++)
			{
				RECTINFO rcHead;
				rcHead.rt = RectCompList[i];

				rcHead.nGaussKernel = rc.nGaussKernel;
				rcHead.nSharpKernel = rc.nSharpKernel;
				rcHead.nCannyKernel = rc.nCannyKernel;
				rcHead.nDilateKernel = rc.nDilateKernel;

				rcHead.eCPType = V_HEAD;
				m_vecV_Head.push_back(rcHead);
				//++ for test
				pPic->lNormalRect.push_back(rcHead);
				//--
			}
			std::sort(m_vecV_Head.begin(), m_vecV_Head.end(), SortByPositionY);
		}
		if(m_vecV_Head.size() != pModelInfo->pModel->vecPaperModel[nPic]->lV_Head.size())
		{
			bResult = false;
			pPic->bFindIssue = true;
			for(int i = 0; i < m_vecV_Head.size(); i++)
				pPic->lIssueRect.push_back(m_vecV_Head[i]);

			strErrDesc = Poco::format("垂直同步头数量为%u, 与模板垂直同步头数量(%u)不一致", m_vecV_Head.size(), pModelInfo->pModel->vecPaperModel[nPic]->lV_Head.size());
		}
#else
		GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic].lFix, rc.rt);
		bool bFindRect = Recog(nPic, rc, matCompPic, pPic, pModelInfo);
		if (bFindRect)
		{
			bResult = false;						//找到问题点
			pPic->bFindIssue = true;
			pPic->lIssueRect.push_back(rc);
		}
		else
			pPic->lNormalRect.push_back(rc);
#endif
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别垂直同步头失败, 原因: %s, 图片名: %s\n", strErrDesc.c_str(), pPic->strPicName.c_str());
		g_Log.LogOut(szLog);
		TRACE(szLog);
	}
	return bResult;
}

bool CRecognizeThread::RecogABModel(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo)
{
	TRACE("识别AB卷型\n");
	bool bResult = true;
	RECTLIST::iterator itCP = pModelInfo->pModel->vecPaperModel[nPic]->lABModel.begin();
	for (; itCP != pModelInfo->pModel->vecPaperModel[nPic]->lABModel.end(); itCP++)
	{
		RECTINFO rc = *itCP;

		if (pModelInfo->pModel->nHasHead)
		{
			if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
			{
				bResult = false;
				pPic->bFindIssue = true;
				pPic->lIssueRect.push_back(rc);
				break;
			}
			rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
			rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
			rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
			rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
		}
		else
			GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);
		bool bFindRect = Recog(nPic, rc, matCompPic, pPic, pModelInfo);
		if (bFindRect)
		{
			bResult = false;						//找到问题点
			pPic->bFindIssue = true;
			pPic->lIssueRect.push_back(rc);
		}
		else
			pPic->lNormalRect.push_back(rc);
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别AB卷型失败, 图片名: %s\n", pPic->strPicName.c_str());
		g_Log.LogOut(szLog);
		TRACE(szLog);
	}
	return bResult;
}

bool CRecognizeThread::RecogCourse(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, int nRecogMode, std::string& strLog)
{
	TRACE("识别科目\n");
	clock_t start, end;
	start = clock();
	//strLog = Poco::format("图片%s\n", pPic->strPicName);

	bool bResult = true;
	RECTLIST::iterator itCP = pModelInfo->pModel->vecPaperModel[nPic]->lCourse.begin();
	for (; itCP != pModelInfo->pModel->vecPaperModel[nPic]->lCourse.end(); itCP++)
	{
		RECTINFO rc = *itCP;

		if (pModelInfo->pModel->nHasHead)
		{
			if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
			{
				bResult = false;
				pPic->bFindIssue = true;
				pPic->lIssueRect.push_back(rc);
				break;
			}
			rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
			rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
			rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
			rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
		}
		else
			GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);
		bool bResult_Recog = Recog(nPic, rc, matCompPic, pPic, pModelInfo);
		if (bResult_Recog)
		{
			if (rc.fRealValuePercent >= rc.fStandardValuePercent)				//**************	如果科目是由多个校验点组合构成，需要另外考虑	*******************
			{
				pPic->lNormalRect.push_back(rc);
				bResult = true;
				continue;
			}
			else
			{
				char szLog[MAX_PATH] = { 0 };
				sprintf_s(szLog, "识别科目失败, 校验失败, 灰度百分比: %f, 问题点: (%d,%d,%d,%d)\n", rc.fRealValuePercent * 100, rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
				strLog.append(szLog);
				TRACE(szLog);
			}
		}
		else
		{
			char szLog[MAX_PATH] = { 0 };
			sprintf_s(szLog, "识别科目失败, 校验失败, 异常结束, 问题点: (%d,%d,%d,%d)\n", rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
			strLog.append(szLog);
			TRACE(szLog);
		}
		
		pPic->bFindIssue = true;
		pPic->lIssueRect.push_back(rc);
		if (nRecogMode == 2)
		{
			bResult = false;						//找到问题点
			break;		//发现问题点时，继续判断后面的点，不停止扫描
		}
	}
	if (!bResult)
	{
		(static_cast<pST_PaperInfo>(pPic->pPaper))->bRecogCourse = false;
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别科目失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		TRACE(szLog);
	}
	end = clock();
#ifdef PrintRunTime
	std::string strTime = Poco::format("识别科目校验点时间: %dms\n", (int)(end - start));
	strLog.append(strTime);
#endif
	//g_Log.LogOut(strLog);
	return bResult;
}

bool CRecognizeThread::RecogQKCP(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, int nRecogMode, std::string& strLog)
{
	TRACE("识别缺考\n");
	bool bResult = true;
	RECTLIST::iterator itCP = pModelInfo->pModel->vecPaperModel[nPic]->lQK_CP.begin();
	for (; itCP != pModelInfo->pModel->vecPaperModel[nPic]->lQK_CP.end(); itCP++)
	{
		RECTINFO rc = *itCP;

		if (pModelInfo->pModel->nHasHead)
		{
			if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
			{
				bResult = false;
				pPic->bFindIssue = true;
				pPic->lIssueRect.push_back(rc);
				break;
			}
			rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
			rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
			rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
			rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
		}
		else
			GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);
		bool bResult_Recog = Recog(nPic, rc, matCompPic, pPic, pModelInfo);
		if (bResult_Recog)
		{
			if (rc.fRealValuePercent >= rc.fStandardValuePercent)
			{
				((pST_PaperInfo)pPic->pPaper)->nQKFlag = 1;			//设置学生缺考
			}
			pPic->lNormalRect.push_back(rc);
			bResult = true;
			continue;
		}
		else
		{
			char szLog[MAX_PATH] = { 0 };
			sprintf_s(szLog, "识别缺考失败, 校验失败, 异常结束, 问题点: (%d,%d,%d,%d)\n", rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
			strLog.append(szLog);
			//g_Log.LogOut(szLog);
			TRACE(szLog);
		}

		pPic->bFindIssue = true;
		pPic->lIssueRect.push_back(rc);
		if (nRecogMode == 2)
		{
			bResult = false;						//找到问题点
			break;
		}
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别缺考失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		//g_Log.LogOut(szLog);
		TRACE(szLog);
	}
	return bResult;
}

bool CRecognizeThread::RecogWJCP(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, int nRecogMode, std::string& strLog)
{
	TRACE("识别违纪\n");
	bool bResult = true;
	RECTLIST::iterator itCP = pModelInfo->pModel->vecPaperModel[nPic]->lWJ_CP.begin();
	for (; itCP != pModelInfo->pModel->vecPaperModel[nPic]->lWJ_CP.end(); itCP++)
	{
		RECTINFO rc = *itCP;

		if (pModelInfo->pModel->nHasHead)
		{
			if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
			{
				bResult = false;
				pPic->bFindIssue = true;
				pPic->lIssueRect.push_back(rc);
				break;
			}
			rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
			rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
			rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
			rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
		}
		else
			GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);
		bool bResult_Recog = Recog(nPic, rc, matCompPic, pPic, pModelInfo);
		if (bResult_Recog)
		{
			if (rc.fRealValuePercent >= rc.fStandardValuePercent)
			{
				((pST_PaperInfo)pPic->pPaper)->nWJFlag = 1;			//设置学生缺考
			}
			pPic->lNormalRect.push_back(rc);
			bResult = true;
			continue;
		}
		else
		{
			char szLog[MAX_PATH] = { 0 };
			sprintf_s(szLog, "校验失败, 异常结束, 问题点: (%d,%d,%d,%d)\n", rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
			strLog.append(szLog);
			//g_Log.LogOut(szLog);
			TRACE(szLog);
		}

		pPic->bFindIssue = true;
		pPic->lIssueRect.push_back(rc);
		if (nRecogMode == 2)
		{
			bResult = false;						//找到问题点
			break;
		}
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别违纪失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		//g_Log.LogOut(szLog);
		TRACE(szLog);
	}
	return bResult;
}

bool CRecognizeThread::RecogGrayCP(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, int nRecogMode, std::string& strLog)
{
	TRACE("识别灰度点\n");

	clock_t start, end;
	start = clock();
	//strLog = Poco::format("图片%s\n", pPic->strPicName);

	bool bResult = true;
	RECTLIST::iterator itCP = pModelInfo->pModel->vecPaperModel[nPic]->lGray.begin();
	for (; itCP != pModelInfo->pModel->vecPaperModel[nPic]->lGray.end(); itCP++)
	{
		RECTINFO rc = *itCP;

		if (pModelInfo->pModel->nHasHead)
		{
			if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
			{
				bResult = false;
				pPic->bFindIssue = true;
				pPic->lIssueRect.push_back(rc);
				break;
			}
			rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
			rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
			rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
			rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
		}
		else
			GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);
		bool bResult_Recog = Recog(nPic, rc, matCompPic, pPic, pModelInfo);
		if (bResult_Recog)
		{
			if (rc.fRealValuePercent >= rc.fStandardValuePercent)
			{
				pPic->lNormalRect.push_back(rc);
				bResult = true;
				continue;
			}
			else
			{
				char szLog[MAX_PATH] = { 0 };
				sprintf_s(szLog, "校验失败, 灰度百分比: %f, 问题点: (%d,%d,%d,%d)\n", rc.fRealValuePercent * 100, rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
				strLog.append(szLog);
				TRACE(szLog);
			}
		}
		else
		{
			char szLog[MAX_PATH] = { 0 };
			sprintf_s(szLog, "校验失败, 异常结束, 问题点: (%d,%d,%d,%d)\n", rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
			strLog.append(szLog);
			TRACE(szLog);
		}

		pPic->bFindIssue = true;
		pPic->lIssueRect.push_back(rc);
		if (nRecogMode == 2)
		{
			bResult = false;						//找到问题点
			break;
		}
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别灰度校验点失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		TRACE(szLog);
	}
	end = clock();
#ifdef PrintRunTime
	std::string strTime = Poco::format("识别灰度校验点时间: %dms\n", (int)(end - start));
	strLog.append(strTime);
#endif
	//g_Log.LogOut(strLog);
	return bResult;
}

bool CRecognizeThread::RecogWhiteCP(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, int nRecogMode, std::string& strLog)
{
	TRACE("recog WhiteCP: %s\n", pPic->strPicName.c_str());

	bool bResult = true;
	float fGrayCount = 0.0;
	float fGrayModel = 0.0;
	RECTLIST::iterator itCP = pModelInfo->pModel->vecPaperModel[nPic]->lWhite.begin();
	for (; itCP != pModelInfo->pModel->vecPaperModel[nPic]->lWhite.end(); itCP++)
	{
		RECTINFO rc = *itCP;

		if (pModelInfo->pModel->nHasHead)
		{
			if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
			{
				bResult = false;
				pPic->bFindIssue = true;
				pPic->lIssueRect.push_back(rc);
				break;
			}
			rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
			rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
			rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
			rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
		}
		else
			GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);
		bool bResult_Recog = Recog(nPic, rc, matCompPic, pPic, pModelInfo);

		fGrayCount += rc.fRealMeanGray;		//单块的空白区域的平均灰度值
		fGrayModel += itCP->fStandardMeanGray;	//模板上的单块的空白区域的平均灰度值

		if (bResult_Recog)
		{
			if (rc.fRealValuePercent >= rc.fStandardValuePercent)
			{
				pPic->lNormalRect.push_back(rc);
				bResult = true;
				continue;
			}
			else
			{
				char szLog[MAX_PATH] = { 0 };
				sprintf_s(szLog, "校验失败, 灰度百分比: %f, 问题点: (%d,%d,%d,%d)\n", rc.fRealValuePercent * 100, rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
				strLog.append(szLog);
				//g_Log.LogOut(szLog);
				TRACE(szLog);
			}
		}
		else
		{
			char szLog[MAX_PATH] = { 0 };
			sprintf_s(szLog, "校验失败, 异常结束, 问题点: (%d,%d,%d,%d)\n", rc.rt.x, rc.rt.y, rc.rt.width, rc.rt.height);
			strLog.append(szLog);
			//g_Log.LogOut(szLog);
			TRACE(szLog);
		}
		
		pPic->bFindIssue = true;
		pPic->lIssueRect.push_back(rc);
		if (nRecogMode == 2)
		{
			bResult = false;						//找到问题点
			break;
		}
	}

	int nWhiteAreaCount = pModelInfo->pModel->vecPaperModel[nPic]->lWhite.size();
	if (nWhiteAreaCount > 0)
	{
		pPic->fWhiteAreaGray = fGrayCount / nWhiteAreaCount;
		pPic->fWhiteAreaGrayModel = fGrayModel / nWhiteAreaCount;

		if (g_nRecogWithContract)
		{
			float fDiff = pPic->fWhiteAreaGrayModel - pPic->fWhiteAreaGray;
			m_nContract = 100 + fDiff;
			m_nBright = (200 + fDiff) * (1 - m_nContract / 100.0);
		}
	}

	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别空白校验点失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		//g_Log.LogOut(szLog);
		TRACE(szLog);
	}
	return bResult;
}

bool CRecognizeThread::RecogSN(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, std::string& strLog)
{
	TRACE("recog SN: %s\n", pPic->strPicName.c_str());

	bool bResult = true;
	if (pModelInfo->pModel->nZkzhType == 2)
		bResult = RecogSn_code(nPic, matCompPic, pPic, pModelInfo, strLog);
	else
		bResult = RecogSn_omr(nPic, matCompPic, pPic, pModelInfo, strLog);
	return bResult;
}

bool CRecognizeThread::RecogOMR(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, std::string& strLog)
{
	TRACE("recog Omr: %s\n", pPic->strPicName.c_str());

	int nNullCount = 0;
	int nDoubtCount = 0;
	int nEqualCount = 0;
	int nNullCount_1 = 0;	//第一种方法识别出的空值
	int nNullCount_2 = 0;	//第二种方法识别出的空值
	int nNullCount_3 = 0;	//第三种方法识别出的空值

	clock_t start, end;
	start = clock();
	//strLog = Poco::format("图片%s\n", pPic->strPicName);

	std::string strTmpTime;

	bool bRecogAll = true;
	bool bResult = true;
	std::vector<int> vecOmr;
	OMRLIST::iterator itOmr = pModelInfo->pModel->vecPaperModel[nPic]->lOMR2.begin();
	for (; itOmr != pModelInfo->pModel->vecPaperModel[nPic]->lOMR2.end(); itOmr++)
	{
		pOMR_QUESTION pOmrQuestion = &(*itOmr);
		
		OMR_RESULT omrResult;
		omrResult.nTH = pOmrQuestion->nTH;
		omrResult.nSingle = pOmrQuestion->nSingle;
		omrResult.nPageId = nPic + 1;

		int nTmpTime[6] = { 0 };	//计算每步时间
		clock_t sTime, eTime;
		sTime = clock();

		std::vector<int> vecVal_calcHist;		//直方图灰度计算的识别结果
		std::vector<int> vecVal_threshold;		//二值化计算的识别结果
		RECTLIST::iterator itOmrItem = pOmrQuestion->lSelAnswer.begin();
		for (; itOmrItem != pOmrQuestion->lSelAnswer.end(); itOmrItem++)
		{
			RECTINFO rc = *itOmrItem;

			if (pModelInfo->pModel->nHasHead)
			{
				if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
				{
					bResult = false;
					pPic->bFindIssue = true;
					pPic->lIssueRect.push_back(rc);
					break;
				}
				rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
				rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
				rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
				rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
			}
			else
				GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);

			bool bResult_Recog = Recog2(nPic, rc, matCompPic, pPic, pModelInfo);
			if (bResult_Recog)
			{
				if (rc.fRealValuePercent > _dAnswerSure_DensityFix_)	//rc.fStandardValuePercent
				{
					vecVal_calcHist.push_back(rc.nAnswer);
				}
			}

// 			bool bResult_Recog2 = RecogVal(nPic, rc, matCompPic, pPic, pModelInfo);
// 			if (bResult_Recog2)
// 			{
// 				vecVal_threshold.push_back(rc.nAnswer);
// 			}
			omrResult.lSelAnswer.push_back(rc);

			#ifdef PaintOmrSnRect	//打印OMR、SN位置
			pPic->lNormalRect.push_back(rc);
			#endif
		}

		eTime = clock();
		nTmpTime[0] = eTime - sTime;
		sTime = clock();

	#if 1
		std::string strRecogAnswer;
		std::string strRecogAnswer1;
		std::vector<pRECTINFO> vecItemsDesc;
		std::vector<ST_ITEM_DIFF> vecOmrItemDiff;
		calcOmrDensityDiffVal(omrResult.lSelAnswer, vecItemsDesc, vecOmrItemDiff);

		float fCompThread = 0.0;		//密度间隔达到要求时，第一个选项的密度必须达到的要求
		float fDiffThread = 0.0;		//选项可能填涂的可能密度梯度阀值
		float fDiffExit = 0;			//密度的梯度递减太快时，可以认为后面选项没有填涂，此时的密度梯度阀值
		if (pModelInfo->pModel->nHasHead)
		{
			fCompThread = _dCompThread_Head_;
			fDiffThread = _dDiffThread_Head_;
			fDiffExit = _dDiffExit_Head_;
		}
		else
		{
			fCompThread = _dCompThread_Fix_;
			fDiffThread = _dDiffThread_Fix_;
			fDiffExit = _dDiffExit_Fix_;
		}

		int nMaybeAnswer = 0;	//可能的答案数
		for (int j = 0; j < vecItemsDesc.size(); j++)
		{
			if (vecItemsDesc[j]->fRealValuePercent > fCompThread)
				nMaybeAnswer++;
		}

		float fDensityMeanPer2 = 0.0;	//密度比值，与模板的密度比值
		float fRealMeanDensity = 0.0;	//实际选项的密度值，不是与模板的比值
		for (int i = 0; i < vecItemsDesc.size(); i++)
		{
			fDensityMeanPer2 += vecItemsDesc[i]->fRealValuePercent; 
			fRealMeanDensity += vecItemsDesc[i]->fRealDensity;
		}
		fDensityMeanPer2 = fDensityMeanPer2 / vecItemsDesc.size();
		fRealMeanDensity = fRealMeanDensity / vecItemsDesc.size();

		float fDensityMeanPer = 0.0;
		for (int i = 0; i < vecOmrItemDiff.size(); i++)
			fDensityMeanPer += vecOmrItemDiff[i].fDiff;
		fDensityMeanPer = fDensityMeanPer / vecOmrItemDiff.size();
		
		//++2017.12.6 灰度部分
		std::vector<pRECTINFO> vecItemsGrayDesc;
		std::vector<ST_ITEM_DIFF> vecOmrItemGrayDiff;
		calcOmrGrayDiffVal(omrResult.lSelAnswer, vecItemsGrayDesc, vecOmrItemGrayDiff);
		float fMeanGrayDiff = 0.0;		//与模板灰度的平均灰度差
		for (int i = 0; i < vecItemsGrayDesc.size(); i++)
		{
			fMeanGrayDiff += (vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray);
		}
		fMeanGrayDiff = fMeanGrayDiff / vecItemsGrayDesc.size();
		float fMeanGrayDiff2 = 0.0;		//选项间的平均灰度差
		for (int i = 0; i < vecOmrItemGrayDiff.size(); i++)
			fMeanGrayDiff2 += vecOmrItemGrayDiff[i].fDiff;
		fMeanGrayDiff2 = fMeanGrayDiff2 / vecOmrItemGrayDiff.size();
		//--

		int nFlag = -1;
		float fThreld = 0.0;
		float fDensityThreshold = 0.0;
		bool bDiffExit = false;		//判断过程中是否存在直接退出判断的情况
		for (int i = 0; i < vecOmrItemDiff.size(); i++)
		{
#if 1	//2017.9.23 for test
			float fGrayThresholdGray = -(vecOmrItemDiff[i].fDiff - fDensityMeanPer);
			float fGrayMean1 = vecItemsDesc[i]->fRealValuePercent - fDensityMeanPer2;
			float fGrayMean12 = -(vecItemsDesc[i + 1]->fRealValuePercent - fDensityMeanPer2);
			if ((vecOmrItemDiff[i].fDiff >= fDiffThread + vecOmrItemDiff[i].fDiff * 0.5 + fGrayThresholdGray * 0.5 + fDensityThreshold))
			{
				nFlag = i;
				fThreld = vecOmrItemDiff[i].fFirst;
				fDensityThreshold += (fDiffThread + vecOmrItemDiff[i].fDiff * 0.5 + fGrayThresholdGray * 0.5 + fDensityThreshold) / 2;
			#ifdef Test_RecogFirst_NoThreshord
				if (vecOmrItemDiff[i].fDiff > fDiffExit)	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
				{
					bDiffExit = true;
					break;
				}
			#endif
			}
			else
				fDensityThreshold += (fDiffThread + vecOmrItemDiff[i].fDiff * 0.5 + fGrayThresholdGray * 0.5 + fDensityThreshold) / 2;		//++ 2017.9.7
#else
			//根据所有选项灰度值排序，相邻灰度值差值超过阀值，同时其中第一个最大的灰度值超过1.0，就认为这个区间为选中的阀值区间
			//(大于1.0是防止最小的灰度值很小的时候影响阀值判断)
			float fDiff = (fCompThread - vecOmrItemDiff[i].fFirst) * 0.5;
			if ((vecOmrItemDiff[i].fDiff >= fDiffThread + fDensityThreshold * 0.5 ))
			{
				nFlag = i;
				fThreld = vecOmrItemDiff[i].fFirst;
				fDensityThreshold += vecOmrItemDiff[i].fDiff;
			#ifdef Test_RecogFirst_NoThreshord
				if (vecOmrItemDiff[i].fDiff > fDiffExit)	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
					break;
			#else
				if (vecOmrItemDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_calcHist.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
					break;
			#endif
			}
			else
				fDensityThreshold += vecOmrItemDiff[i].fDiff;		//++ 2017.9.7
#endif
		}
		if (nFlag < 0 && nMaybeAnswer == 1)	//通过上面的方法没有识别到选项，但是存在差值 > 比较差值(0.2)的选项，即第1项的差值大于比较差值
		{
			if (vecOmrItemDiff[0].fDiff > fDiffThread && vecItemsDesc[0]->fRealMeanGray < _dCompThread_3_)
			{
				nFlag = 0;
				fThreld = vecOmrItemDiff[0].fFirst;
			}
		}
		if (nFlag >= 0)
		{
			//++判断全选的情况
			if (nFlag == vecOmrItemDiff.size() - 1)
			{
				float fGrayThresholdGray = -(vecOmrItemDiff[nFlag].fDiff - fDensityMeanPer);
				float fGrayMean1 = vecItemsDesc[nFlag]->fRealValuePercent - fDensityMeanPer2;
				float fGrayMean12 = -(vecItemsDesc[nFlag + 1]->fRealValuePercent - fDensityMeanPer2);
				if ((vecItemsDesc[nFlag + 1]->fRealValuePercent >= fDensityMeanPer2 - fDiffThread) && (vecOmrItemDiff[nFlag].fDiff < fDiffThread))
					fThreld = vecItemsDesc[nFlag + 1]->fRealValuePercent;
// 				if (vecItemsDesc[vecOmrItemDiff.size()]->fRealValuePercent >= fCompThread + fDiffExit)	//如果密度最低的选项，它的密度大于“最低比较密度 + 最大退出密度差”，则认为全选
// 					fThreld = vecItemsDesc[vecOmrItemDiff.size()]->fRealValuePercent;
			}
			//--

			if (!bDiffExit && nMaybeAnswer > 0 && nFlag < nMaybeAnswer - 1)	//通过上面的方法没有识别到选项，但是存在差值 > 比较差值(0.2)的选项，即第1项的差值大于比较差值
			{
				int nThreld1 = _dDiffExit_3_ + _dAnswerSure_;
				int nThreld2 = (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2;
				int nFlag1 = -1;
				int nFlag2 = -1;
				int nFlag3 = -1;
				for (int i = 0; i < vecItemsDesc.size(); i++)
				{
					if (vecItemsDesc[i]->fRealValuePercent > fCompThread && vecItemsDesc[i]->fRealMeanGray < max(nThreld1, nThreld2))
					{
						nFlag1 = i;
					}
				}
				for (int i = 0; i < vecOmrItemDiff.size(); i++)
				{
					if (vecOmrItemDiff[i].fDiff > fDiffThread && vecItemsDesc[i]->fRealMeanGray < _dCompThread_3_)
					{
						nFlag2 = i;
					}
				}
				if (nFlag1 == nFlag2 && nFlag1 != -1)
				{
					nFlag3 = nFlag1;
				}
				else if (nFlag1 > nFlag2)
				{
					nFlag3 = nFlag1;
				}
				else if (nFlag1 < nFlag2)
				{
					nFlag3 = nFlag2;
				}

				if (nFlag < nFlag3)		//nFlag >= nFlag3时，保留默认判断值
				{
					nFlag = nFlag3;
					fThreld = vecItemsDesc[nFlag]->fRealValuePercent;
				}
			}

			//根据答案确定灰度再次判断
			//如果根据灰度值直接判断确定答案的数量 > 灰度差值判断出的选项数
			//即：灰度差判断出1个，直接答案判定数为3个，则再次判断，可针对全涂或者扫描很深的情况
			//若有答案退出判断标志，则不参与此判断，因存在此标志时，一般密度差较大
			if ((nFlag != vecOmrItemDiff.size() - 1) && (vecVal_calcHist.size() > nFlag + 1))
			{
				int nThreld1 = _dCompThread_3_;
				int nThreld2 = (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2;
				for (int i = 0; i < vecVal_calcHist.size(); i++)
				{
					//查找此选项对应的矩形信息
				#if 1
					for(int j = 0; j < vecItemsDesc.size(); j++)
					{
						if (vecItemsDesc[j]->nAnswer == vecVal_calcHist[i])
						{
							//选项的灰度<(比较灰度 + 灰度答案确认值 + 灰度退出密度值) / 2，
							//或者选项的密度 - 选项平均密度 > 比较灰度
							//或者选项的实际密度 / 所有选项的平均密度 > 直接答案判定密度1.6
							if (vecItemsDesc[j]->fRealMeanGray < min(nThreld1, nThreld2) || \
								((vecItemsDesc[j]->fRealValuePercent - fDensityMeanPer2 > fDiffThread) && vecItemsDesc[j]->fRealMeanGray < max(nThreld1, nThreld2)) || \
								(vecItemsDesc[j]->fRealDensity / fRealMeanDensity > _dAnswerSure_DensityFix_))
							{
								fThreld = vecItemsDesc[j]->fRealValuePercent > fThreld ? fThreld : vecItemsDesc[j]->fRealValuePercent;
							}
							else
							{
								//实际密度与选项平均密度比值 > 1, 同时与前一项的灰度差低于灰度判定值，
// 								float fGrayDiff = abs(fMeanGrayDiff) > _dDiffThread_3_ ? _dDiffThread_3_ : abs(fMeanGrayDiff);
// 								fGrayDiff = fGrayDiff > fMeanGrayDiff2 ? fMeanGrayDiff2 : fGrayDiff;
								float fGrayDiff = fMeanGrayDiff2 > _dDiffThread_3_ ? _dDiffThread_3_ : fMeanGrayDiff2;
								float fGrayDiff2 = fMeanGrayDiff2 + fMeanGrayDiff2 - _dDiffThread_3_;	//fMeanGrayDiff2 > _dDiffThread_3_
								fGrayDiff2 = fMeanGrayDiff2 > _dDiffThread_3_ ? (fMeanGrayDiff2 + fMeanGrayDiff2 - _dDiffThread_3_) : (_dDiffThread_3_ + abs(fMeanGrayDiff2 - _dDiffThread_3_));
								fGrayDiff2 = fGrayDiff2 > (_dDiffThread_3_ + _dDiffExit_3_) / 2 ? (_dDiffThread_3_ + _dDiffExit_3_) / 2 : fGrayDiff2;
								if (j > 0 && vecItemsDesc[j]->fRealMeanGray - vecItemsDesc[j - 1]->fRealMeanGray < fGrayDiff && vecItemsDesc[j]->fRealDensity / fRealMeanDensity > 1)
								{
									fThreld = vecItemsDesc[j]->fRealValuePercent > fThreld ? fThreld : vecItemsDesc[j]->fRealValuePercent;
								}
// 								else if (j > 0 && vecItemsDesc[j]->fRealMeanGray - vecItemsDesc[j - 1]->fRealMeanGray < fGrayDiff2 && vecItemsDesc[j]->fRealDensity / fRealMeanDensity > 1 && \
// 										 vecOmrItemDiff[j].fDiff > fDensityMeanPer)
// 								{
// 									fThreld = vecItemsDesc[j]->fRealValuePercent > fThreld ? fThreld : vecItemsDesc[j]->fRealValuePercent;
// 								}
							}
							break;
						}
					}
				#else
					for (auto rcItem : omrResult.lSelAnswer)
					{
						if (rcItem.nAnswer == vecVal_calcHist[i])
						{
							if (rcItem.fRealMeanGray < min(nThreld1, nThreld2) || \
								(rcItem.fRealValuePercent - fDensityMeanPer2 > fDiffThread) && rcItem.fRealMeanGray < max(nThreld1, nThreld2))	//选项的灰度<(比较灰度 + 灰度答案确认值 + 灰度退出密度值) / 2，或者选项的密度 - 选项平均密度 > 比较灰度
							{
								fThreld = rcItem.fRealValuePercent > fThreld ? fThreld : rcItem.fRealValuePercent;
							}
							break;
						}
					}
				#endif
				}
			}
			else
			{
				if (omrResult.nSingle != 1 && nFlag > 0 /*&& !bDiffExit*/)	//单选,判断, 识别到多个，没有直接退出判断标志
				{
					for (int i = 0; i < nFlag; i++)
					{
						if (vecOmrItemDiff[i].fDiff < fDiffThread && ((vecItemsDesc[i + 1]->fRealMeanGray < (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2) || \
							(vecItemsDesc[i + 1]->fRealValuePercent - fDensityMeanPer2 > fDiffThread && vecItemsDesc[i + 1]->fRealDensity / fRealMeanDensity > 1)))
						{
							fThreld = vecOmrItemDiff[i].fSecond;
						}
						else if (vecOmrItemDiff[i].fDiff >= fDiffThread && vecOmrItemDiff[i].fSecond > _dAnswerSure_DensityFix_ && fDensityMeanPer2 < _dAnswerSure_DensityFix_ && \
								 abs(vecItemsDesc[i]->fRealMeanGray - vecItemsDesc[i + 1]->fRealMeanGray) < _dDiffExit_3_)
						{
							fThreld = vecOmrItemDiff[i].fSecond;
						}
						else if (vecOmrItemDiff[i].fDiff >= fDiffThread && (vecItemsDesc[i + 1]->fRealMeanGray < (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2 || \
							(abs(vecItemsDesc[i]->fRealMeanGray - vecItemsDesc[i + 1]->fRealMeanGray) < _dDiffThread_3_ && vecItemsDesc[i + 1]->fRealValuePercent > fCompThread) /*|| \*/
							/*(vecItemsDesc[i + 1]->fRealValuePercent - fDensityMeanPer2 >= fDiffExit && (vecItemsDesc[i + 1]->fRealValuePercent - fDensityMeanPer2 >= fDensityMeanPer) && \
							(vecItemsDesc[i + 1] - vecItemsDesc[i + 2]的灰度差 > 平均灰度差) && (vecItemsDesc[i] - vecItemsDesc[i + 1]的灰度差 < 直接退出灰度)*/))
						{
							fThreld = vecOmrItemDiff[i].fSecond;
						}
						else
						{
							fThreld = vecOmrItemDiff[i].fFirst;
						}
					}
				}
			}

			RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
			for (; itItem != omrResult.lSelAnswer.end(); itItem++)
			{
				if (itItem->fRealValuePercent >= fThreld)
				{
					char szVal[10] = { 0 };
					sprintf_s(szVal, "%c", itItem->nAnswer + 65);
					strRecogAnswer1.append(szVal);
				}
			}
		}
		else if (vecItemsDesc[vecOmrItemDiff.size()]->fRealValuePercent >= fCompThread + fDiffExit)	//如果密度最低的选项，它的密度大于“最低比较密度 + 最大退出密度差”，则认为全选
		{
			fThreld = vecItemsDesc[vecOmrItemDiff.size()]->fRealValuePercent;

			RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
			for (; itItem != omrResult.lSelAnswer.end(); itItem++)
			{
				if (itItem->fRealValuePercent >= fThreld)
				{
					char szVal[10] = { 0 };
					sprintf_s(szVal, "%c", itItem->nAnswer + 65);
					strRecogAnswer1.append(szVal);
				}
			}
		}
		else if (vecItemsDesc[0]->fRealValuePercent >= fCompThread && vecItemsDesc[vecOmrItemDiff.size()]->fRealValuePercent < (fCompThread + fDiffExit > _dAnswerSure_DensityFix_ ? fCompThread + fDiffExit : _dAnswerSure_DensityFix_))
		{
			int nFlag = -1;
			float fThreld = 0.0;
			for (int i = 0; i < vecOmrItemDiff.size(); i++)
			{
				//根据所有选项灰度值排序，相邻灰度值差值超过阀值，同时其中第一个最大的灰度值超过1.0，就认为这个区间为选中的阀值区间
				//(大于1.0是防止最小的灰度值很小的时候影响阀值判断)
				float fDiff = (fCompThread - vecOmrItemDiff[i].fFirst) * 0.1;
				if ((vecOmrItemDiff[i].fDiff >= fDiffThread && vecOmrItemDiff[i].fFirst > fCompThread) ||
					(vecOmrItemDiff[i].fDiff >= fDiffThread + fDiff && (vecOmrItemDiff[i].fFirst > (fCompThread - 0.1) && fDiff > 0 && vecItemsDesc[i]->fRealMeanGray < _dCompThread_3_)))
				{
					nFlag = i;
					fThreld = vecOmrItemDiff[i].fFirst;
					if (vecOmrItemDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_calcHist.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
					{
						bDiffExit = true;
						break;
					}
				}
			}
			if (!bDiffExit && nMaybeAnswer > 0 && nFlag < nMaybeAnswer - 1)	//通过上面的方法没有识别到选项，但是存在差值 > 比较差值(0.2)的选项，即第1项的差值大于比较差值
			{
				int nThreld1 = _dDiffExit_3_ + _dAnswerSure_;
				int nThreld2 = (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2;
				int nFlag1 = -1;
				int nFlag2 = -1;
				int nFlag3 = -1;
				for (int i = 0; i < vecItemsDesc.size(); i++)
				{
					if (vecItemsDesc[i]->fRealValuePercent > fCompThread && vecItemsDesc[i]->fRealMeanGray < max(nThreld1, nThreld2))
					{
						nFlag1 = i;
					}
				}
				for (int i = 0; i < vecOmrItemDiff.size(); i++)
				{
					if (vecOmrItemDiff[i].fDiff > fDiffThread && vecItemsDesc[i]->fRealMeanGray < _dCompThread_3_)
					{
						nFlag2 = i;
					}
				}
				if (nFlag1 == nFlag2 && nFlag1 != -1)
				{
					nFlag3 = nFlag1;
				}
				else if (nFlag1 > nFlag2)
				{
					nFlag3 = nFlag1;
				}
				else if (nFlag1 < nFlag2)
				{
					nFlag3 = nFlag2;
				}

				if (nFlag < nFlag3)		//nFlag >= nFlag3时，保留默认判断值
				{
					nFlag = nFlag3;
					fThreld = vecItemsDesc[nFlag]->fRealValuePercent;
				}
			}
			if (omrResult.nSingle != 1 && nFlag > 0 /*&& !bDiffExit*/)	//单选,判断, 识别到多个，没有直接退出判断标志
			{
				for (int i = 0; i < nFlag; i++)
				{
					if (vecOmrItemDiff[i].fDiff < fDiffThread && ((vecItemsDesc[i + 1]->fRealMeanGray < (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2) || \
						(vecItemsDesc[i + 1]->fRealValuePercent - fDensityMeanPer2 > fDiffThread && vecItemsDesc[i + 1]->fRealDensity / fRealMeanDensity > 1)))
					{
						fThreld = vecOmrItemDiff[i].fSecond;
					}
					else if (vecOmrItemDiff[i].fDiff >= fDiffThread && vecOmrItemDiff[i].fSecond > _dAnswerSure_DensityFix_ && fDensityMeanPer2 < _dAnswerSure_DensityFix_)
					{
						fThreld = vecOmrItemDiff[i].fSecond;
					}
					else if (vecOmrItemDiff[i].fDiff >= fDiffThread && (vecItemsDesc[i + 1]->fRealMeanGray < (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2 || \
							(abs(vecItemsDesc[i]->fRealMeanGray - vecItemsDesc[i + 1]->fRealMeanGray) < _dDiffThread_3_ && vecItemsDesc[i + 1]->fRealValuePercent > fCompThread)))
					{
						fThreld = vecOmrItemDiff[i].fSecond;
					}
					else
					{
						fThreld = vecOmrItemDiff[i].fFirst;
					}
				}
			}
			if (nFlag >= 0)
			{
				RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
				for (; itItem != omrResult.lSelAnswer.end(); itItem++)
				{
					if (itItem->fRealValuePercent >= fThreld)
					{
						char szVal[2] = { 0 };
						sprintf_s(szVal, "%c", itItem->nAnswer + 65);
						strRecogAnswer1.append(szVal);
					}
				}
			}
			else if (vecItemsDesc[vecOmrItemDiff.size()]->fRealValuePercent > fCompThread && vecItemsDesc[vecOmrItemDiff.size()]->fRealMeanGray < (_dCompThread_3_ +_dDiffExit_3_ + _dAnswerSure_) / 2)	//全选，最低密度>比较密度，同时此选项的灰度<(比较灰度 + 灰度答案确认值 + 灰度退出密度值)/2
			{
				fThreld = vecItemsDesc[vecOmrItemDiff.size()]->fRealValuePercent;

				RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
				for (; itItem != omrResult.lSelAnswer.end(); itItem++)
				{
					if (itItem->fRealValuePercent >= fThreld)
					{
						char szVal[10] = { 0 };
						sprintf_s(szVal, "%c", itItem->nAnswer + 65);
						strRecogAnswer1.append(szVal);
					}
				}
			}
			else
			{
				for (int i = 0; i < vecVal_calcHist.size(); i++)
				{
					char szVal[5] = { 0 };
					sprintf_s(szVal, "%c", vecVal_calcHist[i] + 65);
					strRecogAnswer1.append(szVal);
				}
			}
		}
		else
		{
			for (int i = 0; i < vecVal_calcHist.size(); i++)
			{
				char szVal[10] = { 0 };
				sprintf_s(szVal, "%c", vecVal_calcHist[i] + 65);
				strRecogAnswer1.append(szVal);
			}
		}
	#else
			std::string strRecogAnswer;
			std::string strRecogAnswer1;
			std::vector<pRECTINFO> vecItemsDesc;
			std::vector<ST_ITEM_DIFF> vecOmrItemDiff;
			calcOmrDensityDiffVal(omrResult.lSelAnswer, vecItemsDesc, vecOmrItemDiff);

			float fCompThread = 0.0;		//密度间隔达到要求时，第一个选项的密度必须达到的要求
			float fDiffThread = 0.0;		//选项可能填涂的可能密度梯度阀值
			float fDiffExit = 0;			//密度的梯度递减太快时，可以认为后面选项没有填涂，此时的密度梯度阀值
			if (pModelInfo->pModel->nHasHead)
			{
				fCompThread = _dCompThread_Head_;
				fDiffThread = _dDiffThread_Head_;
				fDiffExit = _dDiffExit_Head_;
			}
			else
			{
				fCompThread = _dCompThread_Fix_;
				fDiffThread = _dDiffThread_Fix_;
				fDiffExit = _dDiffExit_Fix_;
			}

			int nFlag = -1;
			float fThreld = 0.0;
			for (int i = 0; i < vecOmrItemDiff.size(); i++)
			{
				//根据所有选项灰度值排序，相邻灰度值差值超过阀值，同时其中第一个最大的灰度值超过1.0，就认为这个区间为选中的阀值区间
				//(大于1.0是防止最小的灰度值很小的时候影响阀值判断)
				float fDiff = (fCompThread - vecOmrItemDiff[i].fFirst) * 0.1;
				if ((vecOmrItemDiff[i].fDiff >= fDiffThread && vecOmrItemDiff[i].fFirst > fCompThread) ||
					(vecOmrItemDiff[i].fDiff >= fDiffThread + fDiff && vecOmrItemDiff[i].fFirst > (fCompThread - 0.1) && fDiff > 0))
				{
					nFlag = i;
					fThreld = vecOmrItemDiff[i].fFirst;
					if (vecOmrItemDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_calcHist.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
						break;
				}
			}
			if (nFlag >= 0)
			{
				RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
				for (; itItem != omrResult.lSelAnswer.end(); itItem++)
				{
					if (itItem->fRealValuePercent >= fThreld)
					{
						char szVal[10] = { 0 };
						sprintf_s(szVal, "%c", itItem->nAnswer + 65);
						strRecogAnswer1.append(szVal);
					}
				}
			}
			else
			{
				for (int i = 0; i < vecVal_calcHist.size(); i++)
				{
					char szVal[10] = { 0 };
					sprintf_s(szVal, "%c", vecVal_calcHist[i] + 65);
					strRecogAnswer1.append(szVal);
				}
			}
	#endif
		
		eTime = clock();
		nTmpTime[1] = eTime - sTime;
		sTime = clock();

	#ifdef Test_RecogOmr3
		RecogVal_Omr2(nPic, matCompPic, pPic, pModelInfo, omrResult);

		eTime = clock();
		nTmpTime[2] = eTime - sTime;
		sTime = clock();

		RecogVal_Omr3(nPic, matCompPic, pPic, pModelInfo, omrResult);

		eTime = clock();
		nTmpTime[3] = eTime - sTime;
		sTime = clock();

		std::string strRecogAnswer2 = omrResult.strRecogVal2;
		std::string strRecogAnswer3 = omrResult.strRecogVal3;

		if (strRecogAnswer1 == "") nNullCount_1++;
		if (strRecogAnswer2 == "") nNullCount_2++;
		if (strRecogAnswer3 == "") nNullCount_3++;

		int nDoubt = 0;

	#if 1		//识别方法1与识别方法2、方法3 同时识别出来才认为一致，否则认为有问题
		if (strRecogAnswer1 == strRecogAnswer2 && strRecogAnswer1 == strRecogAnswer3)	//为空判断时，方法1与方法3准确度更高
		{
			if (strRecogAnswer1 == "")
			{
				strRecogAnswer = strRecogAnswer1;
				nDoubt = 2;
				nNullCount++;

				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrNull++;
				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
			}
			else
			{
				strRecogAnswer = strRecogAnswer1;
				nDoubt = 0;
				nEqualCount++;
			}
		}
		else
		{
			if (strRecogAnswer1 == "")
			{
				if (strRecogAnswer2 == strRecogAnswer3)
				{
					strRecogAnswer = strRecogAnswer3;
					nDoubt = 0;
					nEqualCount++;
				}
				else
				{
					//方法2、方法3两种方法有一种方法判断非空
					if (strRecogAnswer2 == "" && strRecogAnswer3 != "")
					{
						strRecogAnswer = strRecogAnswer3;
						nDoubt = 1;
						nDoubtCount++;

						(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
						(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrDoubt++;
						(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
					}
					else if (strRecogAnswer3 == "" && strRecogAnswer2 != "")	//方法1与方法3为空时，认为选项为空
					{
						strRecogAnswer = strRecogAnswer3;
						nDoubt = 2;
						nEqualCount++;
					}
					else if (strRecogAnswer2 != "" && strRecogAnswer3 != "")
					{
						strRecogAnswer = strRecogAnswer3;
						nDoubt = 1;
						nDoubtCount++;

						(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
						(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrDoubt++;
						(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
					}
				}
			}
			else
			{
				if (strRecogAnswer1 == strRecogAnswer2 || strRecogAnswer1 == strRecogAnswer3 || (g_nRecogEasyModel && strRecogAnswer2 == strRecogAnswer3 && strRecogAnswer3 != "")) //strRecogAnswer1 == strRecogAnswer2 || strRecogAnswer1 == strRecogAnswer3 
				{
					strRecogAnswer = strRecogAnswer1;
					nDoubt = 0;
					nEqualCount++;
				}
				else
				{
					strRecogAnswer = strRecogAnswer1;
					nDoubt = 1;
					nDoubtCount++;

					(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
					(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrDoubt++;
					(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
				}
			}
		}
	#else				//识别方法1与识别方法2、方法3 两种方法中的一种识别一致就认为正确，否则认为有问题
		if (strRecogAnswer1 == "" && (strRecogAnswer2 == "" || strRecogAnswer3 == ""))
		{
			strRecogAnswer = strRecogAnswer1;
			nDoubt = 2;
			nNullCount++;

			(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
			(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrNull++;
			(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
		}
		else
		{
			if (strRecogAnswer1 == strRecogAnswer2 || strRecogAnswer1 == strRecogAnswer3)
			{
				strRecogAnswer = strRecogAnswer1;
				nDoubt = 0;
				nEqualCount++;
			}
			else
			{
				strRecogAnswer = strRecogAnswer1;
				nDoubt = 1;
				nDoubtCount++;

				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrDoubt++;
				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
			}
		}
	#endif


	#else
		//++ test	测试整题选项进行二值化识别
		RecogVal_Omr2(nPic, matCompPic, pPic, pModelInfo, omrResult);
		//--
		std::string strRecogAnswer2 = omrResult.strRecogVal2;

		if (strRecogAnswer1 == "") nNullCount_1++;
		if (strRecogAnswer2 == "") nNullCount_2++;

		int nDoubt = 0;
		if (strRecogAnswer1 == "" && strRecogAnswer2 == "")
		{
			nDoubt = 2;
			nNullCount++;

			(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
			(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrNull++;
			(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
		}
		else
		{
			if (strRecogAnswer1 == strRecogAnswer2)
			{
				nDoubt = 0;
				nEqualCount++;
			}
			else
			{
				nDoubt = 1;
				nDoubtCount++;

				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.lock();
				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nOmrDoubt++;
				(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmOmrStatistics.unlock();
			}
		}		
	#endif

		omrResult.nDoubt		= nDoubt;
		omrResult.strRecogVal = strRecogAnswer;
		omrResult.strRecogVal1 = strRecogAnswer1;
		omrResult.strRecogVal2 = strRecogAnswer2;
		omrResult.strRecogVal3 = strRecogAnswer3;

		omrResult.fWhiteAreaGray = pPic->fWhiteAreaGray;
		omrResult.fWhiteAreaGrayModel = pPic->fWhiteAreaGrayModel;
		(static_cast<pST_PaperInfo>(pPic->pPaper))->lOmrResult.push_back(omrResult);


		eTime = clock();
		nTmpTime[4] = eTime - sTime;
		strTmpTime.append(Poco::format("[TH:%d, %d:%d:%d:%d:%d] ", omrResult.nTH, nTmpTime[0], nTmpTime[1], nTmpTime[2], nTmpTime[3], nTmpTime[4]));
	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别OMR失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		TRACE(szLog);
	}

	int nCount = pModelInfo->pModel->vecPaperModel[nPic]->lOMR2.size();

// 	std::stringstream ss;
// 	ss << "总数" << nCount << ",空值" << nNullCount << "(" << (double)nNullCount / nCount * 100 << "%)[No.1=" << nNullCount_1 << "(" << (double)nNullCount_1 / nCount * 100 << "%),No.2=" << nNullCount_2
// 		<< "(" << (double)nNullCount_2 / nCount * 100 << "%)],怀疑" << nDoubtCount << "(" << (double)nDoubtCount / nCount * 100 << "%),无怀疑" << nEqualCount << "(" << (double)nEqualCount / nCount * 100
// 		<< "%)";
// 	std::string strTmp = ss.str();

	if (bResult && nCount)
	{
		char szStatistics[150] = { 0 };
		sprintf_s(szStatistics, "图片(%s)选项总数[%d],空值%d(%.2f%%)[No.1=%d(%.2f%%),No.2=%d(%.2f%%), No.3=%d(%.2f%%)],怀疑%d(%.2f%%),无怀疑%d(%.2f%%)", pPic->strPicName.c_str(), nCount, nNullCount, (float)nNullCount / nCount * 100, \
				  nNullCount_1, (float)nNullCount_1 / nCount * 100, nNullCount_2, (float)nNullCount_2 / nCount * 100, nNullCount_3, (float)nNullCount_3 / nCount * 100,\
				  nDoubtCount, (float)nDoubtCount / nCount * 100, nEqualCount, (float)nEqualCount / nCount * 100);

		strLog.append(szStatistics);
	}	
	end = clock();
#ifdef PrintRunTime
	std::string strTime = Poco::format("识别Omr时间: %dms\n%s\n", (int)(end - start), strTmpTime);
	strLog.append(strTime);
#endif
	//g_Log.LogOut(strLog);
	return bResult;
}

bool CRecognizeThread::RecogElectOmr(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, std::string& strLog)
{
	TRACE("recog ElectOmr: %s\n", pPic->strPicName.c_str());

	bool bResult = true;

	std::vector<int> vecOmr;
	ELECTOMR_LIST::iterator itElectOmr = pModelInfo->pModel->vecPaperModel[nPic]->lElectOmr.begin();
	for (; itElectOmr != pModelInfo->pModel->vecPaperModel[nPic]->lElectOmr.end(); itElectOmr++)
	{
		pELECTOMR_QUESTION pOmrQuestion = &(*itElectOmr);

		ELECTOMR_QUESTION omrResult;
		omrResult.sElectOmrGroupInfo = pOmrQuestion->sElectOmrGroupInfo;

		std::vector<int> vecVal_calcHist;		//直方图灰度计算的识别结果
		std::vector<int> vecVal_threshold;		//二值化计算的识别结果
		RECTLIST::iterator itOmrItem = pOmrQuestion->lItemInfo.begin();
		for (; itOmrItem != pOmrQuestion->lItemInfo.end(); itOmrItem++)
		{
			RECTINFO rc = *itOmrItem;

			if (pModelInfo->pModel->nHasHead)
			{
				if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
				{
					bResult = false;
					pPic->bFindIssue = true;
					pPic->lIssueRect.push_back(rc);
					break;
				}
				rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
				rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
				rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
				rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
			}
			else
				GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);

			bool bResult_Recog = Recog2(nPic, rc, matCompPic, pPic, pModelInfo);
			if (bResult_Recog)
			{
				if (rc.fRealValuePercent > rc.fStandardValuePercent)
				{
					vecVal_calcHist.push_back(rc.nAnswer);
				}
			}

// 			bool bResult_Recog2 = RecogVal(nPic, rc, matCompPic, pPic, pModelInfo);
// 			if (bResult_Recog2)
// 			{
// 				vecVal_threshold.push_back(rc.nAnswer);
// 			}
			omrResult.lItemInfo.push_back(rc);

#ifdef PaintOmrSnRect	//打印OMR、SN位置
			pPic->lNormalRect.push_back(rc);
#endif
		}

#if 1
		std::string strRecogAnswer1;
		std::vector<pRECTINFO> vecItemsDesc;
		std::vector<ST_ITEM_DIFF> vecOmrItemDiff;
		calcOmrDensityDiffVal(omrResult.lItemInfo, vecItemsDesc, vecOmrItemDiff);

		float fCompThread = 0.0;		//灰度间隔达到要求时，第一个选项的灰度必须达到的要求
		float fDiffThread = 0.0;		//选项可能填涂的可能灰度梯度阀值
		float fDiffExit = 0;			//灰度的梯度递减太快时，可以认为后面选项没有填涂，此时的灰度梯度阀值
		if (pModelInfo->pModel->nHasHead)
		{
			fCompThread = _dCompThread_Head_;
			fDiffThread = _dDiffThread_Head_;
			fDiffExit = _dDiffExit_Head_;
		}
		else
		{
			fCompThread = _dCompThread_Fix_;
			fDiffThread = _dDiffThread_Fix_;
			fDiffExit = _dDiffExit_Fix_;
		}

		int nFlag = -1;
		float fThreld = 0.0;
		for (int i = 0; i < vecOmrItemDiff.size(); i++)
		{
			//根据所有选项灰度值排序，相邻灰度值差值超过阀值，同时其中第一个最大的灰度值超过1.0，就认为这个区间为选中的阀值区间
			//(大于1.0是防止最小的灰度值很小的时候影响阀值判断)
			float fDiff = (fCompThread - vecOmrItemDiff[i].fFirst) * 0.1;
			if ((vecOmrItemDiff[i].fDiff >= fDiffThread && vecOmrItemDiff[i].fFirst > fCompThread) ||
				(vecOmrItemDiff[i].fDiff >= fDiffThread + fDiff && vecOmrItemDiff[i].fFirst > (fCompThread - 0.1) && fDiff > 0))
			{
				nFlag = i;
				fThreld = vecOmrItemDiff[i].fFirst;
				if (vecOmrItemDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_calcHist.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
					break;
			}
		}
		if (nFlag >= 0)
		{
			RECTLIST::iterator itItem = omrResult.lItemInfo.begin();
			for (; itItem != omrResult.lItemInfo.end(); itItem++)
			{
				if (itItem->fRealValuePercent >= fThreld)
				{
					char szVal[10] = { 0 };
					sprintf_s(szVal, "%c", itItem->nAnswer + 65);
					strRecogAnswer1.append(szVal);
				}
			}
		}
		else
		{
			for (int i = 0; i < vecVal_calcHist.size(); i++)
			{
				char szVal[10] = { 0 };
				sprintf_s(szVal, "%c", vecVal_calcHist[i] + 65);
				strRecogAnswer1.append(szVal);
			}
		}
#else	//一下是直接通过阀值判断是否选中，可用，对填涂不规范并且不清晰的情况不够理想
		std::string strRecogAnswer1;
		for (int i = 0; i < vecVal_calcHist.size(); i++)
		{
			char szVal[10] = { 0 };
			sprintf_s(szVal, "%c", vecVal_calcHist[i] + 65);
			strRecogAnswer1.append(szVal);
		}
#endif
		std::string strRecogAnswer2 = strRecogAnswer1;		//目前第二种方法不可用
		int nDoubt = 0;
		if (strRecogAnswer1 == "" && strRecogAnswer2 == "")
			nDoubt = 2;
		else
		{
			if (strRecogAnswer1 == strRecogAnswer2)
				nDoubt = 0;
			else
				nDoubt = 1;
		}

		omrResult.nDoubt = nDoubt;
		omrResult.strRecogResult = strRecogAnswer1;
		(static_cast<pST_PaperInfo>(pPic->pPaper))->lElectOmrResult.push_back(omrResult);

	}
	if (!bResult)
	{
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别选做题OMR失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		//g_Log.LogOut(szLog);
		TRACE(szLog);
	}

	return bResult;
}

bool CRecognizeThread::RecogVal(int nPic, RECTINFO& rc, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo)
{
	Mat matSrcRoi, matCompRoi;
	Rect rt = rc.rt;
	bool bResult = false;
	try
	{
		matCompRoi = matCompPic(rt);

		Mat imag_src, img_comp;
		if (matCompRoi.channels() == 3)
			cv::cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

		//图片二值化
		threshold(matCompRoi, matCompRoi, 240, 255, THRESH_BINARY_INV);				//200, 255
#if 0
		//确定腐蚀和膨胀核的大小
		Mat element = getStructuringElement(MORPH_RECT, Size(4, 4));	//Size(4, 4)
		//膨胀操作
		dilate(matCompRoi, matCompRoi, element);
		
		Mat element2 = getStructuringElement(MORPH_RECT, Size(15, 15));	//Size(4, 4)
		//腐蚀操作1
		erode(matCompRoi, matCompRoi, element2);
		
		Mat element3 = getStructuringElement(MORPH_RECT, Size(4, 4));	//Size(4, 4)
		//腐蚀操作2
		erode(matCompRoi, matCompRoi, element3);
		
		Mat element4 = getStructuringElement(MORPH_RECT, Size(5, 5));	//Size(4, 4)
		dilate(matCompRoi, matCompRoi, element4);
		
#else
	#if 0
		//确定腐蚀和膨胀核的大小
		Mat element = getStructuringElement(MORPH_RECT, Size(6, 6));	//Size(4, 4)
		//腐蚀操作1
		erode(matCompRoi, matCompRoi, element);
	#else
		//确定腐蚀和膨胀核的大小
		Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));	//Size(4, 4)
		//腐蚀操作1
		erode(matCompRoi, matCompRoi, element);

		//确定腐蚀和膨胀核的大小
		Mat element2 = getStructuringElement(MORPH_RECT, Size(3, 3));	//Size(4, 4)
		//腐蚀操作2
		erode(matCompRoi, matCompRoi, element2);

		//膨胀操作
		dilate(matCompRoi, matCompRoi, element2);
		//膨胀操作
		dilate(matCompRoi, matCompRoi, element);
	#endif
#endif
		IplImage ipl_img(matCompRoi);

		//the parm. for cvFindContours  
		CvMemStorage* storage = cvCreateMemStorage(0);
		CvSeq* contour = 0;

		//提取轮廓  
		cvFindContours(&ipl_img, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		std::vector<Rect>RectCompList;
		for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++)
		{
			CvRect aRect = cvBoundingRect(contour, 0);
			Rect rm = aRect;
			RectCompList.push_back(rm);

// 			cv::Rect rtTmp = rm;
// 			rtTmp.x += rt.x;
// 			rtTmp.y += rt.y;
// 			rectangle(matCompPic, rtTmp, CV_RGB(255, 0, 0));
		}
		cvReleaseMemStorage(&storage);
		if (RectCompList.size() == 0)
			bResult = false;
		else
			bResult = true;
	}
	catch (cv::Exception &exc)
	{
		char szLog[300] = { 0 };
		sprintf_s(szLog, "CRecognizeThread::RecogVal error. detail: %s\n", exc.msg);
		g_Log.LogOut(szLog);
		TRACE(szLog);
	}

	return bResult;
}

inline bool CRecognizeThread::RecogVal2(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, RECTLIST& lSelInfo, std::string& strResult)
{
	Mat matCompRoi;
	
	bool bResult = false;

	//omr框的大小高度
	RECTLIST::iterator itItem = lSelInfo.begin();
	RECTLIST::reverse_iterator itEndItem = lSelInfo.rbegin();

 	cv::Point pt1, pt2;
	if (itItem->eCPType == SN)
	{
		switch (itItem->nRecogFlag)
		{
			case 10:
				pt1 = itItem->rt.tl() - cv::Point(3, 3);
				pt2 = itEndItem->rt.br() + cv::Point(3, 3);
				break;
			case 9:
				pt1 = itEndItem->rt.tl() - cv::Point(3, 3);
				pt2 = itItem->rt.br() + cv::Point(3, 3);
				break;
			case 6:
				pt1 = itItem->rt.tl() - cv::Point(3, 3);
				pt2 = itEndItem->rt.br() + cv::Point(3, 3);
				break;
			case 5:
				pt1 = itEndItem->rt.tl() - cv::Point(3, 3);
				pt2 = itItem->rt.br() + cv::Point(3, 3);
				break;
		}
	}
	else if (itItem->eCPType == OMR)
	{
		switch (itItem->nRecogFlag)
		{
			case 42:
				pt1 = itItem->rt.tl() - cv::Point(3, 3);
				pt2 = itEndItem->rt.br() + cv::Point(3, 3);
				break;
			case 41:
				pt1 = itItem->rt.tl() - cv::Point(3, 3);
				pt2 = itEndItem->rt.br() + cv::Point(3, 3);
				break;
			case 38:
				pt1 = itEndItem->rt.tl() - cv::Point(3, 3);
				pt2 = itItem->rt.br() + cv::Point(3, 3);
				break;
			case 37:
				pt1 = itEndItem->rt.tl() - cv::Point(3, 3);
				pt2 = itItem->rt.br() + cv::Point(3, 3);
				break;
			case 26:
				pt1 = itItem->rt.tl() - cv::Point(3, 3);
				pt2 = itEndItem->rt.br() + cv::Point(3, 3);
				break;
			case 25:
				pt1 = itItem->rt.tl() - cv::Point(3, 3);
				pt2 = itEndItem->rt.br() + cv::Point(3, 3);
				break;
			case 22:
				pt1 = itEndItem->rt.tl() - cv::Point(3, 3);
				pt2 = itItem->rt.br() + cv::Point(3, 3);
				break;
			case 21:
				pt1 = itEndItem->rt.tl() - cv::Point(3, 3);
				pt2 = itItem->rt.br() + cv::Point(3, 3);
				break;
		}
	}
	if (pt1 == pt2)
		return bResult;

// 	pt1 = itItem->rt.tl() - cv::Point(3, 3);
// 	pt2 = itEndItem->rt.br() + cv::Point(3, 3);
	Rect rt = cv::Rect(pt1, pt2);	//ABCD整个题目的选项区

	RECTLIST::iterator itFirst = itItem++;
#if 0
	int nSpace = 0;		//选项间的空格距离(可能是宽，可能是高)
	switch (itItem->nRecogFlag)	//获取选项间的空白区域宽度或高度
	{
		case 42:	//101010
			nSpace = itItem->rt.tl().y - itFirst->rt.br().y;
			break;
		case 41:	//101001
			nSpace = itItem->rt.tl().y - itFirst->rt.br().y;
			break;
		case 38:	//100110
			nSpace = itFirst->rt.tl().y - itItem->rt.br().y;
			break;
		case 37:	//100101
			nSpace = itFirst->rt.tl().y - itItem->rt.br().y;
			break;
		case 26:
			nSpace = itItem->rt.tl().x - itFirst->rt.br().x;
			break;
		case 25:
			nSpace = itItem->rt.tl().x - itFirst->rt.br().x;
			break;
		case 22:
			nSpace = itFirst->rt.tl().x - itItem->rt.br().x;
			break;
		case 21:
			nSpace = itFirst->rt.tl().x - itItem->rt.br().x;
			break;
		default:
			nSpace = max(abs(itFirst->rt.br().x - itItem->rt.tl().x), abs(itFirst->rt.br().y - itItem->rt.tl().y));
			break;
	}
#endif
	int nOmrMinW, nOmrMinH, nAreaMin;
	nOmrMinW = itFirst->rt.width * 0.4;
	nOmrMinH = itFirst->rt.height * 0.4;
	nAreaMin = itFirst->rt.area() * 0.3;
	//根据大小、面积先过滤一下可能框选到题号的情况

	std::string strTmpTime;
	int nTmpTime[6] = { 0 };	//计算每步时间
	clock_t sTime, eTime;
	sTime = clock();

	try
	{
		cv::Point ptNew1, ptNew2;
		if (!pModelInfo->pModel->nHasHead)	//无同步头时，进行坐标重定位
		{
			if (rt.x < 0) rt.x = 0;
			if (rt.y < 0) rt.y = 0;
			if (rt.br().x > matCompPic.cols)
			{
				rt.width = matCompPic.cols - rt.x;
			}
			if (rt.br().y > matCompPic.rows)
			{
				rt.height = matCompPic.rows - rt.y;
			}
			matCompRoi = matCompPic(rt);

			if (matCompRoi.channels() == 3)
				cv::cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

			if ((itFirst->eCPType == OMR || itFirst->eCPType == SN) && g_nRecogWithContract)
			{
				AutoContractBright(matCompRoi);
			}
			//图片二值化
			threshold(matCompRoi, matCompRoi, _nThreshold_Recog2_, 255, THRESH_BINARY_INV);				//200, 255

			IplImage ipl_img(matCompRoi);

			//the parm. for cvFindContours  
			CvMemStorage* storage = cvCreateMemStorage(0);
			CvSeq* contour = 0;

			//提取轮廓  
			cvFindContours(&ipl_img, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

			bool bFirst = true;
			std::vector<Rect> RectBaseList;
			for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++)
			{
				CvRect aRect = cvBoundingRect(contour, 0);
				cv::Rect rm = aRect;

				if (rm.width < nOmrMinW || rm.height < nOmrMinH || rm.area() < nAreaMin)		//************	针对中括号的选项会存在问题	************
					continue;

				//需要去除可能包含的情况


				if (bFirst)
				{
					bFirst = false;
					ptNew1 = rm.tl() + pt1;
					ptNew2 = rm.br() + pt1;
				}
				if (itItem->nRecogFlag >= 37)	//选项为竖向的
				{
					if (rm.tl().y + pt1.y < ptNew1.y)	ptNew1 = rm.tl() + pt1;
					if (rm.br().y + pt1.y > ptNew2.y)	ptNew2 = rm.br() + pt1;
				}
				else  //选项为横向的
				{
					if (rm.tl().x + pt1.x < ptNew1.x)	ptNew1 = rm.tl() + pt1;
					if (rm.br().x + pt1.x > ptNew2.x)	ptNew2 = rm.br() + pt1;
				}
				RectBaseList.push_back(rm);
			}
			cvReleaseMemStorage(&storage);

			if (RectBaseList.size() <= lSelInfo.size())	//识别出的矩形区个数和选项一致，可能有删除的情况，还是去新的起点与原起点的中间值
			{
				ptNew1 = (pt1 + ptNew1) / 2;
				ptNew2 = (pt2 + ptNew2) / 2;
			}

// 			if (itItem->nRecogFlag >= 37)
// 				std::sort(RectBaseList.begin(), RectBaseList.end(), SortByPositionY2);
// 			else
// 				std::sort(RectBaseList.begin(), RectBaseList.end(), SortByPositionX2);
		}
		else
		{
			ptNew1 = pt1;
			ptNew2 = pt2;
		}

		eTime = clock();
		nTmpTime[0] = eTime - sTime;
		sTime = clock();

		//根据新的坐标点计算新选项区矩形的填涂情况
//		matCompRoi.deallocate();
		matCompRoi = matCompPic(cv::Rect(ptNew1, ptNew2));
		if (matCompRoi.channels() == 3)
			cv::cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

		if ((itFirst->eCPType == OMR || itFirst->eCPType == SN) && g_nRecogWithContract)
		{
			AutoContractBright(matCompRoi);
		}

		//++先获取区域的平均值
// 		int fAllMeanGray = 0.0;
// 		int fAllGrayStddev = 0.0;
// 		MatND mean;
// 		MatND stddev;
// 		meanStdDev(matCompRoi, mean, stddev);
// 
// 		IplImage *src;
// 		src = &IplImage(mean);
// 		for (int i = 0; i < mean.rows; i++)
// 		{
// 			for (int j = 0; j < mean.cols; j++)
// 			{
// 				double ImgPixelVal = cvGetReal2D(src, i, j);
// 				fAllMeanGray = ImgPixelVal;
// 				//输出像素值
// 				TRACE("图像的均值: %f\n", ImgPixelVal);
// 			}
// 		}
// 
// 		IplImage *src2;
// 		src2 = &IplImage(stddev);
// 		for (int i = 0; i < stddev.rows; i++)
// 		{
// 			for (int j = 0; j < stddev.cols; j++)
// 			{
// 				double ImgPixelVal = cvGetReal2D(src2, i, j);
// 				fAllGrayStddev = ImgPixelVal;
// 				//输出像素值
// 				TRACE("标准差: %f\n", ImgPixelVal);
// 			}
// 		}
		//--

		eTime = clock();
		nTmpTime[1] = eTime - sTime;
		sTime = clock();

		//++先获取均值和标准差，再计算新的二值化阀值	2017.4.27
		const int channels[1] = { 0 };
		const int histSize[1] = { _nThreshold_Recog2_ };
		float hranges[2] = { 0, _nThreshold_Recog2_ };
		const float* ranges[1];
		ranges[0] = hranges;
		MatND hist;
		calcHist(&matCompRoi, 1, channels, Mat(), hist, 1, histSize, ranges);	//histSize, ranges

		int nSum = 0;
		int nDevSum = 0;
		int nCount = 0;
		for (int h = 0; h < hist.rows; h++)	//histSize
		{
			float binVal = hist.at<float>(h);

			nCount += static_cast<int>(binVal);
			nSum += h*binVal;
		}
		int nThreshold = _nThreshold_Recog2_;
		if (nCount > 0)
		{
			float fMean = (float)nSum / nCount;		//均值

			for (int h = 0; h < hist.rows; h++)	//histSize
			{
				float binVal = hist.at<float>(h);

				nDevSum += pow(h - fMean, 2)*binVal;
			}
			float fStdev = sqrt(nDevSum / nCount);
			nThreshold = fMean + fStdev;
			if (fStdev > fMean)
				nThreshold = fMean + fStdev;
		}

		if (nThreshold > _nThreshold_Recog2_) nThreshold = _nThreshold_Recog2_;
		//--

		//图片二值化
		threshold(matCompRoi, matCompRoi, nThreshold, 255, THRESH_BINARY_INV);		//_nThreshold_Recog2_		//200, 255


		eTime = clock();
		nTmpTime[2] = eTime - sTime;
		sTime = clock();

		//这里进行开闭运算
		//确定腐蚀和膨胀核的大小
		Mat element = getStructuringElement(MORPH_RECT, Size(6, 6));	//Size(4, 4)
		//腐蚀操作
		erode(matCompRoi, matCompRoi, element);

		IplImage ipl_img2(matCompRoi);

		//the parm. for cvFindContours  
		CvMemStorage* storage2 = cvCreateMemStorage(0);
		CvSeq* contour2 = 0;

		//提取轮廓  
		cvFindContours(&ipl_img2, storage2, &contour2, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		int nMaxArea = 0;
		std::vector<Rect>RectCompList;
		for (int iteratorIdx = 0; contour2 != 0; contour2 = contour2->h_next, iteratorIdx++)
		{
			CvRect aRect = cvBoundingRect(contour2, 0);
			cv::Rect rm = aRect;
			rm.x += ptNew1.x;
			rm.y += ptNew1.y;
			if (rm.area() > nMaxArea)
				nMaxArea = rm.area();

			RectCompList.push_back(rm);
		}
		cvReleaseMemStorage(&storage2);

		if (RectCompList.size() > 1)
		{
			int nMinArea = nMaxArea * 0.3;				//在所有二值化识别出来的矩形中再过滤一遍，根据最大识别矩形的面积
			std::vector<Rect>::iterator itRect = RectCompList.begin();
			for (; itRect != RectCompList.end();)
			{
				if (itRect->area() < nMinArea)
					itRect = RectCompList.erase(itRect);
				else
					itRect++;
			}
		}		

		eTime = clock();
		nTmpTime[3] = eTime - sTime;
		sTime = clock();

		//接下来根据位置信息判断abcd
		float fThreod;
		if (pModelInfo->pModel->nHasHead)
			fThreod = _dCompThread_Head_;
		else
			fThreod = _dCompThread_Fix_;

		std::string strRecogAnswer;
		if (RectCompList.size())
		{
			if (itItem->eCPType == OMR && itItem->nRecogFlag >= 37)	//选项竖直排列
			{
				std::string strTmpVal;
				RECTLIST::iterator itItem = lSelInfo.begin();
				for (; itItem != lSelInfo.end(); itItem++)
				{
					bool bFind = false;
					for (int i = 0; i < RectCompList.size(); i++)
					{
						if (RectCompList[i].tl().y < itItem->rt.br().y && RectCompList[i].br().y > itItem->rt.tl().y && itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
						{
							char szVal[10] = { 0 };
							sprintf_s(szVal, "%c", itItem->nAnswer + 65);
// 							if (strTmpVal.find(szVal) == std::string::npos)
// 								strTmpVal.append(szVal);

							if (strRecogAnswer.find(szVal) == std::string::npos)
								strRecogAnswer.append(szVal);
						}
					}
				}

				//下面逻辑和上面有重复，先测试上面的，没问题就可以删除下面
// 				if (strTmpVal.length())
// 				{
// 					RECTLIST::iterator itItem = lSelInfo.begin();
// 					for (; itItem != lSelInfo.end(); itItem++)
// 					{
// 						for (int i = 0; i < RectCompList.size(); i++)
// 						{
// 							if (RectCompList[i].tl().y < itItem->rt.br().y && RectCompList[i].br().x > itItem->rt.tl().x)
// 							{
// 								if (itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
// 								{
// 									char szVal[10] = { 0 };
// 									sprintf_s(szVal, "%c", itItem->nAnswer + 65);
// 									if (strRecogAnswer.find(szVal) == std::string::npos)
// 										strRecogAnswer.append(szVal);
// 								}
// 							}
// 						}
// 					}
// 				}

				//以下可用
// 				RECTLIST::iterator itItem = lSelInfo.begin();
// 				for (; itItem != lSelInfo.end(); itItem++)
// 				{
// 					for (int i = 0; i < RectCompList.size(); i++)
// 					{
// 						if (RectCompList[i].tl().y < itItem->rt.br().y && RectCompList[i].br().x > itItem->rt.tl().x)
// 						{
// 							float fThreod = 1.0;
// 							if (RectCompList.size() == 1)		//当只识别出只有一个选项时，降低灰度值标准
// 								fThreod = 0.95;
// 
// 							if (itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
// 							{
// 								char szVal[10] = { 0 };
// 								sprintf_s(szVal, "%c", itItem->nAnswer + 65);
// 								if (strRecogAnswer.find(szVal) == std::string::npos)
// 									strRecogAnswer.append(szVal);
// 							}
// 						}
// 					}
// 				}
			}
			else if(itItem->eCPType == OMR && itItem->nRecogFlag < 37)	//选项横向排列
			{
#if 1
				std::string strTmpVal;
				RECTLIST::iterator itItem = lSelInfo.begin();
				for (; itItem != lSelInfo.end(); itItem++)
				{
					bool bFind = false;
					for (int i = 0; i < RectCompList.size(); i++)
					{
						if (RectCompList[i].tl().x < itItem->rt.br().x && RectCompList[i].br().x > itItem->rt.tl().x && itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
						{
							char szVal[10] = { 0 };
							sprintf_s(szVal, "%c", itItem->nAnswer + 65);
// 							if (strTmpVal.find(szVal) == std::string::npos)
// 								strTmpVal.append(szVal);

							if (strRecogAnswer.find(szVal) == std::string::npos)
								strRecogAnswer.append(szVal);
						}
					}
				}

// 				if (strTmpVal.length())
// 				{
// 					RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
// 					for (; itItem != omrResult.lSelAnswer.end(); itItem++)
// 					{
// 						for (int i = 0; i < RectCompList.size(); i++)
// 						{
// 							if (RectCompList[i].tl().x < itItem->rt.br().x && RectCompList[i].br().x > itItem->rt.tl().x)
// 							{
// 								if (itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
// 								{
// 									char szVal[10] = { 0 };
// 									sprintf_s(szVal, "%c", itItem->nAnswer + 65);
// 									if (strRecogAnswer.find(szVal) == std::string::npos)
// 										strRecogAnswer.append(szVal);
// 								}
// 							}
// 						}
// 					}
// 				}
#else
				RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
				for (; itItem != omrResult.lSelAnswer.end(); itItem++)
				{
					bool bFind = false;
					for (int i = 0; i < RectCompList.size(); i++)
					{
						if (RectCompList[i].tl().x < itItem->rt.br().x && RectCompList[i].br().x > itItem->rt.tl().x)
						{
							float fThreod = 1.0;
							if (!bFind)								//当单个选项识别出多个小识别块时
							{
								bFind = true;
								if (RectCompList.size() == 1)		//当只识别出只有一个选项时，降低灰度值标准
									fThreod = 0.95;
							}
							else
							{
								fThreod = 0.95;
							}
							
							if (itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
							{
								char szVal[10] = { 0 };
								sprintf_s(szVal, "%c", itItem->nAnswer + 65);
								if (strRecogAnswer.find(szVal) == std::string::npos)
									strRecogAnswer.append(szVal);
							}
						}
					}
				}
#endif
			}
			else if (itItem->eCPType == SN && itItem->nRecogFlag >= 9)	//竖直排列
			{
				std::string strTmpVal;
				RECTLIST::iterator itItem = lSelInfo.begin();
				for (; itItem != lSelInfo.end(); itItem++)
				{
					bool bFind = false;
					for (int i = 0; i < RectCompList.size(); i++)
					{
						if (RectCompList[i].tl().y < itItem->rt.br().y && RectCompList[i].br().y > itItem->rt.tl().y && itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
						{
							char szVal[10] = { 0 };
							sprintf_s(szVal, "%d", itItem->nSnVal);

							if (strRecogAnswer.find(szVal) == std::string::npos)
								strRecogAnswer.append(szVal);
						}
					}
				}
			}
			else if (itItem->eCPType == SN && itItem->nRecogFlag < 9)
			{
				std::string strTmpVal;
				RECTLIST::iterator itItem = lSelInfo.begin();
				for (; itItem != lSelInfo.end(); itItem++)
				{
					bool bFind = false;
					for (int i = 0; i < RectCompList.size(); i++)
					{
						if (RectCompList[i].tl().x < itItem->rt.br().x && RectCompList[i].br().x > itItem->rt.tl().x && itItem->fRealValuePercent > fThreod)	//灰度值必须大于1.0才认为有填涂
						{
							char szVal[10] = { 0 };
							sprintf_s(szVal, "%d", itItem->nSnVal);

							if (strRecogAnswer.find(szVal) == std::string::npos)
								strRecogAnswer.append(szVal);
						}
					}
				}
			}
		}
		strResult = strRecogAnswer;


		eTime = clock();
		nTmpTime[4] = eTime - sTime;

		strTmpTime.append(Poco::format("recog omr 2: (%d:%d:%d:%d:%d)\n", nTmpTime[0], nTmpTime[1], nTmpTime[2], nTmpTime[3], nTmpTime[4]));
		TRACE(strTmpTime.c_str());
	}
	catch (cv::Exception &exc)
	{
		char szLog[300] = { 0 };
		sprintf_s(szLog, "CRecognizeThread::RecogVal2 error. detail: %s\n", exc.msg);
		g_Log.LogOut(szLog);
		TRACE(szLog);
	}

	return bResult;
}

bool CRecognizeThread::AutoContractBright(cv::Mat& matComPic)
{
	for (int i = 0; i < matComPic.rows; i++)
	{
		for (int j = 0; j < matComPic.cols; j++)
		{
			int nTmp = matComPic.at<uchar>(i, j);
			matComPic.at<uchar>(i, j) = saturate_cast<uchar>(m_nContract * 0.01 * (matComPic.at<uchar>(i, j)) + m_nBright);
		}
	}
	return true;
}

int CRecognizeThread::calcOmrDensityDiffVal(RECTLIST& rectList, std::vector<pRECTINFO>& vecItemsDesc, std::vector<ST_ITEM_DIFF>& vecOmrItemDiff)
{
#if 1	//下面将所有选项识别密度值降序排列并相邻比较
	RECTLIST::iterator itItem = rectList.begin();
	for (; itItem != rectList.end(); itItem++)
	{
		vecItemsDesc.push_back(&(*itItem));
	}
	std::sort(vecItemsDesc.begin(), vecItemsDesc.end(), SortByItemDensity);

	for (int i = 0; i < vecItemsDesc.size(); i++)
	{
		int j = i + 1;
		if (j < vecItemsDesc.size())
		{
			ST_ITEM_DIFF stDiff;
			sprintf_s(stDiff.szVal, "%c%c", (char)(vecItemsDesc[i]->nAnswer + 65), (char)(vecItemsDesc[j]->nAnswer + 65));
			stDiff.fDiff = vecItemsDesc[i]->fRealValuePercent - vecItemsDesc[j]->fRealValuePercent;
			stDiff.fFirst = vecItemsDesc[i]->fRealValuePercent;
			stDiff.fSecond = vecItemsDesc[j]->fRealValuePercent;
			vecOmrItemDiff.push_back(stDiff);
		}
	}
#else	//下面是整题所有选项的两两识别密度值的比较并按降序排列
	RECTLIST::iterator itFirst = rectList.begin();
	for (; itFirst != rectList.end(); itFirst++)
	{
		RECTLIST::iterator itSecond = itFirst;
		itSecond++;
		for (; itSecond != rectList.end(); itSecond++)
		{
			ST_ITEM_DIFF stDiff;
			sprintf_s(stDiff.szVal, "%c%c", (char)(itFirst->nAnswer + 65), (char)(itSecond->nAnswer + 65));
			stDiff.fDiff = itFirst->fRealValuePercent - itSecond->fRealValuePercent;
			stDiff.fFirst = vecItemsDesc[i]->fRealValuePercent;
			stDiff.fSecond = vecItemsDesc[j]->fRealValuePercent;
			vecOmrItemDiff.push_back(stDiff);
		}
	}
	std::sort(vecOmrItemDiff.begin(), vecOmrItemDiff.end(), SortByItemDiff);
#endif
	return 1;
}

int CRecognizeThread::calcSnDensityDiffVal(pSN_ITEM pSn, std::vector<pRECTINFO>& vecItemsDesc, std::vector<ST_ITEM_DIFF>& vecOmrItemDensityDiff)
{
	RECTLIST::iterator itItem = pSn->lSN.begin();
	for (; itItem != pSn->lSN.end(); itItem++)
	{
		vecItemsDesc.push_back(&(*itItem));
	}
	std::sort(vecItemsDesc.begin(), vecItemsDesc.end(), SortByItemDensity);

	for (int i = 0; i < vecItemsDesc.size(); i++)
	{
		int j = i + 1;
		if (j < vecItemsDesc.size())
		{
			ST_ITEM_DIFF stDiff;
			sprintf_s(stDiff.szVal, "%d_%d", vecItemsDesc[i]->nSnVal, vecItemsDesc[j]->nSnVal);
			stDiff.fDiff = vecItemsDesc[i]->fRealValuePercent - vecItemsDesc[j]->fRealValuePercent;
			stDiff.fFirst = vecItemsDesc[i]->fRealValuePercent;
			stDiff.fSecond = vecItemsDesc[j]->fRealValuePercent;
			vecOmrItemDensityDiff.push_back(stDiff);
		}
	}
	return 1;
}

int CRecognizeThread::calcSnGrayDiffVal(pSN_ITEM pSn, std::vector<pRECTINFO>& vecItemsDesc, std::vector<ST_ITEM_DIFF>& vecOmrItemGrayDiff)
{
	RECTLIST::iterator itItem = pSn->lSN.begin();
	for (; itItem != pSn->lSN.end(); itItem++)
	{
		vecItemsDesc.push_back(&(*itItem));
	}
	std::sort(vecItemsDesc.begin(), vecItemsDesc.end(), SortByItemGray);

	for (int i = 0; i < vecItemsDesc.size(); i++)
	{
		int j = i + 1;
		if (j < vecItemsDesc.size())
		{
			ST_ITEM_DIFF stDiff;
			sprintf_s(stDiff.szVal, "%d_%d", vecItemsDesc[i]->nSnVal, vecItemsDesc[j]->nSnVal);
			stDiff.fDiff = vecItemsDesc[i]->fRealValuePercent - vecItemsDesc[j]->fRealValuePercent;
			stDiff.fFirst = vecItemsDesc[i]->fRealValuePercent;
			stDiff.fSecond = vecItemsDesc[j]->fRealValuePercent;
			vecOmrItemGrayDiff.push_back(stDiff);
		}
	}
	return 1;
}

int CRecognizeThread::calcOmrGrayDiffVal(RECTLIST& rectList, std::vector<pRECTINFO>& vecItemsDesc, std::vector<ST_ITEM_DIFF>& vecOmrItemGrayDiff)
{
	RECTLIST::iterator itItem = rectList.begin();
	for (; itItem != rectList.end(); itItem++)
	{
		vecItemsDesc.push_back(&(*itItem));
	}
	std::sort(vecItemsDesc.begin(), vecItemsDesc.end(), SortByItemGray);

	for (int i = 0; i < vecItemsDesc.size(); i++)
	{
		int j = i + 1;
		if (j < vecItemsDesc.size())
		{
			ST_ITEM_DIFF stDiff;
			sprintf_s(stDiff.szVal, "%c%c", (char)(vecItemsDesc[i]->nAnswer + 65), (char)(vecItemsDesc[j]->nAnswer + 65));
			stDiff.fDiff = abs(vecItemsDesc[i]->fRealMeanGray - vecItemsDesc[j]->fRealMeanGray);
			stDiff.fFirst = vecItemsDesc[i]->fRealMeanGray;
			stDiff.fSecond = vecItemsDesc[j]->fRealMeanGray;
			vecOmrItemGrayDiff.push_back(stDiff);
		}
	}
	return 1;
}
bool CRecognizeThread::Recog2(int nPic, RECTINFO& rc, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo)
{
	bool bResult_Recog = false;

	if (!pModelInfo->pModel->nHasHead)	//同步头模式不需要判断是否矩形区框选到了其他区域
	{
		try
		{
			Mat matCompRoi;

			if (rc.rt.x < 0) rc.rt.x = 0;
			if (rc.rt.y < 0) rc.rt.y = 0;
			if (rc.rt.br().x > matCompPic.cols)
			{
				rc.rt.width = matCompPic.cols - rc.rt.x;
			}
			if (rc.rt.br().y > matCompPic.rows)
			{
				rc.rt.height = matCompPic.rows - rc.rt.y;
			}
			matCompRoi = matCompPic(rc.rt);
			
			if (matCompRoi.channels() == 3)
				cv::cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

			//图片二值化
			// 局部自适应阈值的图像二值化
			int blockSize = 25;		//25
			int constValue = 10;
			cv::Mat local;
//			cv::adaptiveThreshold(matCompRoi, matCompRoi, 255, CV_ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, blockSize, constValue);
			cv::threshold(matCompRoi, matCompRoi, rc.nThresholdValue, 255, THRESH_BINARY);
			cv::Canny(matCompRoi, matCompRoi, 0, _nCannyKernel_, 5);
			Mat element = getStructuringElement(MORPH_RECT, Size(2, 2));	//Size(6, 6)	普通空白框可识别
			dilate(matCompRoi, matCompRoi, element);

			IplImage ipl_img(matCompRoi);

			//the parm. for cvFindContours  
			CvMemStorage* storage = cvCreateMemStorage(0);
			CvSeq* contour = 0;

			//提取轮廓  
			cvFindContours(&ipl_img, storage, &contour, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

			int nMaxArea = 0;
			cv::Rect rt;
			int i = 0;
			for (int iteratorIdx = 0; contour != 0; contour = contour->h_next, iteratorIdx++)
			{
				CvRect aRect = cvBoundingRect(contour, 0);
				cv::Rect rm = aRect;

				if (rm.area() > nMaxArea)
				{
					nMaxArea = rm.area();
					rt = rm;
					rt.x += rc.rt.x;
					rt.y += rc.rt.y;
				}
				i++;
			}
			cvReleaseMemStorage(&storage);

#if 1

#else		//这里先不进行选项框的裁剪(裁剪目的是放在选项框到题目或其他选项的矩形区)，起码这里是有问题的，选项有填涂的时候，如果矩形区全都框选到，会识别出多个矩形，这个需要解决
			if (i > 1)
				rc.rt = rt;
#endif
		}
		catch (cv::Exception &exc)
		{
			bResult_Recog = false;
		}
	}
	bResult_Recog = Recog(nPic, rc, matCompPic, pPic, pModelInfo);
	return bResult_Recog;
}

bool CRecognizeThread::RecogVal_Omr2(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, OMR_RESULT& omrResult)
{
	return RecogVal2(nPic, matCompPic, pPic, pModelInfo, omrResult.lSelAnswer, omrResult.strRecogVal2);
}

bool CRecognizeThread::RecogVal_Sn2(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, pSN_ITEM pSn, std::vector<int>& vecItemVal)
{
	std::string strResult;
	RecogVal2(nPic, matCompPic, pPic, pModelInfo, pSn->lSN, strResult);
	char* p = NULL;
	for (int i = 0; i < strResult.length(); i++)
	{
		p = const_cast<char*>(strResult.c_str() + i);
		vecItemVal.push_back(atoi(p));
	}
	return true;
}

bool CRecognizeThread::RecogSn_omr(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, std::string& strLog)
{
	bool bRecogAll = true;
	bool bResult = true;
	std::vector<int> vecSN;

	clock_t start, end;
	start = clock();
	//strLog = Poco::format("图片%s\n", pPic->strPicName);

	SNLIST::iterator itSN = pModelInfo->pModel->vecPaperModel[nPic]->lSNInfo.begin();
	for (; itSN != pModelInfo->pModel->vecPaperModel[nPic]->lSNInfo.end(); itSN++)
	{
		pSN_ITEM pSnItem = *itSN;

		pSN_ITEM pSn = new SN_ITEM;
		pSn->nItem = pSnItem->nItem;
		(static_cast<pST_PaperInfo>(pPic->pPaper))->lSnResult.push_back(pSn);

		std::vector<int> vecItemVal;
		RECTLIST::iterator itSnItem = pSnItem->lSN.begin();
		for (; itSnItem != pSnItem->lSN.end(); itSnItem++)
		{
			RECTINFO rc = *itSnItem;

			if (pModelInfo->pModel->nHasHead)
			{
				if (rc.nHItem >= m_vecH_Head.size() || rc.nVItem >= m_vecV_Head.size())
				{
					bResult = false;
					pPic->bFindIssue = true;
					pPic->lIssueRect.push_back(rc);
					break;
				}
				rc.rt.x = m_vecH_Head[rc.nHItem].rt.tl().x;
				rc.rt.y = m_vecV_Head[rc.nVItem].rt.tl().y;
				rc.rt.width = m_vecH_Head[rc.nHItem].rt.width;
				rc.rt.height = m_vecV_Head[rc.nVItem].rt.height;
			}
			else
				GetPosition(pPic->lFix, pModelInfo->pModel->vecPaperModel[nPic]->lFix, rc.rt);
#if 1
			bool bResult_Recog = Recog2(nPic, rc, matCompPic, pPic, pModelInfo);
			if (bResult_Recog)
			{
				if (rc.fRealValuePercent > rc.fStandardValuePercent)
					vecItemVal.push_back(rc.nSnVal);
			}
			char szTmp[300] = { 0 };
			sprintf_s(szTmp, "图片名: %s, SN: 第%d位, 选项=%d, 识别实际比例=%.3f, val=%.2f, 识别标准=%.3f, val=%.2f, 是否成功:%d\n", pPic->strPicName.c_str(), \
					  pSnItem->nItem, rc.nSnVal, rc.fRealValuePercent, rc.fRealValue, rc.fStandardValuePercent, rc.fStandardValue, rc.fRealValuePercent > rc.fStandardValuePercent);
			TRACE(szTmp);
#else
			bool bResult_Recog = RecogVal(nPic, rc, matCompPic, pPic, pModelInfo);
			if (bResult_Recog)
			{
				vecItemVal.push_back(rc.nSnVal);
			}
#endif
			pSn->lSN.push_back(rc);

#ifdef PaintOmrSnRect	//打印OMR、SN位置
			pPic->lNormalRect.push_back(rc);
#endif
		}
		if (!bResult)
			break;

#if 1	//根据选项差值判断选中
		std::vector<pRECTINFO> vecItemsDesc;
		std::vector<ST_ITEM_DIFF> vecSnItemDiff;
		calcSnDensityDiffVal(pSn, vecItemsDesc, vecSnItemDiff);

		float fCompThread = 0.0;		//灰度间隔达到要求时，第一个选项的灰度必须达到的要求
		float fDiffThread = 0.0;		//选项可能填涂的可能灰度梯度阀值
		float fDiffExit = 0;			//灰度的梯度递减太快时，可以认为后面选项没有填涂，此时的灰度梯度阀值
		if (pModelInfo->pModel->nHasHead)
		{
			fCompThread = _dCompThread_Head_;
			fDiffThread = _dDiffThread_Head_;
			fDiffExit = _dDiffExit_Head_;
		}
		else
		{
			fCompThread = _dCompThread_Fix_;
			fDiffThread = _dDiffThread_Fix_;
			fDiffExit = _dDiffExit_Fix_;
		}

		int nFlag = -1;
		float fThreld = 0.0;
#if 1
		vecItemVal.clear();
		if(vecItemsDesc[0]->fRealValuePercent > fCompThread)
		{
			vecItemVal.push_back(vecItemsDesc[0]->nSnVal);
		}
#else
		for (int i = 0; i < vecSnItemDiff.size(); i++)
		{
			//根据所有选项灰度值排序，相邻灰度值差值超过阀值，同时其中第一个最大的灰度值超过1.0，就认为这个区间为选中的阀值区间
			//(大于1.0是防止最小的灰度值很小的时候影响阀值判断)
			float fDiff = (fCompThread - vecSnItemDiff[i].fFirst) * 0.1;
			if ((vecSnItemDiff[i].fDiff >= fDiffThread && vecSnItemDiff[i].fFirst > fCompThread) ||
				(vecSnItemDiff[i].fDiff >= fDiffThread + fDiff && vecSnItemDiff[i].fFirst > (fCompThread - 0.1) && fDiff > 0))
			{
				nFlag = i;
				fThreld = vecSnItemDiff[i].fFirst;
				if (vecSnItemDiff[i].fDiff > fDiffExit)	//灰度值变化较大，直接退出
					break;
			}
		}

		if (nFlag >= 0)
		{
			vecItemVal.clear();
			RECTLIST::iterator itItem = pSn->lSN.begin();
			for (; itItem != pSn->lSN.end(); itItem++)
			{
				if (itItem->fRealValuePercent >= fThreld)
				{
					vecItemVal.push_back(itItem->nSnVal);
				}
			}
		}
#endif
		if (vecItemVal.size() != 1)
		{
			char szTmpLog[300] = { 0 };
// 			for (int i = 0; i < vecItemsDesc.size(); i++)
// 			{
// 				char szTmp[10] = { 0 };
// 				sprintf_s(szTmpLog, "%d=%.3f", vecItemsDesc[i]->nSnVal, vecItemsDesc[i]->fRealValuePercent);
// 				strcat_s(szTmpLog, szTmp);
// 			}
			sprintf_s(szTmpLog, "第%d位SN[", pSn->nItem);
			for (int i = 0; i < vecSnItemDiff.size(); i++)
			{
				char szTmp[15] = { 0 };
				sprintf_s(szTmp, "%s:%.5f ", vecSnItemDiff[i].szVal, vecSnItemDiff[i].fDiff);
				strcat_s(szTmpLog, szTmp);
			}
			strcat_s(szTmpLog, "]\n");
			strLog.append(szTmpLog);
		}
#endif

		if (vecItemVal.size() == 1)
		{
			pSn->nRecogVal = vecItemVal[0];
			vecSN.push_back(vecItemVal[0]);
			for (auto itSn : pSn->lSN)
				if (itSn.nSnVal == pSn->nRecogVal)
				{
					pSn->rt = itSn.rt;
					break;
				}
		}
		else
		{
		#if 1	//第二种ZKZH识别方法 test
			std::vector<int> vecItemVal2;
			RecogVal_Sn2(nPic, matCompPic, pPic, pModelInfo, pSn, vecItemVal2);
		#endif

			if (vecItemVal.size() == 0 && vecItemVal2.size() == 1)
			{
				pSn->nRecogVal = vecItemVal2[0];
				vecSN.push_back(vecItemVal2[0]);
				for (auto itSn : pSn->lSN)
					if (itSn.nSnVal == pSn->nRecogVal)
					{
						pSn->rt = itSn.rt;
						break;
					}
			}
			else
			{
				std::vector<int> vecItemVal3;
				RecogVal_Sn3(nPic, matCompPic, pPic, pModelInfo, pSn, vecItemVal3);
				if (vecItemVal3.size())
				{
					pSn->nRecogVal = vecItemVal3[0];
					vecSN.push_back(vecItemVal3[0]);
					for (auto itSn : pSn->lSN)
						if (itSn.nSnVal == pSn->nRecogVal)
						{
							pSn->rt = itSn.rt;
							break;
						}
				}
				else
				{
					bRecogAll = false;
					char szVal[21] = { 0 };
					for (int i = 0; i < vecItemVal.size(); i++)
					{
						char szTmp[3] = { 0 };
						sprintf_s(szTmp, "%d ", vecItemVal[i]);
						strcat(szVal, szTmp);
					}
					char szLog[MAX_PATH] = { 0 };
					sprintf_s(szLog, "识别准考证号第%d位失败,识别出结果%d位(%s), 图片名: %s\n", pSnItem->nItem, vecItemVal.size(), szVal, pPic->strPicName.c_str());
					strLog.append(szLog);
					TRACE(szLog);
				}				
			}
		}
	}
	if (bRecogAll && vecSN.size() > 0)
	{
		for (int i = 0; i < vecSN.size(); i++)
		{
			char szTmp[5] = { 0 };
			itoa(vecSN[i], szTmp, 10);
			(static_cast<pST_PaperInfo>(pPic->pPaper))->strSN.append(szTmp);
		}
		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别准考证号完成(%s), 图片名: %s\n", (static_cast<pST_PaperInfo>(pPic->pPaper))->strSN.c_str(), pPic->strPicName.c_str());
		strLog.append(szLog);
		TRACE(szLog);
	}
	if (!bRecogAll)
	{
		(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmSnStatistics.lock();
		(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nSnNull++;
		(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmSnStatistics.unlock();
	}
	if (!bResult)
	{
		(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmSnStatistics.lock();
		(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->nSnNull++;
		(static_cast<pPAPERSINFO>((static_cast<pST_PaperInfo>(pPic->pPaper))->pPapers))->fmSnStatistics.unlock();

		char szLog[MAX_PATH] = { 0 };
		sprintf_s(szLog, "识别准考证号失败, 图片名: %s\n", pPic->strPicName.c_str());
		strLog.append(szLog);
		TRACE(szLog);
	}
	end = clock();
#ifdef PrintRunTime
	std::string strTime = Poco::format("识别考号时间: %dms\n", (int)(end - start));
	strLog.append(strTime);
#endif
	//g_Log.LogOut(strLog);
	return bResult;
}

bool CRecognizeThread::RecogSn_code(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, std::string& strLog)
{
	clock_t start, end;
	start = clock();
	//strLog = Poco::format("图片%s\n", pPic->strPicName);

	bool bResult = true;
	SNLIST::iterator itSN = pModelInfo->pModel->vecPaperModel[nPic]->lSNInfo.begin();
	for (; itSN != pModelInfo->pModel->vecPaperModel[nPic]->lSNInfo.end(); itSN++)
	{
		pSN_ITEM pSnItem = *itSN;
		
		pSN_ITEM pSn = new SN_ITEM;
		pSn->nItem = 0;
		(static_cast<pST_PaperInfo>(pPic->pPaper))->lSnResult.push_back(pSn);

		RECTLIST::iterator itSnItem = pSnItem->lSN.begin();
		for (; itSnItem != pSnItem->lSN.end(); itSnItem++)
		{
			RECTINFO rc = *itSnItem;
			pSn->lSN.push_back(rc);

			try
			{
				if (rc.rt.x < 0) rc.rt.x = 0;
				if (rc.rt.y < 0) rc.rt.y = 0;
				if (rc.rt.br().x > matCompPic.cols)
					rc.rt.width = matCompPic.cols - rc.rt.x;
				if (rc.rt.br().y > matCompPic.rows)
					rc.rt.height = matCompPic.rows - rc.rt.y;

				Mat matCompRoi;
				matCompRoi = matCompPic(rc.rt);

				if (matCompRoi.channels() == 3)
					cv::cvtColor(matCompRoi, matCompRoi, CV_BGR2GRAY);

				string strTypeName;
				string strResult = GetQR(matCompRoi, strTypeName);

				std::string strTmpLog;
				if (strResult != "")
					strTmpLog = "识别准考证号完成(" + strResult + "), 图片名: " + pPic->strPicName;
				else
				{
					strTmpLog = "识别准考证号失败, 图片名:" + pPic->strPicName;
//					bResult = false;
				}
				(static_cast<pST_PaperInfo>(pPic->pPaper))->strSN = strResult;
				strLog.append(strTmpLog);
			}
			catch (cv::Exception& exc)
			{
				std::string strTmpLog = "识别二维码或条码失败(" + pPic->strPicName + "): " + exc.msg;
				strLog.append(strTmpLog);
				break;
			}
		}
	}
	end = clock();
#ifdef PrintRunTime
	std::string strTime = Poco::format("识别考号时间: %dms\n", (int)(end - start));
	strLog.append(strTime);
#endif
	//g_Log.LogOut(strLog);
	return bResult;
}

bool CRecognizeThread::RecogVal_Omr3(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, OMR_RESULT& omrResult)
{
	int nMaybeAnswer = 0;	//可能的答案数
	std::vector<int> vecVal_AnswerSuer;
	RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
	for (; itItem != omrResult.lSelAnswer.end(); itItem++)
	{
		if (itItem->fRealMeanGray < _dAnswerSure_)
		{
			vecVal_AnswerSuer.push_back(itItem->nAnswer);
		}
		if (itItem->fRealMeanGray < (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2)
			nMaybeAnswer++;
	}

	std::string strRecogAnswer;
	std::vector<pRECTINFO> vecItemsGrayDesc;
	std::vector<ST_ITEM_DIFF> vecOmrItemGrayDiff;
	calcOmrGrayDiffVal(omrResult.lSelAnswer, vecItemsGrayDesc, vecOmrItemGrayDiff);

	float fCompThread = 0.0;		//灰度间隔达到要求时，第一个选项的灰度必须达到的要求
	float fDiffThreshold = 0.0;		//选项可能填涂的可能灰度梯度阀值
	float fDiffExit = 0;			//灰度的梯度递减太快时，可以认为后面选项没有填涂，此时的灰度梯度阀值

	fCompThread = _dCompThread_3_;
	fDiffThreshold = _dDiffThread_3_;
	fDiffExit = _dDiffExit_3_;

#if 1		//灰度差值满足后，加上单项Omr的灰度需要比标准的小
	float fMeanGrayDiff = 0.0;
	for (int i = 0; i < vecItemsGrayDesc.size(); i++)
	{
		fMeanGrayDiff += (vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray);
	}
	fMeanGrayDiff = fMeanGrayDiff / vecItemsGrayDesc.size();

	int nFlag = -1;
	float fThreld = 0.0;
	float fGrayDiffLast = 0.0;		//对上一次判断选中的选项对下一个选项选中判断的增益
	for (int i = 0; i < vecOmrItemGrayDiff.size(); i++)
	{
		float fGrayThresholdGray = vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray - fMeanGrayDiff;
		if (vecOmrItemGrayDiff[i].fDiff >= fDiffThreshold + fGrayThresholdGray + fGrayDiffLast)		//vecOmrItemGrayDiff[i].fDiff >= fDiffThreshold + fGrayThresholdGray
		{
			nFlag = i;
			fThreld = vecOmrItemGrayDiff[i].fFirst;
			if (vecOmrItemGrayDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_AnswerSuer.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
				break;
		}
		fGrayDiffLast += abs(fGrayThresholdGray) / 2;
	}
	if (nFlag >= 0)
	{
		//++判断全都选中的情况
		if (nFlag == vecOmrItemGrayDiff.size() - 1)	
		{
			if (vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray <= fCompThread)
			{
				fThreld = vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray;
			}
		}
		//--
		//if (nFlag == 0 && vecOmrItemGrayDiff[0].fDiff < fDiffThreshold)
		if(nFlag < nMaybeAnswer - 1)
		{
			int nThreld1 = _dDiffExit_3_ + _dAnswerSure_;
			int nThreld2 = (_dCompThread_3_ + _dDiffExit_3_ + _dAnswerSure_) / 2;
			for (int i = 0; i < vecItemsGrayDesc.size(); i++)
			{
				if (vecItemsGrayDesc[i]->fRealMeanGray < min(nThreld1, nThreld2))
				{
					fThreld = vecItemsGrayDesc[i]->fRealMeanGray < fThreld ? fThreld : vecItemsGrayDesc[i]->fRealMeanGray;
				}
				else if (vecItemsGrayDesc[i]->fRealMeanGray < max(nThreld1, nThreld2) && vecItemsGrayDesc[i]->fRealValuePercent > _dCompThread_Fix_)
				{
					fThreld = vecItemsGrayDesc[i]->fRealMeanGray < fThreld ? fThreld : vecItemsGrayDesc[i]->fRealMeanGray;
				}
			}
		}
		RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
		for (; itItem != omrResult.lSelAnswer.end(); itItem++)
		{
			if (itItem->fRealMeanGray <= fThreld)
			{
				char szVal[2] = { 0 };
				sprintf_s(szVal, "%c", itItem->nAnswer + 65);
				strRecogAnswer.append(szVal);
			}
		}
	}
	else if (vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray <= fCompThread)		//++判断全都选中的情况
	{
		fThreld = vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray;
		RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
		for (; itItem != omrResult.lSelAnswer.end(); itItem++)
		{
			if (itItem->fRealMeanGray <= fThreld)
			{
				char szVal[2] = { 0 };
				sprintf_s(szVal, "%c", itItem->nAnswer + 65);
				strRecogAnswer.append(szVal);
			}
		}
	}
	else
	{
		bool bFind = false;
		for (int i = 0; i < vecItemsGrayDesc.size(); i++)
		{
			float fDiff = abs(vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray);
			if (fDiff > fDiffThreshold && vecItemsGrayDesc[i]->fRealMeanGray < fCompThread)
			{
				fThreld = vecItemsGrayDesc[i]->fRealMeanGray > fThreld ? vecItemsGrayDesc[i]->fRealMeanGray : fThreld;
				bFind = true;
			}
		}
		if (bFind)
		{
			RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
			for (; itItem != omrResult.lSelAnswer.end(); itItem++)
			{
				if (itItem->fRealMeanGray <= fThreld)
				{
					char szVal[2] = { 0 };
					sprintf_s(szVal, "%c", itItem->nAnswer + 65);
					strRecogAnswer.append(szVal);
				}
			}
		}
		else
		{
			for (int i = 0; i < vecVal_AnswerSuer.size(); i++)
			{
				char szVal[5] = { 0 };
				sprintf_s(szVal, "%c", vecVal_AnswerSuer[i] + 65);
				strRecogAnswer.append(szVal);
			}
		}
	}
#else
	int nFlag = -1;
	float fThreld = 0.0;
	for (int i = 0; i < vecOmrItemGrayDiff.size(); i++)
	{
		if ((vecOmrItemGrayDiff[i].fDiff >= fDiffThreshold && vecOmrItemGrayDiff[i].fFirst < fCompThread))
		{
			nFlag = i;
			fThreld = vecOmrItemGrayDiff[i].fFirst;
			if (vecOmrItemGrayDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_AnswerSuer.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
				break;
		}
	}
	if (nFlag >= 0)
	{
		RECTLIST::iterator itItem = omrResult.lSelAnswer.begin();
		for (; itItem != omrResult.lSelAnswer.end(); itItem++)
		{
			if (itItem->fRealMeanGray <= fThreld)
			{
				char szVal[2] = { 0 };
				sprintf_s(szVal, "%c", itItem->nAnswer + 65);
				strRecogAnswer.append(szVal);
			}
		}
	}
	else
	{
		for (int i = 0; i < vecVal_AnswerSuer.size(); i++)
		{
			char szVal[5] = { 0 };
			sprintf_s(szVal, "%c", vecVal_AnswerSuer[i] + 65);
			strRecogAnswer.append(szVal);
		}
	}
#endif
	omrResult.strRecogVal3 = strRecogAnswer;
	return true;
}

bool CRecognizeThread::RecogVal_Sn3(int nPic, cv::Mat& matCompPic, pST_PicInfo pPic, pMODELINFO pModelInfo, pSN_ITEM pSn, std::vector<int>& vecItemVal)
{
	std::vector<int> vecVal_AnswerSuer;
	RECTLIST::iterator itItem = pSn->lSN.begin();
	for (; itItem != pSn->lSN.end(); itItem++)
	{
		if (itItem->fRealMeanGray < _dAnswerSure_)
		{
			vecVal_AnswerSuer.push_back(itItem->nSnVal);
		}
	}

	std::string strRecogAnswer;
	std::vector<pRECTINFO> vecItemsGrayDesc;
	std::vector<ST_ITEM_DIFF> vecOmrItemGrayDiff;
	calcOmrGrayDiffVal(pSn->lSN, vecItemsGrayDesc, vecOmrItemGrayDiff);

	float fCompThread = 0.0;		//灰度间隔达到要求时，第一个选项的灰度必须达到的要求
	float fDiffThread = 0.0;		//选项可能填涂的可能灰度梯度阀值
	float fDiffExit = 0;			//灰度的梯度递减太快时，可以认为后面选项没有填涂，此时的灰度梯度阀值

	fCompThread = _dCompThread_3_;
	fDiffThread = _dDiffThread_3_;
	fDiffExit = _dDiffExit_3_;
#if 1
	float fMeanGrayDiff = 0.0;
	for (int i = 0; i < vecItemsGrayDesc.size(); i++)
	{
		fMeanGrayDiff += (vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray);
	}
	fMeanGrayDiff = fMeanGrayDiff / vecItemsGrayDesc.size();

	int nFlag = -1;
	float fThreld = 0.0;
	float fGrayDiffLast = 0.0;		//对上一次判断选中的选项对下一个选项选中判断的增益
	for (int i = 0; i < vecOmrItemGrayDiff.size(); i++)
	{
		float fGrayThresholdGray = vecItemsGrayDesc[i]->fRealMeanGray - vecItemsGrayDesc[i]->fStandardMeanGray - fMeanGrayDiff;
		if ((vecOmrItemGrayDiff[i].fDiff >= fDiffThread + fGrayThresholdGray + fGrayDiffLast))
		{
			nFlag = i;
			fThreld = vecOmrItemGrayDiff[i].fFirst;
			if (vecOmrItemGrayDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_AnswerSuer.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
				break;
		}
		fGrayDiffLast += abs(fGrayThresholdGray) / 2;
	}
	if (nFlag >= 0)
	{
		if (nFlag == vecOmrItemGrayDiff.size() - 1)
		{
			if (vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray <= fCompThread)
			{
				fThreld = vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray;
			}
		}
		RECTLIST::iterator itItem = pSn->lSN.begin();
		for (; itItem != pSn->lSN.end(); itItem++)
		{
			if (itItem->fRealMeanGray <= fThreld)
			{
				vecItemVal.push_back(itItem->nSnVal);
			}
		}
	}
	else if (vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray <= fCompThread)		//++判断全都选中的情况
	{
		fThreld = vecItemsGrayDesc[vecOmrItemGrayDiff.size()]->fRealMeanGray;
		RECTLIST::iterator itItem = pSn->lSN.begin();
		for (; itItem != pSn->lSN.end(); itItem++)
		{
			if (itItem->fRealMeanGray <= fThreld)
			{
				vecItemVal.push_back(itItem->nSnVal);
			}
		}
	}
	else
	{
		vecItemVal = vecVal_AnswerSuer;
	}
#else
	int nFlag = -1;
	float fThreld = 0.0;
	for (int i = 0; i < vecOmrItemGrayDiff.size(); i++)
	{
		if ((vecOmrItemGrayDiff[i].fDiff >= fDiffThread && vecOmrItemGrayDiff[i].fFirst < fCompThread))
		{
			nFlag = i;
			fThreld = vecOmrItemGrayDiff[i].fFirst;
			if (vecOmrItemGrayDiff[i].fDiff > fDiffExit && i + 1 >= vecVal_AnswerSuer.size())	//灰度值变化较大，直接退出，如果阀值直接判断出来的个数超过当前判断的数量，就不能马上退
				break;
		}
	}
	if (nFlag >= 0)
	{
		RECTLIST::iterator itItem = pSn->lSN.begin();
		for (; itItem != pSn->lSN.end(); itItem++)
		{
			if (itItem->fRealMeanGray <= fThreld)
			{
				vecItemVal.push_back(itItem->nSnVal);
			}
		}
	}
	else
	{
		vecItemVal = vecVal_AnswerSuer;
	}
#endif
	return true;
}

void CRecognizeThread::HandleWithErrPaper(pST_PaperInfo pPaper)
{
	pPaper->bIssuePaper = true;				//标识此学生试卷属于问题试卷
	pPAPERSINFO pPapers = static_cast<pPAPERSINFO>(pPaper->pPapers);

	pPapers->fmlPaper.lock();
	PAPER_LIST::iterator itPaper = pPapers->lPaper.begin();
	for (; itPaper != pPapers->lPaper.end();)
	{
		if (*itPaper == pPaper)
		{
			itPaper = pPapers->lPaper.erase(itPaper);
			break;
		}
		else
			itPaper++;
	}
	pPapers->fmlPaper.unlock();

	pPapers->fmlIssue.lock();
	bool bFind = false;
	PAPER_LIST::iterator itIssuePaper = pPapers->lIssue.begin();
	for (; itIssuePaper != pPapers->lIssue.end();)
	{
		if (*itIssuePaper == pPaper)
		{
			bFind = true;
			break;
		}
		else
			itIssuePaper++;
	}
	if (!bFind)
	{
		pPapers->lIssue.push_back(pPaper);
		pPapers->nRecogErrCount++;
	}
	pPapers->fmlIssue.unlock();

	(static_cast<CDialog*>(pPaper->pSrcDlg))->SendMessage(MSG_ERR_RECOG, (WPARAM)pPaper, (LPARAM)pPapers);		//PostMessageW
}


