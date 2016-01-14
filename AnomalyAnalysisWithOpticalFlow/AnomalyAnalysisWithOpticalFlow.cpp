// AnomalyAnalysisWithOpticalFlow.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>
#include "ImageTemplate.h"
#include "ImageUtils.h"
#include "EventDetection.h"
#include "GetBGImage.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <io.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <queue>
#include <omp.h>
#include <ctime>

//��16*16�ֿ�
#define BLOCK_16
//���ÿһ֡���е�ʱ��
// #define MEASURE_TIME //��stdafx.h��ͳһ����
//չʾ�ͱ����м���
//#define SHOW_RES



using namespace std;
using namespace cv;

#define  _CRT_SECURE_NO_WARNINGS


/**  ���������ļ��� */
char feature_folder[255] = "train_features";
/** �����Ƿ񱣴������Ŀ��� */
bool bSaveFeatures = false;
/** �����Ƿ�ִ�м��Ŀ��أ������ִ�У������resVideo��ȫ�ڵģ�����Matlab�Ĺ��������detection */
bool bPerformDetection = true;
/** �����Ƿ񱣴����ݵĿ��� */
bool bSaveBackground = false;


/** ��ȡ��Ƶ�е��˶�Ŀ�겢���浽�ļ� (>= opencv 1.0)
@param Video_Name - ������Ƶ·��
@param outBackground - �����Ƶ·��
@param FrmNumforBuildBG - ��������ģ�������֡���� Ĭ�� 100
@param defaultMinArea - ������С�������ֵ��Ĭ��100
@return a boolean value - �Ƿ�ɹ�
*/
bool subtractMovingObject(const char* video_Name, const char* outBackground, const char* outForeground, int FrmNumforBuildBG = 100, int defaultMinArea = 100)//
{
	yxFGDetectMPBaseRGB fgdetector;

	int nFrmNum = 0;
	CvScalar meanScalar0, meanScalar;

	vector<CvRect> VecTargetBlob;

	VideoCapture inputVideo(video_Name);        // Open input
	if (!inputVideo.isOpened())
	{
		cout << "Could not open the input video." << video_Name << endl;
		return false;
	}

	Mat frame;
	inputVideo.read(frame);
	VideoWriter backWriter(outBackground, CV_FOURCC('X', 'V', 'I', 'D'), inputVideo.get(CV_CAP_PROP_FPS), cvSize(frame.cols, frame.rows), false);
	VideoWriter foreWriter(outForeground, CV_FOURCC('X', 'V', 'I', 'D'), inputVideo.get(CV_CAP_PROP_FPS), cvSize(frame.cols, frame.rows), false);

	Mat pFrame, pBkImg, pBGImg, pPreImage, pDiffImage, pFinalFGImage, showSrcImage, pFrImg, BianryImage, showSrcImage1, BigImg;

	inputVideo >> pFrame;
	fgdetector.Init(pFrame);

#ifdef SHOW_RES
	namedWindow("background", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	namedWindow("FGImage", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	namedWindow("currentFrame", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	resizeWindow("background", 800, 600);
	resizeWindow("FGImage", 800, 600);
	resizeWindow("currentFrame", 800, 600);
#endif

	while (true)
	{
		inputVideo >> pFrame;
		if (pFrame.empty()) break;

		nFrmNum++;
		/**********************************************************************************************************/
		/*  ������ʼ����ģ��
		/**********************************************************************************************************/
		if (nFrmNum < FrmNumforBuildBG)
		{
			//����ǵ�һ֡����Ҫ�����ڴ棬����ʼ��
			//	printf("Now: %d\n", nFrmNum);
			//	double timeJianMo=GetTickCount();
			if (fgdetector.ConstructBGModel(pFrame)) break;
			//		timeJianMo=GetTickCount()-timeJianMo;
			//	printf("timeJianMo=%f\n",timeJianMo);
			pBGImg = fgdetector.GetBGImg();

#ifdef SHOW_RES
			//��ʾ����ͼ��
			imshow("background", pBGImg);
			waitKey(10);
#endif

		}
		if (nFrmNum >= FrmNumforBuildBG)
		{
			if (nFrmNum == FrmNumforBuildBG)
			{
				inputVideo.release();
				inputVideo.open(video_Name);
				inputVideo >> pFrame;

				cvtColor(pBGImg, pBkImg, CV_BGR2GRAY);
				cvtColor(pFrame, pPreImage, CV_BGR2GRAY);
			}
			else
			{
				cvtColor(pFrame, pFrImg, CV_BGR2GRAY);
				imshow("currentFrame", pFrame);
#pragma omp parallel sections
				{
#pragma omp section//���
					{
						showSrcImage = pFrame.clone();
						showSrcImage1 = pFrame.clone();
						//��ǰ֡ת��Ϊ�Ҷ�ͼ��
						//ǰһ֡�Ҷ�ͼ���뵱ǰ֡�Ҷ�ͼ�����
						absdiff(pPreImage, pFrImg, pDiffImage);
						//��ֵ��֡��ͼ��
						threshold(pDiffImage, BianryImage, 15, 255.0, CV_THRESH_BINARY);
					}
#pragma omp section//������
					{
						//���㵱ǰͼ��ľ�ֵ	 
						meanScalar0 = mean(pBkImg);
						meanScalar = mean(pFrImg);

						//���±����Ա���Ӧ���ձ仯
						Scalar diff;
						diff.val[0] = meanScalar.val[0] - meanScalar0.val[0];
						pBkImg = pBkImg + diff;

						// ʵʱ���±���ģ�ͣ���˹����ƽ����

						//myRunningAvg(pFrImg,pBkImg, Mat(),0.003f); //�����Ż� error in vs 2013

						//	accumulateWeighted(pFrImg,pBkImg,0.003); //�˺����� BUG
						addWeighted(pFrImg, 0.003, pBkImg, 1 - 0.003, 0, pBkImg);  // vs 2013 O2 �Ż�ʱ��������㷨���������

						//��ǰ֡������ͼ���
						absdiff(pBkImg, pFrImg, pFinalFGImage);
						//subtract(pFrImg, pBkImg, pFinalFGImage);
						//abs(pFinalFGImage);
						//��ֵ��ǰ��ͼ
						threshold(pFinalFGImage, pFrImg, 30, 255.0, CV_THRESH_BINARY);
					}
				}
#ifdef SHOW_RES
				//��ʾ����ͼ��
				imshow("background", pBGImg);
#endif
				BinaryORBinaryImage(BianryImage, pFrImg, pFinalFGImage);

				Mat FGImage;
				//double t2=GetTickCount();
				RemoveSmallErea(pFinalFGImage, FGImage, defaultMinArea);
				//������̬ѧ�˲���ȥ������
				//IplConvKernel*Kerne1=cvCreateStructuringElementEx(3,7,0,0,CV_SHAPE_ELLIPSE);
				dilate(FGImage, FGImage, Mat());

#ifdef SHOW_RES
				imshow("FGImage", FGImage);
				waitKey(10);
#endif
				foreWriter.write(FGImage);

				if (bSaveBackground) {
			    	backWriter.write(pBkImg);
			    }

				
			}
			//����ǰһ֡ͼ��
			cvtColor(pFrame, pPreImage, CV_BGR2GRAY);
			if (cvWaitKey(1) == 27) break;
		}
	}
	destroyAllWindows();
	return true;
}


/**
����һ֡���Ե�����
@param feature �������ά��������
@param integralImages ��������ͼ����
@param blockWidth ͳ�������ֿ�Ŀ��
@param blockHeight ͳ�������ֿ�ĸ߶�
@param blockDeltaX ͳ�������ֿ�Ŀ�ȼ��
@param blockDeltaY ͳ�������ֿ�ĸ߶ȼ��
*/

void addFrameToFeature(Mat& feature, const Mat* integralImages, const int blockWidth, const int blockHeight, int blockDeltaX, int blockDeltaY) {
	int i, j;
#pragma omp parallel for private (i, j)
	for (i = 0; i < feature.rows; ++i) {
		for (j = 0; j < feature.cols; ++j) {
			Vec7f& vec = feature.at<Vec7f>(i, j); //��ͨ������
			int blockX = j * blockDeltaX;
			int blockY = i * blockDeltaY;
			int xLimit = blockX + blockWidth;
			int yLimit = blockY + blockHeight;

			//����������磬�����߽�
			if (blockX + blockWidth >= integralImages[0].cols) {
				//xLimit = integralImages[0].cols - 1;
				continue;
			}
			if (blockY + blockHeight >= integralImages[0].rows) {
				//yLimit = integralImages[0].rows - 1;
				continue;
			}

			for (int k = 0; k < feature.channels(); ++k) {
				float a = integralImages[k].at<float>(blockY, blockX);
				float b = integralImages[k].at<float>(blockY, xLimit);
				float c = integralImages[k].at<float>(yLimit, blockX);
				float d = integralImages[k].at<float>(yLimit, xLimit);
				vec[k] += (a + d - b - c);
			}
		}
	}
}

/**
�����������м���
@param feature �������ά��������
@param integralImages ��������ͼ����
@param blockWidth ͳ�������ֿ�Ŀ��
@param blockHeight ͳ�������ֿ�ĸ߶�
@param blockDeltaX ͳ�������ֿ�Ŀ�ȼ��
@param blockDeltaY ͳ�������ֿ�ĸ߶ȼ��
*/
void removeFrameFromFeature(Mat& feature, const Mat* integralImages, const int blockWidth, const int blockHeight, int blockDeltaX, int blockDeltaY) {
	int i, j;
#pragma omp parallel for private (i, j)
	for (i = 0; i < feature.rows; ++i) {
		for (j = 0; j < feature.cols; ++j) {
			Vec7f& vec = feature.at<Vec7f>(i, j); //��ͨ������
			int blockX = j * blockDeltaX;
			int blockY = i * blockDeltaY;
			int xLimit = blockX + blockWidth;
			int yLimit = blockY + blockHeight;

			//����������磬�����߽�
			if (blockX + blockWidth >= integralImages[0].cols) {
				//xLimit = integralImages[0].cols - 1;
				continue;
			}
			if (blockY + blockHeight >= integralImages[0].rows) {
				//yLimit = integralImages[0].rows - 1;
				continue;
			}

			for (int k = 0; k < feature.channels(); ++k) {
				float a = integralImages[k].at<float>(blockY, blockX);
				float b = integralImages[k].at<float>(blockY, xLimit);
				float c = integralImages[k].at<float>(yLimit, blockX);
				float d = integralImages[k].at<float>(yLimit, xLimit);
				vec[k] -= (a + d - b - c);
			}
		}
	}
}

/** �����������ļ�
   @param featureBuf ����������vector
   @param bins ��������ά��������һά��ֹ��������ʵ�������� bins + 1 ά
   */

void saveFeature(vector<Mat>& featureBuf, int bins) {
	if (featureBuf.size() == 0)
		return;

	int rows = featureBuf[0].rows;
	int cols = featureBuf[0].cols;

	printf("rows = %d, cols = %d\n", rows, cols);
	char filename[255];
	printf("��ʼ������������(OpenCV���ڿ��ܻ�ʧȥ��Ӧ���벻Ҫ�رճ���)...\n");
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			int fileNum = i * cols + j;
			sprintf_s(filename, "%s/f_%d.txt", feature_folder, fileNum);
			FILE* fout = fopen(filename, "a");
			if (fout == NULL) {
				printf("error open file %s for writing.\n", filename);
				break;
			}

			for (int k = 0; k < featureBuf.size(); ++k) {
				Vec7f& vec = featureBuf[k].at<Vec7f>(i, j);
				//fprintf(fout, "1\t");
				for (int l = 0; l < bins + 1; ++l)
					//	fprintf(fout, "%d:%.0f\t", l, vec[l]);
					fprintf(fout, "%.0f\t", vec[l]);
				fprintf(fout, "\n");
			}
			//fflush(fouts[fileNum]);
			fclose(fout);
		}
	}
	featureBuf.clear();
	printf("���һ�α��档\n");
}


#ifdef  MEASURE_TIME
clock_t startTime;
clock_t endTime;
#endif

/**
���� busyness ͼ����߽����쳣��� (>= opencv 2.0)
@param foregroundVideo - �����ǰ����Ƶ
@param originVideo - �����ԭʼ��Ƶ
@param win_size ʱ�䴰�ڴ�С
@param bCompBusyness �Ƿ��Ǽ���busyness
@param busyness ����Ǽ���busyness���򷵻ش�ֵ���������ô�ֵ�����쳣���
@param writer ��������쳣��⣬�������� writer
@param of_time_interval ������ʱ��������
@param of_space_interval �����Ŀռ�������
@param bins Ͱ�ĸ���
@param blockWidth ͳ�������ֿ�Ŀ��
@param blockHeight ͳ�������ֿ�ĸ߶�
@param blockDeltaX ͳ�������ֿ�Ŀ�ȼ��
@param blockDeltaY ͳ�������ֿ�ĸ߶ȼ��
@param thresh �б��Ƿ��쳣����ֵ��������[0, 1]֮�䣬����ֵΪ 0.032
@return �Ƿ�ɹ�
*/
bool busynessAndDetectionCommon(const char* foregroundVideo, const char* originVideo, const int& win_size, const bool bCompBusyness, Mat& busyness, VideoWriter& writer, const char* outFlowVideo, const int& of_time_interval, const int& of_space_interval, int bins, int blockWidth, int blockHeight, int blockDeltaX, int blockDeltaY, float thresh)  {
	//�������
	assert(win_size >= 1);

	VideoCapture capture(foregroundVideo);
	if (!capture.isOpened())
	{
		printf("Can not open foreground video file %s\n", foregroundVideo);
		return false;
	}

	VideoCapture oCapture(originVideo);
	if (!oCapture.isOpened())
	{
		printf("Can not open original video file %s\n", originVideo);
		return false;
	}

	//�������ݶ��� queue  ע�⣺�ö��е�ʱ��һ��Ҫ push(mat.clone())������push����mat�����ã�����

	queue<Mat> ofQueue;       // ����ʱ��������
	queue<Mat*> frameQueue; // ǰ���Ҷ�ͼ�� frame ���У�angle ���У� flowmag ����

	// rgbForeground ��RGBǰ����ǰ֡, foreground �ǻҶȻ��ĵ�ǰ֡, curFrame ��ԭʼ��Ƶ��ǰ֡
	Mat rgbForeground, foreground, curFrame;

	capture.read(rgbForeground);
	oCapture.read(curFrame);
	//���������Ƶ��֡��Сһ��
	assert(rgbForeground.cols == curFrame.cols && rgbForeground.rows == curFrame.rows);
	
#ifdef SHOW_RES
	// �������������ļ�
	VideoWriter flowWriter(outFlowVideo, CV_FOURCC('X', 'V', 'I', 'D'), capture.get(CV_CAP_PROP_FPS), cvSize(curFrame.cols, curFrame.rows), true);
	if (!flowWriter.isOpened())
	{
		printf("Can not open outflow video file %s for writing.\n", outFlowVideo);
		return false;
	}
#endif

	//����(bins+1)ά��������
	Mat feature(busyness.rows, busyness.cols, CV_32FC(bins + 1), Scalar(0));
	//���徭����ģ������������
	Mat blurredFeature;
	Mat* binsImages = NULL, *integralImages = NULL;

	int nFrame = 0;

#ifdef SHOW_RES
	namedWindow("flow", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	namedWindow("foreground", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	namedWindow("flowmag", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	namedWindow("integral", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
#endif
	
	//	namedWindow("of_last_frame", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	if (!bCompBusyness) {
		namedWindow("res", CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
	}

	//Ϊ���ıղ�����׼��
	int dilation_size = 1;
	Mat element = getStructuringElement(MORPH_ELLIPSE,
		Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		Point(dilation_size, dilation_size));
	//////////////////

	//  x������y�������Ƕȣ������������������mask��������ͷ��ͼ��
	Mat xflow, yflow, angle, flowmag, sparseMask, showFlowMat;

	//�����������ڱ��浽�ļ�
	int bufsize = 1000;
	vector<Mat> featureBuf;

	//�����쳣������
	FILE* fout = NULL;
	if (!bCompBusyness)
		fout = fopen("abnormalBlock.txt", "w");

	while (capture.read(rgbForeground) && oCapture.read(curFrame)) {
#ifdef  MEASURE_TIME
		startTime = clock();
#endif

		++nFrame;
		//ת��Ϊ�Ҷ� frame
		cvtColor(rgbForeground, foreground, CV_RGB2GRAY);

		//�տ�ʼʱ�����ȶ���interval+1֡ͼ�����ǽ���Щͼ���Ϊ��׼ͼ����
		if (nFrame == 1) {
			for (int k = 0; k < of_time_interval + 1; k++)
			{
				ofQueue.push(curFrame.clone());
				if (k < of_time_interval)
					oCapture.read(curFrame);
			}
			continue;
		}
		//�Ե�ǰͼ�����׼ͼ����㵥�ܶȹ���
		// Ϊ�˲��ԣ���maskȫ����ֵ�ɰ�ɫ
		//foreground = Mat::ones(foreground.rows, foreground.cols, foreground.type()) * 255;

		if (!calcSparseOpticalFlow(curFrame, ofQueue.front(), xflow, yflow, angle, flowmag, sparseMask, showFlowMat, of_space_interval, foreground))
			continue;

#ifdef  MEASURE_TIME
		endTime = clock();
		printf("calcSparseOpticalFLow time = %.3f\n", double(endTime - startTime) / CLOCKS_PER_SEC);
#endif

		//�����������ֱ��ͼ
		binsImages = new Mat[bins];
		integralImages = new Mat[bins + 1];
		calcIntegralHist(angle, flowmag, bins, binsImages, integralImages); // ע�� calcIntegralHist ���ʼ�� integraImages[bins] ���ڴ�
		//����ǰ������ͼ
		//binsImages[bins] = Mat(foreground.rows+1,foreground.cols+1,CV_32FC1);
		myIntegralUchar(foreground, integralImages[bins]);
		delete[] binsImages;

		//ǰ���������Ƕȡ��������� �����
		frameQueue.push(integralImages);

		//���ӵ�ǰ֡������
		addFrameToFeature(feature, integralImages, blockWidth, blockHeight, blockDeltaX, blockDeltaY);

#ifdef  MEASURE_TIME
		endTime = clock();
		printf("addFrameToFeature time = %.3f\n", double(endTime - startTime) / CLOCKS_PER_SEC);
#endif

		// ��ʼ���� busynessf
		if (nFrame >= win_size) {
			//��ȥ�Ѿ������ڵ� frame
			Mat* prevIntegralImages = frameQueue.front();
			removeFrameFromFeature(feature, prevIntegralImages, blockWidth, blockHeight, blockDeltaX, blockDeltaY);
			//������
			frameQueue.pop();
			delete[] prevIntegralImages;

			//��������ģ������
			int blockKsize = 1;
			int ksize = blockWidth / blockDeltaX * blockKsize;
			blur(feature, blurredFeature, Size(ksize, ksize));

			// ��blurredFeature������������
			if (bSaveFeatures)
				featureBuf.push_back(blurredFeature.clone());
		
			//�����������ļ�
			// �� featureBuf �ﵽ�ڴ滺������ʱ��������д���ļ�������� featureBuf
			if (bSaveFeatures && featureBuf.size() > bufsize) {
				saveFeature(featureBuf, bins); //����·������ѵ���ͼ����������
			}

#ifdef SHOW_RES
			imshow("foreground", foreground);
			imshow("flow", showFlowMat);
			imshow("flowmag", flowmag);
			imshow("integral", integralImages[bins]);
			waitKey(1);
			flowWriter.write(showFlowMat);
#endif

			if (bCompBusyness) {
				//�������ֵ�Ի�� busyness
				max(blurredFeature, busyness, busyness);
			}
			else { //�� detection
				//��⵱ǰ�۲�ͼ��
				//	Mat res(feature.rows * blockHeight, feature.cols * blockHeight, CV_8UC3, Scalar(0));
				Mat res(foreground.rows, foreground.cols, CV_8UC3, Scalar(0));
				int i, j;
				int channel = 0;
				// openmp ����
				//	double cmpRes = compareHist(feature, busyness, CV_COMP_CORREL);
				//	printf("cmpRes = %d\n", cmpRes);
				if (bPerformDetection) {
					int nAbnormal = 0;
#pragma omp parallel for private (i, j)
					for (i = 0; i < feature.rows; ++i) {
						for (j = 0; j < feature.cols; ++j) {
							//�������ǰ�����˶�������
							//for (int k = 0; k < bins + 1; ++k) {
							for (int k = 0; k < bins; ++k) {
								Vec7f& vec2 = blurredFeature.at<Vec7f>(i, j);
								Vec7f& vec1 = busyness.at<Vec7f>(i, j);
								//�������ܼ��
								//	float MAX_FLOW =  1.0f * win_size * blockWidth * blockHeight / (of_space_interval * of_space_interval); //��ֵ��ֵ
								//	if (vec2[k] > MAX_FLOW)
								//	vec2[k] = MAX_FLOW;


								//����ǰ������busyness�Ƚϣ� threshold ����ֵ����Ҫ���� win_size ����
								float threshold = thresh * win_size * blockWidth * blockHeight / (of_space_interval * of_space_interval); //��ֵ��ֵ
								if (k == bins) //����ǰ����������ֵ��ͬ
									threshold = thresh * win_size * blockWidth * blockHeight;
								float ratio = 0.2f;  //������ֵ�����б�Ҫ
								if (float(vec2[k] - vec1[k]) / (vec1[k] + 2) >= ratio &&
									vec2[k] - vec1[k] >= threshold) {
									for (int y = i * blockDeltaY; y < i * blockDeltaY + blockHeight; ++y) {
										//����������磬�����߽�
										if (y >= res.rows) {
											break;
										}
										for (int x = j * blockDeltaX; x < j * blockDeltaX + blockWidth; ++x) {
											//����������磬�����߽�
											if (x >= res.cols) {
												break;
											}

											for (int channel = 0; channel < 3; ++channel)
												res.at<Vec3b>(y, x)[channel] = 255;
										}
									} 
									++nAbnormal;
									break;
								} // end if >
							} //end for k
						}  //end for j
					}  //end for i
					//���˼��֡���쳣�����д���ļ�
					int foregroundSum = 0;
					for (int i = 0; i < foreground.rows; ++i) {
						for (int j = 0; j < foreground.cols; ++j) {
							if (foreground.at<uchar>(i, j) > 128)
								++foregroundSum;
						}
					}
					double ratio = nAbnormal;
					if (foregroundSum > 0)
						ratio /= foregroundSum;
					fprintf(fout, "%d %.3f %d\n", nAbnormal, ratio, foregroundSum);
					//�ղ���
					//erode(res, res, element);
					//dilate(res, res, element);
				} // end if (bPerformDetection)

#ifdef SHOW_RES
				imshow("res", res);
				waitKey(1);
#endif
				//����������Ƶ
				writer.write(res);
			}
		}
		//����ǰ֡�����׼ͼ���飬���ù���֡������
		ofQueue.push(curFrame.clone());
		ofQueue.pop();


#ifdef SHOW_RES
		//ESC ���˳�
		if (waitKey(8) == 27) {
			break;
		}
#endif
		

#ifdef  MEASURE_TIME
		clock_t endTime = clock();
		printf("one-frame time = %.3f\n", double(endTime - startTime) / CLOCKS_PER_SEC);
#endif
	}

	//��β���� + ��������
	if (bSaveFeatures && featureBuf.size() > 0) {
		saveFeature(featureBuf, bins);

	}

	//�б�����Ҫ Sparse ����Ŀ�
	if (bSaveFeatures) {
		char filePath[255];
		sprintf(filePath, "%s\\validBlocks.txt", feature_folder);
		FILE* fout = fopen(filePath, "w");
		for (int i = 0; i < busyness.rows; ++i) {
			for (int j = 0; j < busyness.cols; ++j) {
				bool bValid = false;
				//���� validBlocks ʱ�����Ǿ�ֹ���� (ֻѭ���� bins ������ bins + 1��
				for (int k = 0; k < bins; ++k) {
					Vec7f& vec = busyness.at<Vec7f>(i, j);
					//����ǰ������busyness�Ƚϣ� threshold ����ֵ����Ҫ���� win_size ����
					float threshold = thresh * win_size * blockWidth * blockHeight / (of_space_interval * of_space_interval); //��ֵ��ֵ
					//if (k == bins) //����ǰ����������ֵ��ͬ
						//threshold = thresh * win_size * blockWidth * blockHeight;
					if (vec[k] > threshold && threshold > 0) {
						bValid = true;
						break;
					}
				}
				if (bValid) {
					int fileNum = i * busyness.cols + j;
					fprintf(fout, "%d ", fileNum);
				}
			}
		}
		fclose(fout);
	}

	while (!frameQueue.empty()) {
		Mat* prevIntegralImages = frameQueue.front();
		frameQueue.pop();
		delete[] prevIntegralImages;
	}
	destroyAllWindows();
	if (fout != NULL)
		fclose(fout);

	return true;
}


/**
���� busyness ͼ�� (>= opencv 2.0)
@param foregroundVideo - �����ǰ����Ƶ
@param originVideo - �����ԭʼ��Ƶ
@param win_size - ʱ�䴰�ڴ�С
@param of_time_interval - ������ʱ��������
@param of_space_interval - �����Ŀռ�������
@param blockDeltaX ͳ�������ֿ�Ŀ�ȼ��
@param blockDeltaY ͳ�������ֿ�ĸ߶ȼ��
@return Mat - busyness ͼ��(6 ͨ��)��x����, y�����ٶ����ֵ����Сֵ�� ���� ��flowmag < 2*2��, ����ٶȷ��� max(flowmag)
*/

Mat computeBusyness(const char* foregroundVideo, const char* originVideo, const int& win_size, const char* outFlowVideo, const int& of_time_interval, const int& of_space_interval, int bins, int blockWidth, int blockHeight, int blockDeltaX, int blockDeltaY) {
	VideoCapture capture(foregroundVideo);
	if (!capture.isOpened())
	{
		printf("Can not open foreground video file %s\n", foregroundVideo);
		return Mat::zeros(1, 1, CV_8UC1);
	}
	Mat frame;
	capture.read(frame);
	//����Ҫ�����busyness
	Mat busyness((frame.rows - blockHeight) / blockDeltaX, (frame.cols - blockWidth) / blockDeltaY, CV_32FC(bins + 1), Scalar(0));
	//Mat busyness(frame.rows / blockDeltaX, frame.cols / blockDeltaY, CV_32FC(bins + 1), Scalar(0)); //���ص�ʱ����Ҫ��ȥ blockWidth
	//����һ�����õ�writer�Ա���ÿ�ܺ���
	VideoWriter writer;
	//����ͨ�ÿ�� (busyness)
	float thresh = 0.02f; //�˲��������ʱ���ã��˴�����
	busynessAndDetectionCommon(foregroundVideo, originVideo, win_size, true, busyness, writer, outFlowVideo, of_time_interval, of_space_interval, bins, blockWidth, blockHeight, blockDeltaX, blockDeltaY, thresh);
	return busyness;
}


/**
��������ǰ����Ƶ�е��쳣�¼�
@param foregroundVideo - �����ǰ����Ƶ
@param originVideo - �����ԭʼ��Ƶ
@param outVideo - ������쳣�����Ƶ
@param win_size - ʱ�䴰�ڴ�С
@param of_time_interval - ������ʱ��������
@param of_space_interval - �����Ŀռ�������
@param blockDeltaX ͳ�������ֿ�Ŀ�ȼ��
@param blockDeltaY ͳ�������ֿ�ĸ߶ȼ��
@param thresh �б��Ƿ��쳣����ֵ��������[0, 1]֮�䣬����ֵΪ 0.02
@return �Ƿ�ɹ�
*/

bool detectAbnomal(Mat busyness, const char* foregroundVideo, const char* originVideo, const char* outVideo, const int win_size, const char* outFlowVideo, const int& of_time_interval, int bins, const int& of_space_interval, int blockWidth, int blockHeight, int blockDeltaX, int blockDeltaY, float thresh) {
	VideoCapture capture(foregroundVideo);
	if (!capture.isOpened())
	{
		printf("Can not open foreground video file %s\n", foregroundVideo);
		return false;
	}
	Mat frame;
	capture.read(frame);
	//���� writer
	VideoWriter writer(outVideo, CV_FOURCC('X', 'V', 'I', 'D'), capture.get(CV_CAP_PROP_FPS), cvSize(frame.cols, frame.rows), true);
	//����ͨ�ÿ�� (detection)
	return busynessAndDetectionCommon(foregroundVideo, originVideo, win_size, false, busyness, writer, outFlowVideo, of_time_interval, of_space_interval, bins, blockWidth, blockHeight, blockDeltaX, blockDeltaY, thresh);
}


/**
������
@param argc ��������
@param argv ����
@return ����ֵ
*/
int _tmain(int argc, char* argv[])
{
	if (argc < 3) {
		printf("�÷���\n");
		printf("AnomalyAnalysisWithOpticalFlow.exe �ļ�Ŀ¼ ѵ���ļ��� �����ļ��� [1-4] thresh [test | train] BLOCK_WIDTH BLOCK_HEIGHT\n");
		printf("���ӣ����еڶ��������Ĳ�����\n");
		printf("AnomalyAnalysisWithOpticalFlow.exe E:\videos train.avi test.avi  2\n");
		printf("���ӣ��޸ļ����� threshΪ0.2����\n");
		printf("AnomalyAnalysisWithOpticalFlow.exe E:\videos train.avi test.avi 3 0.2\n");
		printf("���ӣ����еڶ������ԣ���\n");
		printf("AnomalyAnalysisWithOpticalFlow.exe E:\videos train.avi test.avi  2  0.02 test\n");
		printf("���ӣ�ȫ�����У���\n");
		printf("AnomalyAnalysisWithOpticalFlow.exe E:\videos train.avi test.avi\n");
		return 0;
	}

	vector<string> files;
	char* folder = argv[1];
	char* trainfile = argv[2];
	char* testfile = argv[3];
	int runPart = 0;
	if (argc >= 4 && argv[4] != NULL) {
		runPart = atoi(argv[4]);
	}
	//�����ֵ����
	float thresh = 0.1f;
	if (argc >= 5 && argv[5] != NULL) {
		thresh = float(atof(argv[5]));
	}

	//���� bPerformDetection
	if (argc >= 6 && argv[6] != NULL) {
		bPerformDetection = atoi(argv[6]) != 0;
	}

	const int TRAIN = 1;
	const int TEST = 2;
	const int TRAIN_AND_TEST = 3;
	int trainOrTest = TRAIN_AND_TEST;
	if (argc >= 7 && argv[7] != NULL) {
		if (strcmp(argv[7], "train") == 0) {
			trainOrTest = TRAIN;
		}
		else if (strcmp(argv[7], "test") == 0) {
			trainOrTest = TEST;
		}
	}

	//blockSize
#ifdef BLOCK_16
	int blockWidth = 16, blockHeight = 16;
#else
	int blockWidth = 32, blockHeight = 32;
#endif

	if (argc >= 8 && argv[8] != NULL) {
		blockWidth = atoi(argv[8]);
	}
	if (argc >= 9 && argv[9] != NULL) {
		blockHeight = atoi(argv[9]);
	}

	char fileTrain[255];
	sprintf_s(fileTrain, "%s\\%s", folder, trainfile);
	char fileTrainBackground[255];
	sprintf_s(fileTrainBackground, "%s\\%s_background.avi", folder, trainfile);
	char fileTrainForeground[255];
	sprintf_s(fileTrainForeground, "%s\\%s_foreground.avi", folder, trainfile);

	char fileDetection[255];
	sprintf_s(fileDetection, "%s\\%s", folder, testfile);
	char fileDetectionBackground[255];
	sprintf_s(fileDetectionBackground, "%s\\%s_background.avi", folder, testfile);
	char fileDetectionForeground[255];
	sprintf_s(fileDetectionForeground, "%s\\%s_foreground.avi", folder, testfile);


	char fileTrainFlow[255];
	sprintf_s(fileTrainFlow, "%s\\%s_flow.avi", folder, trainfile);
	char fileDetectionFlow[255];
	sprintf_s(fileDetectionFlow, "%s\\%s_flow.avi", folder, testfile);

	char fileDetectionRes[255];
	sprintf_s(fileDetectionRes, "%s\\%s_result.avi", folder, testfile);
	char fileDetectionResCombined[255];
	sprintf_s(fileDetectionResCombined, "%s\\%s_�ϲ����.avi", folder, testfile);

	//char fileBusyness[255];
	//sprintf_s(fileBusyness, "%s\\%s_busyness.bmp", folder, trainfile);
	char fileBusynessTxt[255];
	sprintf_s(fileBusynessTxt, "%s\\%s_busyness.txt", folder, trainfile);
	//����
	int win_size = 5;
	//ע�⣺�޸�bins�����޸� typedef Vec<float, 10> Vec7f
	int of_time_interval = 2, of_space_interval = 1, bins = 9; //blockDeltaX = 16, blockDeltaY = 16;

#ifdef BLOCK_16
	int blockDeltaX = 8, blockDeltaY = 8;
#else
	int blockDeltaX = 16, blockDeltaY = 16;
#endif

	// �Ƿ񱣴�����
	bSaveFeatures = false;
	// �Ƿ�ִ�м��
	bPerformDetection = true;

	Mat busyness;

	// 1. ��ȡѵ�����ݺͼ����Ƶ�˶�Ŀ�겢����
	int frameForBackground = 100;
	int smallAreaLimit = 30;
	if (runPart == 0 || runPart == 1) {
		printf("1. ��ȡѵ�����ݺͼ����Ƶ�˶�Ŀ�겢����\n");
		if (trainOrTest == TRAIN || trainOrTest == TRAIN_AND_TEST) //�Ƿ���ȡѵ��ǰ��
			subtractMovingObject(fileTrain, fileTrainBackground, fileTrainForeground, frameForBackground, smallAreaLimit);
		if (trainOrTest == TEST || trainOrTest == TRAIN_AND_TEST) //�Ƿ���ȡ����ǰ��
			subtractMovingObject(fileDetection, fileDetectionBackground, fileDetectionForeground, frameForBackground, smallAreaLimit);
	}
	// 2. ���� busyness ͼ��
	if (runPart == 0 || runPart == 2) {
		if (bSaveFeatures) {
			char cmd[255];
			sprintf(feature_folder, "train_features");
			sprintf(cmd, "mkdir %s", feature_folder);
			system(cmd);
		}

		printf("2. ���� busyness ͼ��\n");
		busyness = computeBusyness(fileTrainForeground, fileTrain, win_size, fileTrainFlow, of_time_interval, of_space_interval, bins, blockWidth, blockHeight, blockDeltaX, blockDeltaY);
		saveImageAsText(busyness, fileBusynessTxt);
	}

	// 3. ���Ŀ����Ƶ
	if (runPart == 0 || runPart == 3) {
		printf("3. ���Ŀ����Ƶ\n");
		if (bSaveFeatures) {
			char cmd[255];
			sprintf(feature_folder, "detect_features");
			sprintf(cmd, "mkdir %s", feature_folder);
			system(cmd);
		}
		busyness = loadImageFromText(fileBusynessTxt);
		detectAbnomal(busyness, fileDetectionForeground, fileDetection, fileDetectionRes, win_size, fileDetectionFlow, of_time_interval, bins, of_space_interval, blockWidth, blockHeight, blockDeltaX, blockDeltaY, thresh);
	}

	// 4. �ϲ���Ƶ
	if (runPart == 0 || runPart == 4) {
		printf("4. �ϲ���Ƶ\n");
		combineVideoFiles(fileDetection, fileDetectionRes, fileDetectionResCombined, int(win_size * 0.5 + 0.5), ONE_COL);
	}
	return 0;
}

