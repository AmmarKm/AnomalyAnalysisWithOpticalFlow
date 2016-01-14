#include "stdafx.h"
#include "yxFGDetectMPBaseRGB.h"
#include "opencv2/highgui/highgui.hpp"

#include "opencv2/legacy/legacy.hpp"
//#include "opencv2/core/internal.hpp"
//#include "opencv2/video/tracking.hpp"
//#include "opencv2/video/background_segm.hpp"
//#include "opencv2/legacy/blobtrack.hpp"
//#include "opencv2/legacy/compat.hpp"

#include "math.h"
#include <iostream>
using namespace std;

#define  FG_VIBE_LIFE_SPAN             100        //��������
#define  FG_VIBE_MIN_MATCH				2        //��Сƥ����
#define  FG_VIBE_RADIUS				   30        //ƥ��뾶
#define  FG_VIBE_MODEL_NUM			   30        //ģ���еĲ�����
#define	 FG_VIBE_UPDATE_PROB		   10        //�������µĸ���
#define  FG_VIBE_BORDER				   10        //��Ե��ȣ�������µİ뾶
#define  FG_MP_HIST_BIN_NUM			   64        //ֱ��ͼ����
#define  FG_MP_TRAINNING_NUM		   200        //ѵ������֡��
#define	 FG_MP_TRAINNING_INTERVAL       2        //ѵ���������ʱû��
#define  FG_MP_BG_LEVLES				1        //�����㼶����ʱû��

//ȫ�ֺ���
void FindMax(unsigned char * arr , int n, unsigned char & maxval, unsigned char & idx)
{
	maxval = arr[0];
	idx = 0;
	for(int i=1; i< n; i++)
	{
		if(arr[i]>maxval)
		{
			maxval = arr[i];
			idx = i;
		}
	}
}
//bubble sort
void BubbleSort(int * arr, int n, int * index)
{
	int i,j, tmp1=0,tmp2=0;
	for(i=0; i< n; i++)
		index[i]=i;

	for (i=0; i< n; i++)
		for(j=n-1; j > i; j--){		
			if(arr[j] > arr[j-1]){
				tmp1 = arr[j-1];
				arr[j-1] = arr[j];
				arr[j] = tmp1;

				tmp2 = index[j-1];
				index[j-1] = index[j];
				index[j] = tmp2;
			}
		}
}

yxFGDetectMPBaseRGB::yxFGDetectMPBaseRGB()
{
	//MP�㷨��������ֵ
	m_defaultHistBinNum = FG_MP_HIST_BIN_NUM;
	m_defaultTrainningNum = FG_MP_TRAINNING_NUM;       
	m_defaultTrainningItv = FG_MP_TRAINNING_INTERVAL;  
	m_defaultBGLevels = FG_MP_BG_LEVLES;
	m_defaultRadius=FG_VIBE_RADIUS;
	m_pFGImg = NULL;
	m_pBGImg = NULL;

	m_ppppHist = NULL;
	m_frmNum=0;

#ifdef USING_VIBE
	//VIBE�㷨��������ֵ
	m_defaultLifetime=FG_VIBE_LIFE_SPAN;
	m_defaultMinMatch=FG_VIBE_MIN_MATCH;
	m_defaultModelNum=FG_VIBE_MODEL_NUM;
	m_defaultBorder=FG_VIBE_BORDER;
	m_defaultUpdateProb = FG_VIBE_UPDATE_PROB;
	m_ppLifeTime = NULL;
	m_pBGModel = NULL;
#endif
}

