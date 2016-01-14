#ifndef IMAGEUTILS_H_TOMHEAVEN_20140730
#define IMAGEUTILS_H_TOMHEAVEN_20140730

#include <opencv2/opencv.hpp>
#include <cstdio>
#include <fstream>
using namespace std;
using namespace cv;
#define  _CRT_SECURE_NO_WARNINGS

/** ע�⣺�޸�bins�����޸Ĵ˴�! */
typedef Vec<float, 10> Vec7f;


/** ��ȡһ����ͨ��ͼ������� w, h */ // CV_IMAGE_ELEM
uchar getPixel(IplImage* img, int x, int y) {
	CV_Assert(img != NULL && img->nChannels != 3);
	return ((uchar*)(img->imageData + img->widthStep*y))[x];
}

/** ����һ����ͨ��ͼ������� w, h  Alt: */  // CV_IMAGE_ELEM
void setPixel(IplImage* img, int x, int y, uchar val) {
	CV_Assert(img != NULL && img->nChannels != 3);
	((uchar*)(img->imageData + img->widthStep*y))[x] = val;
}


/**
��ͼ�񱣴���ı��ļ�������鿴
*/

bool saveImageAsText(const Mat& img, const char* textFile) {
	FILE* fout = NULL;
	fopen_s(&fout, textFile, "w");
	if (fout == NULL) {
		return false;
	}
	fprintf(fout, "%d %d %d\n", img.rows, img.cols, img.channels());
	for(int i = 0; i < img.rows; ++i) {
		for(int j = 0; j < img.cols; ++j) {
			int k = 0;
			if (img.channels() == 1) {
				fprintf(fout, "%.0f  " , img.at<float>(i, j));
			}
			else {
				for(; k < img.channels() - 1; ++k)
					fprintf(fout, "%.0f," , img.at<Vec7f>(i, j)[k]);
				fprintf(fout, "%.0f  " , img.at<Vec7f>(i, j)[k]);
			}
		}
		fprintf(fout, "\n");
	}
	fclose(fout);
	return true;
}

/* // fstream �汾
bool saveImageAsText(const Mat& img, const char* textFile) {
	ofstream fout(textFile);
	if (fout.is_open() == false)
		return false;
	fout << img.rows << " " << img.cols << " " << img.channels() << endl;
	for(int i = 0; i < img.rows; ++i) {
		for(int j = 0; j < img.cols; ++j) {
			int k = 0;
			if (img.channels() == 1) {
				fout << img.at<double>(i, j) << " ";
			}
			else {
				for(; k < img.channels() - 1; ++k)
					fout << img.at<Vec7f>(i, j)[k] << ",";
				fout << img.at<Vec7f>(i, j)[k] << "    ";
			}
		}
		fout << endl;
	}
	fout.close();
	return true;
}*/

/**
���ı��ļ���ȡͼ��
*/
Mat loadImageFromText(const char*  textFile) {
	//printf("textFile = %s\n", textFile);
	FILE* fin = NULL;
	fopen_s(&fin, textFile, "r");
	assert(fin != NULL);
	int rows, cols, channels;
	fscanf_s(fin, "%d%d%d\n", &rows, &cols, &channels);
	Mat mat(rows, cols, CV_32FC(channels));
	for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			int k = 0;
			for(; k < channels - 1; ++k) {
				fscanf_s(fin, "%f,", &mat.at<Vec7f>(i, j)[k]);
			}
			fscanf_s(fin, "%f", &mat.at<Vec7f>(i, j)[k]);
		}
		fscanf_s(fin, "\n");
	}
	fclose(fin);
	return mat;
}


/** ��ͼ���л�һ���������� */
void DrawRect(IplImage * img, CvRect rect,CvScalar color)
{
	cvRectangle(img,cvPoint(rect.x,rect.y),cvPoint(rect.x+rect.width,rect.y+rect.height),color,2, CV_AA);
}

/** ������ͼ������� */
void BinaryORBinaryImage(Mat&srcbianryImage1,Mat&srcbianryImage2,Mat&dstbianryImage)
{
	int i,j;
#pragma omp parallel for private(i,j)
	for(i=0;i<srcbianryImage1.rows;i++)
		for(j=0;j<srcbianryImage1.cols;j++)
		{
			if(srcbianryImage1.at<uchar>(i,j)>0||srcbianryImage2.at<uchar>(i,j)>0)
			{
				dstbianryImage.at<uchar>(i,j)=255;
			}
			else
				dstbianryImage.at<uchar>(i,j)=0;

		}
}

void BinaryORBinaryImage(IplImage*srcbianryImage1,IplImage*srcbianryImage2,IplImage*dstbianryImage)
{
	BwImage srcImage1(srcbianryImage1);
	BwImage srcImage2(srcbianryImage2);
	BwImage dstImage(dstbianryImage);
	for(int i=0;i<srcbianryImage1->height;i++)
		for(int j=0;j<srcbianryImage1->width;j++)
		{
			if(srcImage1[i][j]>0||srcImage2[i][j]>0)
			{
				dstImage[i][j]=255;
			}
		}
}

/** ɾ�����С����ֵ�Ŀ� */
void RemoveSmallErea(const Mat&srcBianryImage,Mat&dst,double MinArea)
{
	dst=Mat(srcBianryImage.rows,srcBianryImage.cols,CV_8UC1,Scalar(0));
	vector<vector<Point> > contours;    vector<Vec4i> hierarchy;   
	findContours( srcBianryImage, contours, hierarchy, CV_RETR_EXTERNAL , CV_CHAIN_APPROX_NONE );  
	int n= (int) contours.size();
	Scalar color( 255, 255,255 );
	int idx;
#pragma omp parallel for private(idx)
	for(idx = 0; idx <n;idx++)    
	{        
		if(double(contourArea(contours[idx]))>MinArea)
		{
			drawContours(dst, contours, idx, color, CV_FILLED, 8);   
		}
	}
}

