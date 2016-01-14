//#include "stdafx.h"
#include "opencv2/highgui/highgui.hpp"

//�������Vibe�㷨���±���
#define	USING_VIBE

class yxFGDetectMPBaseRGB
{
public:
	yxFGDetectMPBaseRGB();					//���캯��
    ~yxFGDetectMPBaseRGB();					//��������
	void Process(IplImage *img);			//�����������ⲿ����
	IplImage* GetFGImg();					//��ȡǰ��ͼ���ⲿ����
	IplImage* GetBGImg();					//��ȡ����ͼ���ⲿ����
public:
	void Init(IplImage *img);				//��ʼ������
	bool ConstructBGModel(IplImage *img);   //��������ģ��
	void FGDetect(IplImage *img);           //ǰ����⼰����ģ�͸���
	void FilterFGbyHSV(IplImage * srcimg,double sthr=0.1, double vthr=0.4); //����ԭʼͼ���HSV�����ǰ�����й��ˣ���Ҫ��Ϊ���޳�һЩ��ɫ������Ҫ������ص�

protected:
	void DeleteSmallArea();                 //ɾ��С���Ŀ��

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
	IplImage * m_pFGImg;					//ǰ��ͼ��
	IplImage * m_pBGImg;					//����ͼ��

	int m_frmNum;						    //��¼��ǰ֡����ţ���Ϊǰ20ֻ֡����������ģ�ͣ�����Ҫ���

	//�������VIBE�㷨���±���
#ifdef USING_VIBE
	//VIBE�㷨
	int m_defaultLifetime;					//���ڱ��һ���������Ϊǰ���Ĵ���
	int m_defaultMinMatch;					//��ǰ�����뱳��ģ��ƥ������ٸ���
	int m_defaultModelNum;					//ÿ�����ص������
	int m_defaultBorder;					//������µĴ�С
	int m_defaultUpdateProb;				//��ĳ�������ж�Ϊ����������±����ĸ���
	//VIBE�㷨
	unsigned char ****m_pBGModel;			//����ģ��
	int **m_ppLifeTime;						//������¼һ�����ر�������Ϊǰ���Ĵ���

private:
	//��ȡ�����
	int GetRandom(int istart,int iend); // Ĭ��istart=0,iend=15
    int GetRandom(int centre);          //������centre���ĵ�����Χ���һ�������
    int GetRandom();                    //����һ��0����m_defaultModelNum-1��֮��������
#endif
};