//��ʼ������
void yxFGDetectMPBaseRGB::Init(IplImage *img)
{
	if(!img)
	{
		cout<<"the parameter referenced to NULL pointer!"<<endl;
		return;
	}
	//MP�㷨��������ֵ
	m_imgHeight=img->height;
	m_imgWidth=img->width;
	m_defaultHistBinNum = FG_MP_HIST_BIN_NUM;	
	m_defaultTrainningNum = FG_MP_TRAINNING_NUM;       
	m_defaultTrainningItv = FG_MP_TRAINNING_INTERVAL; 
	m_defaultBGLevels = FG_MP_BG_LEVLES;
	m_defaultRadius=FG_VIBE_RADIUS;
	m_defaultMinArea=200;
	m_frmNum=0;

	//����ǰ��ͼ��
	if(m_pFGImg==NULL)	
	{
		m_pFGImg=cvCreateImage(cvSize(m_imgWidth,m_imgHeight),IPL_DEPTH_8U,1);
		cvZero(m_pFGImg);
	}
	//��������ͼ��
	if(m_pBGImg== NULL)
	{
		m_pBGImg = cvCreateImage(cvSize(m_imgWidth,m_imgHeight),IPL_DEPTH_8U,3);
		cvZero(m_pBGImg);
	}

	//����ֱ��ͼ����
	int i,j,k,d;
	if(m_ppppHist == NULL)
	{
		m_ppppHist=new unsigned char ***[m_imgHeight];
		for(i=0;i<m_imgHeight;i++)
		{
			m_ppppHist[i]=new unsigned char **[m_imgWidth];
			for(j=0;j<m_imgWidth;j++)
			{
				m_ppppHist[i][j]=new unsigned char * [3];
				for(d=0; d< 3; d++)
				{
					m_ppppHist[i][j][d]=new unsigned char [m_defaultHistBinNum];
					for(k=0;k<m_defaultHistBinNum;k++)
					{
						m_ppppHist[i][j][d][k]=0;
					}
				}
			}
		}
	}

#ifdef USING_VIBE
	//VIBE�㷨��������ֵ
	m_defaultLifetime=FG_VIBE_LIFE_SPAN;
	m_defaultMinMatch=FG_VIBE_MIN_MATCH;
	m_defaultModelNum=FG_VIBE_MODEL_NUM;
	m_defaultBorder=FG_VIBE_BORDER;
	m_defaultUpdateProb = FG_VIBE_UPDATE_PROB;

	//��������ʼ��ǰ������Ĥ����������
	if(m_ppLifeTime == NULL)
	{
		m_ppLifeTime=new int *[m_imgHeight];
		for(i=0;i<m_imgHeight;i++)
		{
			m_ppLifeTime[i]=new int [m_imgWidth];
			for(j=0;j<m_imgWidth;j++)
			{
				m_ppLifeTime[i][j]=0;
			}
		}
	}
	//��������ģ������
	if(m_pBGModel == NULL)
	{
		m_pBGModel=new unsigned char ***[m_imgHeight];
		for(i=0;i<m_imgHeight;i++)
		{
			m_pBGModel[i]=new unsigned char **[m_imgWidth];
			for(j=0;j<m_imgWidth;j++)
			{
				m_pBGModel[i][j]=new unsigned char *[3];
				for(d=0; d< 3; d++)
				{
					m_pBGModel[i][j][d]=new unsigned char [m_defaultModelNum];
					for(k=0;k<m_defaultModelNum;k++)
					{
						m_pBGModel[i][j][d][k]=0;
					}
				}
			}
		}
	}
#endif
}

