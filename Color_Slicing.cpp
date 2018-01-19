#include <iostream>
#include <math.h>
#include <algorithm>

using namespace std;

void GaussianFilter(unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth);
void SmoothFilter(unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth);
bool SkinTrue(unsigned char R, unsigned char G, unsigned char B);
unsigned char** MemAlloc2D(int nHeight, int nWidth, unsigned char nInitVal);
void MemFree2D(unsigned char **Mem, int nHeight);
unsigned char** Padding(unsigned char** In, int nHeight, int nWidth, int nFilterSize);
void RGB_input(unsigned char* RGB, unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth);
void RGB_output(unsigned char* RGB, unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth);

int main()
{
	cout << "파일을 생성중입니다..." << endl;

	int nHeight = 512;
	int nWidth = 512;
	unsigned char* header = (unsigned char*)malloc(sizeof(unsigned char) * 54);
	unsigned char* RGB = (unsigned char*)malloc(sizeof(unsigned char)*(nHeight*nWidth * 3));
	
	// Bmp 포맷의 헤더 54byte를 읽어들인다.
	FILE* input;
	fopen_s(&input, "test3.bmp", "rb");
	fread(header, 54, sizeof(unsigned char), input);
	fread(RGB, nHeight*nWidth * 3, sizeof(unsigned char), input);
	fclose(input);

	unsigned char** R = MemAlloc2D(nHeight, nWidth, 0);
	unsigned char** G = MemAlloc2D(nHeight, nWidth, 0);
	unsigned char** B = MemAlloc2D(nHeight, nWidth, 0);

	// Skin True를 적용하기 위해 raw 파일을 R, G, B를 분리
	RGB_input(RGB, R, G, B, nHeight, nWidth);
	

	// 선택적으로 Filter를 사용한다.
	SmoothFilter(R, G, B, nHeight, nWidth);
	//GaussianFilter(R, G, B, nHeight, nWidth);


	// 적용된 결과를 다시 RGB 스트림으로 변환
	RGB_output(RGB, R, G, B, nHeight, nWidth);
	
	FILE* Output;
	fopen_s(&Output, "result3_255.raw", "wb");
	fwrite(RGB, nHeight*nWidth * 3, sizeof(unsigned char), Output);
	fclose(Output);

	delete RGB;
	MemFree2D(R, nHeight);
	MemFree2D(G, nHeight);
	MemFree2D(B, nHeight);
	return 0;
}


void GaussianFilter(unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth)
{
	double RTemp = 0, GTemp = 0, BTemp = 0; int nPadSize = (int)(3 / 2), FilterSize = 3;
	unsigned char** R_Pad = Padding(R, nHeight, nWidth, FilterSize);
	unsigned char** G_Pad = Padding(G, nHeight, nWidth, FilterSize);
	unsigned char** B_Pad = Padding(B, nHeight, nWidth, FilterSize);

	double stdv = 1;
	double r, s = 2.0 * stdv * stdv;
	double sum = 0.0;

	double gaussian[3][3];

	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			r = (x*x + y*y);
			gaussian[x + 1][y + 1] = (exp(-(r) / s)) / sqrt(3.14 * s);
			sum += gaussian[x + 1][y + 1];
		}
	}

	for (int i = 0; i < 3; ++i) // Loop to normalize the kernel
		for (int j = 0; j < 3; ++j)
			gaussian[i][j] /= sum;


	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			int real_h = h + 1;
			int real_w = w + 1;

			// 피부색으로 판단되는 부분만 Filter를 적용한다.
			if (SkinTrue(R_Pad[real_h][real_w], G_Pad[real_h][real_w], B_Pad[real_h][real_w]))
			{
				RTemp = 0; GTemp = 0; BTemp = 0; 
				for (int n = 0; n < FilterSize; n++)
					for (int m = 0; m < FilterSize; m++)
					{
							RTemp += (double)R_Pad[h + n][w + m] * gaussian[n][m];
							GTemp += (double)G_Pad[h + n][w + m] * gaussian[n][m];;
							BTemp += (double)B_Pad[h + n][w + m] * gaussian[n][m];
					}
				R[h][w] = RTemp;
				G[h][w] = GTemp;
				B[h][w] = BTemp;
			}
			else
			{
				R[h][w] = R_Pad[real_h][real_w];
				G[h][w] = G_Pad[real_h][real_w];
				B[h][w] = B_Pad[real_h][real_w];
			}
		}
	}
}


