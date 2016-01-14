#ifndef MATRIX_H_TOMHEAVEN_20140730
#define MATRIX_H_TOMHEAVEN_20140730

#define  _CRT_SECURE_NO_WARNINGS

/************************************************************************/
/** ����ṹ Matrix ���ڼ���                                            */
/************************************************************************/

typedef struct Matrix {
	int ** data;
	int width;
	int height;

	/** ���캯��  */
	Matrix() {
		width = height = 0;
		data = NULL;
	}
	/** ���캯��1 */
	Matrix(int w, int h) {
		width = w;
		height = h;

		data = new int*[h];
	    for(int i = 0; i < h; ++i) {
			data[i] = new int[w];
			for(int j = 0; j <w; ++j)
				data[i][j] = 0;
		}
	}

	/** �������� */
	~Matrix() {
		release();
	}

	/** �ͷ��ڴ� */
	void release() {
		for(int i = 0; i < height; ++i) {
			if (data != NULL && data[i] != NULL)
				delete(data[i]);
		}
		if (data != NULL)
			delete(data); 
	}

	/** ���캯��2 */
	Matrix(const Matrix& m) {
		width = m.width;
		height = m.height;

		data = new int*[height];
		for(int i = 0; i < height; ++i) {
			data[i] = new int[width];
			for(int j = 0; j <width; ++j)
				data[i][j] = m.data[i][j];
		}
	}

	/** ��ӡ */
	void printMatrix() {
		for(int i = 0; i < height; ++i) {
			for(int j = 0; j <width; ++j)
				printf("%d ", data[i][j]);
			printf("\n");

		}
	}

	/** ��������ļ�
	   @param filepath ������ļ�·��
	   @return һ��boolֵ����ʾ�Ƿ�ɹ���
	*/
	bool saveMatrix(const char* filepath) {
		FILE* fout = NULL;
		fopen_s(&fout, filepath, "w");
		if (fout == NULL)
			return false;
		fprintf(fout, "%d %d\n", height, width);
		for(int i = 0; i < height; ++i) {
			for(int j = 0; j <width; ++j)
				fprintf(fout, "%d ", data[i][j]);
			fprintf(fout, "\n");

		}
		fclose(fout);
		return true;
	}

	/** ���ļ����ؾ���
	  @param filepath ���ص��ļ�·��
	  @return һ��boolֵ����ʾ�Ƿ�ɹ���
	*/
	bool loadMatrix(const char* filepath) {
		//�ͷ������ڴ�
		for(int i = 0; i < height; ++i) {
			if (data != NULL && data[i] != NULL)
			    delete(data[i]);
		}
		if (data != NULL)
		    delete(data);
		

		FILE* fin = NULL;
		fopen_s(&fin, filepath, "r");
		if (fin == NULL)
			return false;
		fscanf_s(fin, "%d%d", &height, &width);

		//�������ڴ沢��ȡ����
		data = new int*[height];
		for(int i = 0; i < height; ++i) {
			data[i] = new int[width];
			for(int j = 0; j < width; ++j)
				fscanf_s(fin, "%d", &data[i][j]);
		}

		fclose(fin);
		return true;
	}

	

	/** ������ͬ�ξ����ӦԪ��֮�ͣ�����ֵ������ 
	    @param m ������ĵڶ�������
	*/
	void add(const Matrix& m) {
		CV_Assert(height == m.height && width == m.width);
		for(int i = 0; i < height; ++i) {
			for(int j = 0; j <width; ++j) {
                 data[i][j] += m.data[i][j];
			}
		}
	}

	/** ������ͬ�ξ����ӦԪ��֮�����ֵ������ 
	    @param m ������ĵڶ�������
	*/
	void subtract(const Matrix& m) {
		CV_Assert(height != m.height || width != m.width);
		for(int i = 0; i < height; ++i) {
			for(int j = 0; j <width; ++j) {
                 data[i][j] -= m.data[i][j];
			}
		}
	}


	/** ������Ԫ�����ֵ
	 @return ���ֵ int
	*/
	int max() {
		int maxValue = 0;
		for(int i = 0; i < height; ++i) {
			for(int j = 0; j <width; ++j) {
				if (data[i][j] > maxValue) {
					maxValue = data[i][j];
				}
			}
		}
		return maxValue;
	}

	/** ������ͬ�ξ����ӦԪ�����ֵ������ֵ������ */
	void max(const Matrix& m) {
		CV_Assert(height == m.height && width == m.width);
		for(int i = 0; i < width; ++i) {
			for(int j = 0; j < height; ++j) {
				data[j][i] = std::max(data[j][i], m.data[j][i]);
			}
		}
	}

	/** ��һ������ֵ�� [0, 255] */
	void normalize() {
		int maxValue = max();
		for(int i = 0; i < height; ++i) {
			for(int j = 0; j <width; ++j) {
			    data[i][j] = (int)(float(data[i][j]) / maxValue * 255 + 0.5);
			}
		}
	}
	
	/** ת��Ϊ IplImage ͼ��ע������֮��Ҫ cvRleaseImage(IlpImage**)�����������ڴ�й©��
	    @return ��ͨ��IplImage*
	*/
	IplImage* toIplImage() {
		Matrix m(*this);
		m.normalize();
		IplImage* img = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
		for(int i = 0; i < height; ++i) {
			for(int j = 0; j <width; ++j) {
				setPixel(img, j, i, m.data[i][j]);
			//	if (m.data[i][j] == 0)
				//    printf("m.data[i][j] = %d\n", m.data[i][j]);
			}
		}
		return img;
	}
}  Matrix;

#endif