//��������ģ��
bool yxFGDetectMPBaseRGB::ConstructBGModel(IplImage *img)
{
	if(m_frmNum  >= m_defaultTrainningNum)
		return true;
	//���֡��С��ѵ���������������ͳ��ÿ������λ�õ�ֱ��ͼ
	else
	{
		//��ͼ�����������ע������ͼ������Ǹ���ģ�����������
		double qfactor = 256/m_defaultHistBinNum;
		IplImage * qimg = cvCreateImage(cvGetSize(img),IPL_DEPTH_32F ,3);
		cvScale(img,qimg,1/qfactor,0);

		//ͳ��ÿ������λ�õ�ֱ��ͼ��ÿ��һ֡ͼ��ͽ����ۼӣ�
		int i,j,d,k;
		CvScalar cval;
		for(i=0;i<m_imgHeight;i++)
		{
			for(j=0;j<m_imgWidth;j++)
			{
				cval=cvGet2D(qimg,i,j);
				for(d=0; d< 3; d++)
				{
					int histidx = (int)cval.val[d];
					m_ppppHist[i][j][d][histidx]++;
				}
			}
		}
		cvReleaseImage(&qimg);

		//Ѱ��ֱ��ͼ��ǰ���ɸ����ֵ��Ӧ����ɫֵ����Щ��ɫֵ�����˱���ͼ��
		CvScalar index;
		unsigned char *tmphist = new unsigned char[m_defaultHistBinNum];
		memset(tmphist,0,m_defaultHistBinNum*sizeof(unsigned char));
		for(i=0;i<m_imgHeight;i++)
		{
			for(j=0;j<m_imgWidth;j++)
			{
				for(d=0; d<3 ;d++)
				{
					for(k=0; k< m_defaultHistBinNum; k++)
					{
						tmphist[k] = m_ppppHist[i][j][d][k];
					}
					//Ѱ�����ֵ
					unsigned char maxval,idx;
					FindMax(tmphist,m_defaultHistBinNum,maxval,idx);
					index.val[d]=qfactor*idx;
				}
				//�����ֵ��������Ϊ����ͼ��
				cvSet2D(m_pBGImg,i,j,index);
			}
		}
		delete [] tmphist;

		//ѵ����������ʱ���ӻ�ȡ�ı���ͼ��������������õ�����ģ��
		if(m_frmNum==(m_defaultTrainningNum-1))
		{
			//���Ƚ�ֱ��ͼ�ͷ�
			if(m_ppppHist != NULL)
			{
				for(i=0;i<m_imgHeight;i++)
				{
					for(j=0;j<m_imgWidth;j++)
					{
						for(d=0; d< 3; d++)
						{
							delete m_ppppHist[i][j][d];
							m_ppppHist[i][j][d] = NULL;
						}
						delete m_ppppHist[i][j];
						m_ppppHist[i][j] = NULL;
					}
					delete [] m_ppppHist[i];
					m_ppppHist[i] = NULL;
				}
				delete [] m_ppppHist;
				m_ppppHist = NULL;
			}

//�������VIBE�㷨
#ifdef USING_VIBE
			//������ͼ����ÿ�����ص�������������������������ģ��
			CvScalar svalue;
			for(i=m_defaultBorder;i<m_imgHeight-m_defaultBorder;i++)
			{
				for(j=m_defaultBorder;j<m_imgWidth-m_defaultBorder;j++)
				{
					for(k=0; k< m_defaultModelNum; k++)
					{
						int Ri=GetRandom(i);
						int Rj=GetRandom(j);
						svalue=cvGet2D(m_pBGImg,Ri,Rj);
						m_pBGModel[i][j][0][k]=(unsigned char)(svalue.val[0] + 0.5);
						m_pBGModel[i][j][1][k]=(unsigned char)(svalue.val[1] + 0.5);
						m_pBGModel[i][j][2][k]=(unsigned char)(svalue.val[2] + 0.5);
					}
				}
			}
#endif
		}

		//֡������1
		m_frmNum++;

		//cvNamedWindow("BG");
		//cvShowImage("BG",m_pBGImg);
		return false;
	}
}
////��������ģ��
//bool yxFGDetectMPBaseRGB::ConstructBGModel(IplImage *img)
//{
//	if(m_frmNum  == 0)
//	{
//		cvCopy(img,m_pBGImg);
//	}
//	//���֡��С��ѵ���������������ͳ��ÿ������λ�õ�ֱ��ͼ
//	else
//	{
//			//�������VIBE�㷨
//#ifdef USING_VIBE
//			int i,j,d,k;
//			//������ͼ����ÿ�����ص�������������������������ģ��
//			CvScalar svalue;
//			for(i=m_defaultBorder;i<m_imgHeight-m_defaultBorder;i++)
//			{
//				for(j=m_defaultBorder;j<m_imgWidth-m_defaultBorder;j++)
//				{
//					for(k=0; k< m_defaultModelNum; k++)
//					{
//						int Ri=GetRandom(i);
//						int Rj=GetRandom(j);
//						svalue=cvGet2D(m_pBGImg,Ri,Rj);
//						m_pBGModel[i][j][0][k]=svalue.val[0];
//						m_pBGModel[i][j][1][k]=svalue.val[1];
//						m_pBGModel[i][j][2][k]=svalue.val[2];
//					}
//				}
//			}
//#endif
//		}
//
//		//֡������1
//		m_frmNum++;
//
//		//cvNamedWindow("BG");
//		//cvShowImage("BG",m_pBGImg);
//		return false;
//}
//����������������ⲿ����
void yxFGDetectMPBaseRGB::Process(IplImage *img)
{
	//��һ֡���г�ʼ��
	if(m_frmNum ==0  || m_pFGImg==NULL)
	{
#ifdef USING_VIBE
		if(m_ppLifeTime==NULL || m_pBGModel==NULL)
#endif
		Init(img);
	}
	//ǰ����֡�����б�����ģ�������м��
	if(m_frmNum<m_defaultTrainningNum)
	{
		ConstructBGModel(img);
	}
	//ǰ�����
	else
	{
		FGDetect(img);
	}
}