void SmoothFilter(unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth)
{
	double RTemp = 0, GTemp = 0, BTemp = 0; int maskContain = 0, nPadSize = (int)(5 / 2), FilterSize = 5;
	unsigned char** R_Pad = Padding(R, nHeight, nWidth, FilterSize);
	unsigned char** G_Pad = Padding(G, nHeight, nWidth, FilterSize);
	unsigned char** B_Pad = Padding(B, nHeight, nWidth, FilterSize);

	double Mask[5][5] = {
		{ 0, 0, 1, 0, 0 },
		{ 0, 2, 2, 2, 0 },
		{ 1, 2, 5, 2, 1 },
		{ 0, 2, 2, 2, 0 },
		{ 0, 0, 1, 0, 0 }
	};

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			int real_h = h + 1;
			int real_w = w + 1;
			if (SkinTrue(R_Pad[real_h][real_w], G_Pad[real_h][real_w], B_Pad[real_h][real_w]))
			{
				RTemp = 0; GTemp = 0; BTemp = 0; maskContain = 0;
				for (int n = 0; n < FilterSize; n++)
					for (int m = 0; m < FilterSize; m++)
					{
						// 피부색으로 판단되는 부분만 Filter를 적용한다.
						if (SkinTrue(R_Pad[h + n][w + m], G_Pad[h + n][w + m], B_Pad[h + n][w + m]))
						{
							maskContain += Mask[n][m];
							RTemp += (double)R_Pad[h + n][w + m] * Mask[n][m];
							GTemp += (double)G_Pad[h + n][w + m] * Mask[n][m];;
							BTemp += (double)B_Pad[h + n][w + m] * Mask[n][m];
						}
					}
				R[h][w] = ((double)RTemp / maskContain);
				G[h][w] = ((double)GTemp / maskContain);
				B[h][w] = ((double)BTemp / maskContain);
				
			}
			else
			{
				R[h][w] = R_Pad[real_h][real_w];
				G[h][w] = G_Pad[real_h][real_w];
				B[h][w] = B_Pad[real_h][real_w];
			}
		}
	}

	MemFree2D(R_Pad, nHeight + 2 * nPadSize);
	MemFree2D(G_Pad, nHeight + 2 * nPadSize);
	MemFree2D(B_Pad, nHeight + 2 * nPadSize);
}

bool SkinTrue(unsigned char R, unsigned char G, unsigned char B)
{
	unsigned char *RGB_sorting = (unsigned char*)malloc(sizeof(unsigned char) * 3);
	unsigned char *temp = RGB_sorting;

	if (R > 95 && G > 40 && B > 20)
	{

		*RGB_sorting = R; RGB_sorting++;
		*RGB_sorting = G; RGB_sorting++;
		*RGB_sorting = B;

		RGB_sorting = temp;
		sort(RGB_sorting, RGB_sorting + 3);

		unsigned char Min = *RGB_sorting;
		unsigned char Max = *(RGB_sorting + 2);

		if (Max - Min > 15 && abs(R - G) > 15 && R > G && R > B)
		{
			delete RGB_sorting;
			return true;
		}
	}

	delete RGB_sorting;
	return false;
}

unsigned char** MemAlloc2D(int nHeight, int nWidth, unsigned char nInitVal)
{
	unsigned char** rtn = new unsigned char*[nHeight];
	for (int n = 0; n < nHeight; n++)
	{
		rtn[n] = new unsigned char[nWidth];
		memset(rtn[n], nInitVal, sizeof(unsigned char) * nWidth);
	}
	return rtn;
}

