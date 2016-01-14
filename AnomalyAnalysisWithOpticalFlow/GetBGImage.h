#ifndef GETBGIMAGE_H_TOMHEAVEN_20140801
#define GETBGIMAGE_H_TOMHEAVEN_20140801


//#include "stdafx.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv_modules.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
using namespace std;
using namespace cv;
class yxFGDetectMPBaseRGB
{
public:
	yxFGDetectMPBaseRGB();					//���캯��
	~yxFGDetectMPBaseRGB();					//��������
	Mat GetFGImg();					//��ȡǰ��ͼ���ⲿ����
	Mat GetBGImg();					//��ȡ����ͼ���ⲿ����
public:
	void Init(const Mat&img);				//��ʼ������
	bool ConstructBGModel(const Mat&img);   //��������ģ��
private:
	//ͼ���С
	int m_imgHeight;						//ͼ��߶�
	int m_imgWidth;							//ͼ����
	//MP�㷨����
	double m_defaultMinArea;				//�˶�������С����������޳�����
	int m_defaultRadius;;					//����İ뾶��Ҳ��ƥ�����ֵ��Ĭ��Ϊ20
	int m_defaultHistBinNum;				//ֱ��ͼ������
	int m_defaultTrainningNum;				//���ڱ���ѧϰ��֡��
	int m_defaultTrainningItv;				//����ѧϰ��֡���
	int m_defaultBGLevels;					//����ͼ�������
	//MP�㷨
	unsigned char ****m_ppppHist;			//ÿ���ռ�λ������ʱ�����ϵ�ֱ��ͼ
	Mat m_pFGImg;					//ǰ��ͼ��
	Mat m_pBGImg;					//����ͼ��

	int m_frmNum;						    //��¼��ǰ֡����ţ���Ϊǰ20ֻ֡����������ģ�ͣ�����Ҫ���

};

#endif