void yxFGDetectMPBaseRGB::FGDetect(IplImage *img)
{

	//��ͼ���������
	//double qfactor = 256/m_defaultHistBinNum;
	//IplImage * qimg = cvCreateImage(cvGetSize(img),8,1);
	//cvScale(grayimg,qimg,1/qfactor,0);
	//cvScale(qimg,qimg,qfactor,0);

	//���Բ���ͼ�����������Ϊ�˱��������õ�ͼ����qimg���������²���
	IplImage * qimg = cvCreateImage(cvGetSize(img),8,3);
	cvScale(img,qimg,1,0);

	//��ʼ��ǰ��ͼ��
	cvZero(m_pFGImg);

	//�����������MP�㷨����������VIBE�㷨
#ifndef USING_VIBE
	//������ʱ����
	double dist0,dist1,dist2,dist;
	int i,j;
	//��ʼ����ÿ������
	CvScalar bgval,frmval;
	for(i=0;i<m_imgHeight;i++)
	{
		for(j=0;j<m_imgWidth;j++)
		{
			frmval=cvGet2D(qimg,i,j);
			bgval = cvGet2D(m_pBGImg,i,j);
			dist0 = frmval.val[0] - bgval.val[0];
			dist1 = frmval.val[1] - bgval.val[1];
			dist2 = frmval.val[2] - bgval.val[2];
			dist = sqrt(dist0*dist0+dist1*dist1+dist2*dist2);
			if(dist>m_defaultRadius*2)
			{
				//����ֵ����Ϊ255
				cvSetReal2D(m_pFGImg,i,j,255);
			}
		}
	}
#endif

	//�������VIBE�㷨
#ifdef USING_VIBE
	//������ʱ����
	int i,j,k;
	int matchCnt=0;//����Ƚ�����ֵ�ڵĴ���
	int iR1,iR2; //���������
	int Ri,Rj;   //����������X��Y�������
	double dist0,dist1,dist2,dist;

	//��ʼ����ÿ������
	CvScalar svalue;
	for(i=m_defaultBorder;i<m_imgHeight-m_defaultBorder;i++)
	{
		for(j=m_defaultBorder;j<m_imgWidth-m_defaultBorder;j++)
		{
			matchCnt=0;
			svalue=cvGet2D(qimg,i,j);
			for(k=0;k<m_defaultModelNum && matchCnt<m_defaultMinMatch;k++)
			{
				dist0 = svalue.val[0] - m_pBGModel[i][j][0][k];
				dist1 = svalue.val[1] - m_pBGModel[i][j][1][k];
				dist2 = svalue.val[2] - m_pBGModel[i][j][2][k];
				dist = sqrt(dist0*dist0+dist1*dist1+dist2*dist2);
				if(dist<m_defaultRadius)
				{
					matchCnt++;
				}
			}

			//������ƥ��Բ�ڵĴ�������m_defaultMinMatch������Ϊƥ���ϣ���Ϊ����
			if(matchCnt>=m_defaultMinMatch)
			{
				//������������Ϊ0
				cvSetReal2D(m_pFGImg,i,j,0);
				//����������0
				m_ppLifeTime[i][j]=0;

				//���±���ģ��
				iR1=GetRandom(0,m_defaultUpdateProb);
				if(iR1==0)
				{
					iR2=GetRandom();
					m_pBGModel[i][j][0][iR2]=(unsigned char)(svalue.val[0] + 0.5);
					m_pBGModel[i][j][1][iR2]=(unsigned char)(svalue.val[1] + 0.5);
					m_pBGModel[i][j][2][iR2]=(unsigned char)(svalue.val[2] + 0.5);
				}
				//��һ����������ģ��
				iR1=GetRandom(0,m_defaultUpdateProb);
				if(iR1==0)
				{
					Ri=GetRandom(i);
					Rj=GetRandom(j);
					iR2=GetRandom();
					m_pBGModel[Ri][Rj][0][iR2]=(unsigned char)(svalue.val[0] + 0.5);
					m_pBGModel[Ri][Rj][1][iR2]=(unsigned char)(svalue.val[1] + 0.5);
					m_pBGModel[Ri][Rj][2][iR2]=(unsigned char)(svalue.val[2] + 0.5);
				}
			}
			//��������Ϊǰ��
			else               
			{
				//����ֵ����Ϊ255
				cvSetReal2D(m_pFGImg,i,j,255);

				//�������ڼ�1
				m_ppLifeTime[i][j]=m_ppLifeTime[i][j]+1;

				//����������ڵ�ֵ������ֵ��˵�����ܴ��ڽ���������Ϊǰ�������������ǿ�Ƹ��±���ģ�����ɴ�
				if(m_ppLifeTime[i][j]>m_defaultLifetime) 
				{
					//��������
					m_ppLifeTime[i][j]=0;

					// ���Ҹ��±���ģ������
					iR1=GetRandom();
					m_pBGModel[i][j][0][iR1]=(unsigned char)(svalue.val[0] + 0.5);
					m_pBGModel[i][j][1][iR1]=(unsigned char)(svalue.val[1] + 0.5);
					m_pBGModel[i][j][2][iR1]=(unsigned char)(svalue.val[2] + 0.5);
					iR2=GetRandom();
					m_pBGModel[i][j][0][iR2]=(unsigned char)(svalue.val[0] + 0.5);
					m_pBGModel[i][j][1][iR2]=(unsigned char)(svalue.val[1] + 0.5);
					m_pBGModel[i][j][2][iR2]=(unsigned char)(svalue.val[2] + 0.5);
				}
			}
		}
	}
#endif

	//�ͷ�ͼ��
	cvReleaseImage(&qimg);

	//ɾ��С���Ŀ��
	DeleteSmallArea();

	//��̬ѧ����
	//IplImage * tmpimg = cvCreateImage(cvGetSize(img),8,1);
	//IplConvKernel* kernel = cvCreateStructuringElementEx(5,5,2,2,CV_SHAPE_ELLIPSE);
	//cvMorphologyEx(m_pFGImg,m_pFGImg,tmpimg,kernel,CV_MOP_CLOSE ,1);
	//cvReleaseStructuringElement(&kernel);
	//cvReleaseImage(&tmpimg);
	//cvDilate(m_pFGImg,m_pFGImg);
	//cvErode(m_pFGImg,m_pFGImg);
	//cvDilate(m_pFGImg,m_pFGImg);
	//cvErode(m_pFGImg,m_pFGImg);
	//cvDilate(m_pFGImg,m_pFGImg);
	//cvErode(m_pFGImg,m_pFGImg);
}
void yxFGDetectMPBaseRGB::FilterFGbyHSV(IplImage * srcimg,double sthr, double vthr)
{
	assert(srcimg->nChannels==3);
	if(srcimg->width != m_pFGImg->width || srcimg->height != m_pFGImg->height)
		return;
	IplImage * hsv = cvCreateImage(cvGetSize(srcimg),8,3);
	cvCvtColor(srcimg,hsv,CV_BGR2HSV);
	IplImage * simg = cvCreateImage(cvGetSize(srcimg),8,1);
	IplImage * vimg = cvCreateImage(cvGetSize(srcimg),8,1);
	cvSplit(hsv,NULL,simg,vimg,NULL);
	for(int i=0; i< srcimg->height; i++)
		for(int j=0; j < srcimg->width; j++)
		{
			uchar sval = CV_IMAGE_ELEM(simg,uchar,i,j);
			uchar vval = CV_IMAGE_ELEM(vimg,uchar,i,j);
			if(sval < (uchar)(sthr*255) && vval > (uchar)(vthr*255))
				CV_IMAGE_ELEM(m_pFGImg,uchar,i,j)=0;
		}
	cvReleaseImage(&hsv);
	cvReleaseImage(&simg);
	cvReleaseImage(&vimg);
}
void yxFGDetectMPBaseRGB::DeleteSmallArea()
{
	int region_count=0;
	CvSeq *first_seq=NULL, *prev_seq=NULL, *seq=NULL;
	CvMemStorage *storage=cvCreateMemStorage();
	cvClearMemStorage(storage);
    cvFindContours(m_pFGImg,storage,&first_seq,sizeof(CvContour),CV_RETR_EXTERNAL );
	for(seq=first_seq;seq;seq=seq->h_next)
	{
		CvContour *cnt=(CvContour*)seq;
		double area = cvContourArea(cnt);

		if(/*cnt->rect.height*cnt->rect.width*/area<m_defaultMinArea)
		{
			prev_seq=seq->h_prev;
			if(prev_seq)
			{
				prev_seq->h_next=seq->h_next;
				if(seq->h_next) 
					seq->h_next->h_prev=prev_seq;
			}
			else
			{
				first_seq=seq->h_next;
				if(seq->h_next)
					seq->h_next->h_prev=NULL;
			}
		}
		else
		{
			region_count++;
		}
	}

	cvZero(m_pFGImg);
	cvDrawContours(m_pFGImg,first_seq,CV_RGB(0,0,255),CV_RGB(0,0,255),10,-1);
	cvReleaseMemStorage(&storage);
}
#ifdef USING_VIBE
int yxFGDetectMPBaseRGB::GetRandom() //����һ��0����m_defaultModelNum-1��֮��������
{
	int val=int(m_defaultModelNum*1.0*rand()/RAND_MAX + 0.5);
	if(val==m_defaultModelNum)
		return val-1;
	else
		return val;
}