void RemoveSmallArea(IplImage*srcbinaryImage,double MinArea)
{
	int region_count=0;
	CvSeq *first_seq=NULL, *prev_seq=NULL, *seq=NULL;
	CvMemStorage *storage=cvCreateMemStorage();
	cvClearMemStorage(storage);
	cvFindContours(srcbinaryImage,storage,&first_seq,sizeof(CvContour),CV_RETR_EXTERNAL );
	for(seq=first_seq;seq;seq=seq->h_next)
	{
		CvContour *cnt=(CvContour*)seq;
		double area = cvContourArea(cnt);
		if(/*cnt->rect.height*cnt->rect.width*/area<MinArea)
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

	cvZero(srcbinaryImage);
	cvDrawContours(srcbinaryImage,first_seq,CV_RGB(0,0,255),CV_RGB(0,0,255),10,-1);
	cvReleaseMemStorage(&storage);
}




enum COMBINE_STYLE {ONE_COL, TWO_COL};

/**
��������Ƶ���źϲ�
@param inputFile1 ������Ƶ1
@param inputFile2 ������Ƶ2
@param outFile �����Ƶ·��
@param delay  ��Ƶ2����Ƶ1���ӳ�֡��
@param style �ϲ���ʽ ONE_COL���� TWO_COL
@return �Ƿ�ɹ�
*/
bool combineVideoFiles(const char* inputFile1, const char* inputFile2, const char* outFile, const int delay, const COMBINE_STYLE style = TWO_COL) {

	VideoCapture cap1(inputFile1), cap2(inputFile2);
	if (!cap1.isOpened()) {
		printf("input file %s cannot be opened!\n", inputFile1);
		return false;
	}
	if (!cap2.isOpened()) {
		printf("input file %s cannot be opened!\n", inputFile2);
		return false;
	}

	Mat mat1, mat2;
	for(int i = 0; i < delay; ++i)
		cap1.read(mat1);

	if (delay == 0) {
		cap1.read(mat1);
		cap2.read(mat2);
	}

	VideoWriter writer;

	if (style == TWO_COL) {
		writer = VideoWriter(outFile, CV_FOURCC('X','V','I','D'), cap1.get(CV_CAP_PROP_FPS), cvSize(mat1.cols*2, mat1.rows), true);
	} else {
		writer = VideoWriter(outFile, CV_FOURCC('X','V','I','D'), cap1.get(CV_CAP_PROP_FPS), cvSize(mat1.cols, mat1.rows), true);
	}

	if (!writer.isOpened()) {
		printf("output file %s cannot be opened!\n", outFile);
		return false;
	}

	namedWindow("combined", CV_WINDOW_KEEPRATIO);
	while (cap1.read(mat1) && cap2.read(mat2)) {
		if (mat1.rows != mat2.rows || mat1.cols != mat2.cols) {
			resize(mat2, mat2, cvSize(mat1.cols, mat1.rows));
		}
		////��������Ƶ
		if (style == TWO_COL) {
		    Mat outImg = Mat(mat1.rows, mat1.cols*2, CV_8UC3);
			Rect rect1(0,0, mat1.cols, mat1.rows);
			Rect rect2(mat1.cols,0,mat2.cols,mat2.rows);
		    mat1.copyTo(outImg(rect1));
		    mat2.copyTo(outImg(rect2));
		    writer.write(outImg);
			imshow("combined", outImg);
		} else if (style == ONE_COL) {
			vector<vector<Point> > contours;
			vector<Vec4i> hierarchy;
			Mat gray2;
			cvtColor(mat2, gray2, CV_RGB2GRAY);
			for(int i = 0; i < mat1.rows; ++i) {
				for(int j = 0; j < mat1.cols; ++j) {
					Vec3b& vec1 = mat1.at<Vec3b>(i, j);
					uchar grayLevel = gray2.at<uchar>(i, j);

					if (grayLevel > 128) {
						vec1[0] = (uchar)(vec1[0] * 0.5);
						vec1[1] = (uchar)(vec1[1] * 0.5);
						vec1[2] = (uchar)(vec1[2] * 0.5 + 255 * 0.5);
					}
				}
			}
			//addWeighted(mat1, 1.0f, mat2, 0.2f, 0.0, mat1);
			writer.write(mat1);
			imshow("combined", mat1);
	    }

		
		if (waitKey(1) == 27) {
			break;
		}
	}
	destroyWindow("combined");
	return true;
}

/** �����Ż��� runningAvg*/
void myRunningAvg(const Mat&srcMat, Mat&dstMat,const Mat&maskFrame, float alpha)
{
	int x,y,colsNum=srcMat.cols,rowsNum=srcMat.rows;
	int index;
	int n=rowsNum*colsNum;

#pragma omp parallel for private(index)
	for(index=0 ; index<n; index++)
	{
		y=index/colsNum;
		x=index%colsNum;
		dstMat.at<uchar>(y,x) = uchar( (srcMat.at<uchar>(y,x))*alpha + (1-alpha)*dstMat.at<uchar>(y,x) );
	}
	//#pragma omp parallel for private(y,x)
	//for(y=0 ; y<srcMat.rows ; y++)
	//for(x=0 ; x<srcMat.cols; x++)
	//{
	//	dstMat.at<uchar>(y,x) = (srcMat.at<uchar>(y,x))*alpha + (1-alpha)*dstMat.at<uchar>(y,x);
	//}
} 

#endif