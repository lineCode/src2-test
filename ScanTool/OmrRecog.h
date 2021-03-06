#pragma once
#include "global.h"

class COmrRecog
{
public:
	COmrRecog();
	~COmrRecog();

	//给定一个矩形，按给定方向旋转后的矩形
	//1:正向，不需要旋转，2：右转90, 3：左转90, 4：右转180
	cv::Rect GetRectByOrientation(cv::Rect& rtPic, cv::Rect rt, int nOrientation);

	bool RecogFixCP(int nPic, cv::Mat& matCompPic, RECTLIST& rlFix, pMODEL	pModel, int nOrientation = 1);		//识别定点, 默认正向，不进行旋转
	bool RecogRtVal(RECTINFO& rc, cv::Mat& matCompPic);			//识别给定的一个矩形的灰度、密度等信息
	bool RecogZkzh(int nPic, cv::Mat& matCompPic, pMODEL pModel, int nOrientation);	//识别二维码或者条码，根据识别成功与否来判断图像方向
protected:	
	//识别矩形灰度、密度等信息
	inline bool Recog(int nPic, RECTINFO& rc, cv::Mat& matCompPic, pST_PicInfo pPic, pMODEL	pModel);
};