void MemFree2D(unsigned char **Mem, int nHeight)
{
	for (int n = 0; n < nHeight; n++)
	{
		delete[] Mem[n];
	}
	delete[] Mem;
}

unsigned char** Padding(unsigned char** In, int nHeight, int nWidth, int nFilterSize)
{
	int nPadSize = (int)(nFilterSize / 2);
	unsigned char** Pad = MemAlloc2D(nHeight + 2 * nPadSize, nWidth + 2 * nPadSize, 0);

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			Pad[h + nPadSize][w + nPadSize] = In[h][w];
		}
	}
	for (int h = 0; h < nPadSize; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			Pad[h][w + nPadSize] = In[0][w];
			Pad[h + (nHeight - 1) + nPadSize][w + nPadSize] = In[nHeight - 1][w];
		}
	}

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nPadSize; w++)
		{
			Pad[h + nPadSize][w] = In[h][0];
			Pad[h + nPadSize][w + (nWidth - 1) + nPadSize] = In[h][nWidth - 1];
		}
	}

	for (int h = 0; h < nPadSize; h++)
	{
		for (int w = 0; w < nPadSize; w++)
		{
			Pad[h][w] = In[0][0];
			Pad[h + (nHeight - 1) + nPadSize][w] = In[nHeight - 1][0];
			Pad[h][w + (nWidth - 1) + nPadSize] = In[0][nWidth - 1];
			Pad[h + (nHeight - 1) + nPadSize][w + (nWidth - 1) + nPadSize] = In[nHeight - 1][nWidth - 1];
		}
	}
	return Pad;
}

void RGB_input(unsigned char* RGB, unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth)
{
	int rgb_index = 0;
	int h = 0, w = 0;

	for (rgb_index = 0; rgb_index < nHeight*nWidth * 3; rgb_index++)
		if (rgb_index % 3 == 0)
		{
			B[h][w++] = RGB[rgb_index];
			if (w % nWidth == 0)
			{
				w = 0;
				h++;
			}
		}

	h = 0, w = 0;
	for (rgb_index = 0; rgb_index < nHeight*nWidth * 3; rgb_index++)
		if (rgb_index % 3 == 1)
		{
			G[h][w++] = RGB[rgb_index];
			if (w % nWidth == 0)
			{
				w = 0;
				h++;
			}
		}

	h = 0, w = 0;
	for (rgb_index = 0; rgb_index < nHeight*nWidth * 3; rgb_index++)
		if (rgb_index % 3 == 2)
		{
			R[h][w++] = RGB[rgb_index];
			if (w % nWidth == 0)
			{
				w = 0;
				h++;
			}
		}
}

void RGB_output(unsigned char* RGB, unsigned char** R, unsigned char** G, unsigned char** B, int nHeight, int nWidth)
{
	int rgb_index;
	int	h = nHeight-1, w = 0;

	for (rgb_index = 0; rgb_index < nHeight*nWidth * 3; rgb_index++)
		if (rgb_index % 3 == 0)
		{
			RGB[rgb_index] = R[h][w++];
			if (w == nWidth)
			{
				w = 0;
				h--;
			}
		}

	h = nHeight-1, w = 0;
	for (rgb_index = 0; rgb_index < nHeight*nWidth * 3; rgb_index++)
		if (rgb_index % 3 == 1)
		{
			RGB[rgb_index] = G[h][w++];
			if (w == nWidth)
			{
				w = 0;
				h--;
			}
		}
	h = nHeight-1, w = 0;
	for (rgb_index = 0; rgb_index < nHeight*nWidth * 3; rgb_index++)
		if (rgb_index % 3 == 2)
		{
			RGB[rgb_index] = B[h][w++];
			if (w == nWidth)
			{
				w = 0;
				h--;
			}
		}
}