int yxFGDetectMPBaseRGB::GetRandom(int centre)//������centre���ĵ�����Χ���һ�������
{
	int val=int(centre-m_defaultBorder+rand()%(2*m_defaultBorder) + 0.5);
	if(val<centre-m_defaultBorder)
	{
		val=centre-m_defaultBorder;
	}
	if(val>centre+m_defaultBorder)
	{
		val=centre+m_defaultBorder;
	}
	return val;
}

int yxFGDetectMPBaseRGB::GetRandom(int istart, int iend)
{
	int val=istart+rand()%(iend-istart);
	return val;
}
#endif

IplImage* yxFGDetectMPBaseRGB::GetFGImg()
{
	return m_pFGImg;
}

IplImage* yxFGDetectMPBaseRGB::GetBGImg()
{
	return m_pBGImg;
}

//��������
yxFGDetectMPBaseRGB::~yxFGDetectMPBaseRGB()
{
	if(m_pFGImg != NULL)
	{
		cvReleaseImage(&m_pFGImg);
		m_pFGImg = NULL;
	}
	if(m_pBGImg!= NULL)
	{
		cvReleaseImage(&m_pBGImg);
		m_pBGImg= NULL;
	}
	int i,j,d;
	if(m_ppppHist != NULL)
	{
		for(i=0;i<m_imgHeight;i++)
		{
			for(j=0;j<m_imgWidth;j++)
			{
				for(d=0; d< 3; d++)
				{
					delete [] m_ppppHist[i][j][d];
					m_ppppHist[i][j][d] = NULL;
				}
				delete [] m_ppppHist[i][j];
				m_ppppHist[i][j] = NULL;
			}
			delete []m_ppppHist[i];
			m_ppppHist[i] = NULL;
		}
		delete [] m_ppppHist;
		m_ppppHist = NULL;
	}

#ifdef  USING_VIBE
	if (m_ppLifeTime!= NULL)
	{
		for(i=0;i<m_imgHeight;i++)
		{
			delete []m_ppLifeTime[i];
			m_ppLifeTime[i] = NULL;
		}
		delete []m_ppLifeTime;
		m_ppLifeTime = NULL;
	}
	if(m_pBGModel != NULL)
	{
		for(i=0;i<m_imgHeight;i++)
		{
			for(j=0;j<m_imgWidth;j++)
			{
				for(d=0; d< 3; d++)
				{
					delete [] m_pBGModel[i][j][d];
					m_pBGModel[i][j][d] = NULL;
				}
				delete [] m_pBGModel[i][j];
				m_pBGModel[i][j] = NULL;
			}
			delete []m_pBGModel[i];
			m_pBGModel[i] = NULL;
		}
		delete [] m_pBGModel;
		m_pBGModel = NULL;
	}
#endif
}
