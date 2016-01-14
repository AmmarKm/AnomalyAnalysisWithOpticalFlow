#include "stdafx.h"
#include "GetBGImage.h"
#include "opencv2/highgui/highgui.hpp"

#include "opencv2/opencv.hpp"
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
#define  FG_MP_TRAINNING_NUM		   500        //ѵ������֡��
#define	 FG_MP_TRAINNING_INTERVAL       2        //ѵ���������ʱû��
#define  FG_MP_BG_LEVLES				1        //�����㼶����ʱû��


//ȫ�ֺ���
void FindMax(unsigned char * arr , int n, unsigned char & maxval, unsigned char & idx)
{
	maxval = arr[0];
	idx = 0;
	int i;
  //  #pragma omp parallel for private(i) shared(maxval, arr) ���ô������࣬������ omp�Ż�
	for(i=1; i< n; i++)
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
	//m_pFGImg = NULL;
	//m_pBGImg = NULL;

	m_ppppHist = NULL;
	m_frmNum=0;
}

//��ʼ������
void yxFGDetectMPBaseRGB::Init(const Mat&img)
{
	if(img.empty())
	{
		cout<<"the parameter referenced to NULL pointer!"<<endl;
		return;
	}
	//MP�㷨��������ֵ
	m_imgHeight=img.rows;
	m_imgWidth=img.cols;
	m_defaultHistBinNum = FG_MP_HIST_BIN_NUM;	
	m_defaultTrainningNum = FG_MP_TRAINNING_NUM;       
	m_defaultTrainningItv = FG_MP_TRAINNING_INTERVAL; 
	m_defaultBGLevels = FG_MP_BG_LEVLES;
	m_defaultRadius=FG_VIBE_RADIUS;
	m_defaultMinArea=200;
	m_frmNum=0;

	//����ǰ��ͼ��
	if(m_pFGImg.empty())	
	{
		m_pFGImg=Mat(m_imgHeight,m_imgWidth,CV_8UC3,Scalar(0,0,0));
	}
	//��������ͼ��
	if(m_pBGImg.empty())
	{
		m_pBGImg = Mat(m_imgHeight,m_imgWidth,CV_8UC3,Scalar(0,0,0));
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
}

//��������ģ��
bool yxFGDetectMPBaseRGB::ConstructBGModel(const Mat&img)
{
	if(m_frmNum  >= m_defaultTrainningNum)
		return true;
	//���֡��С��ѵ���������������ͳ��ÿ������λ�õ�ֱ��ͼ
	else
	{
		//��ͼ�����������ע������ͼ������Ǹ���ģ�����������
		double qfactor = double(m_defaultHistBinNum)/double(256);

		Mat qimg;
		img.convertTo(qimg,CV_32F);
		qimg=qimg*qfactor;
		int i,j,d,k;

		//#pragma omp parallel for num_threads(dtn(n, MIN_ITERATOR_NUM))
//		int histidx;
//		#pragma omp for private(i, j)
		for(i=0;i<m_imgHeight;i++)
		{
			for(j=0;j<m_imgWidth;j++)
			{
				Vec3f cval=qimg.at<Vec3f>(i,j);
				m_ppppHist[i][j][0][(int)cval.val[0]]++;
				m_ppppHist[i][j][1][(int)cval.val[1]]++;
				m_ppppHist[i][j][2][(int)cval.val[2]]++;
			}
		}

		//cvReleaseImage(&qimg);

		//Ѱ��ֱ��ͼ��ǰ���ɸ����ֵ��Ӧ����ɫֵ����Щ��ɫֵ�����˱���ͼ��
		//CvScalar index;
		Vec3f index;
		unsigned char *tmphist = new unsigned char[m_defaultHistBinNum];
		memset(tmphist,0,m_defaultHistBinNum*sizeof(unsigned char));

//#pragma omp for private(i, j)
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
					index.val[d]=float(1/qfactor*idx);
					//printf("%f\n",1/qfactor*idx);
				}
				//�����ֵ��������Ϊ����ͼ��
				//cvSet2D(m_pBGImg,i,j,index);
				m_pBGImg.at<Vec3b>(i,j)=index;
			}
		}
		delete [] tmphist;

		//ѵ����������ʱ���ӻ�ȡ�ı���ͼ��������������õ�����ģ��
		if(m_frmNum==(m_defaultTrainningNum-1))
		{
			//���Ƚ�ֱ��ͼ�ͷ�
			if(m_ppppHist != NULL)
			{
//#pragma omp for private(i, j, d)
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
		}

		//֡������1
		m_frmNum++;

		//cvNamedWindow("BG");
		//cvShowImage("BG",m_pBGImg);
		return false;
	}
}
Mat yxFGDetectMPBaseRGB::GetFGImg()
{
	return m_pFGImg;
}

Mat yxFGDetectMPBaseRGB::GetBGImg()
{
	return m_pBGImg;
}

//��������
yxFGDetectMPBaseRGB::~yxFGDetectMPBaseRGB()
{
	//if(m_pFGImg != NULL)
	//{
	//	cvReleaseImage(&m_pFGImg);
	//	m_pFGImg = NULL;
	//}
	//if(m_pBGImg!= NULL)
	//{
	//	cvReleaseImage(&m_pBGImg);
	//	m_pBGImg= NULL;
	//}
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
}
