// CImageProc.h
#pragma once
#include "pch.h"
#include <vector>

class CImageProc {
public:
    CImageProc();
    ~CImageProc();

    void OpenFile();
    void LoadBmp(CString stFileName);
    void ShowBMP(CDC* pDC);
    void GetColor(CClientDC* pDC, int x, int y);
	std::vector<int> CalculateHistogramMix(); // 计算灰度直方图
    std::vector<std::vector<int>> CalculateHistogramRGB();// 计算RGB直方图
	std::vector<std::vector<int>> Balance_Transformations(CClientDC& dc);    // 直方图均衡化

    bool isPaletteDarkToLight;
    HANDLE m_hDib;

    void ApplyBlackAndWhiteStyle();// 黑白风格
	void ApplyVintageStyle();  // 复古风格

    bool HistogramMatching(CImageProc& targetImageProc);
    void AddSaltAndPepperNoise(double noiseRatio, double saltRatio = 0.5); // 添加椒盐噪声，默认噪声比例为5%
    void AddImpulseNoise(double noiseRatio = 0.05, BYTE noiseValue1 = 0, BYTE noiseValue2 = 255); // 添加脉冲噪声
    void AddGaussianNoise(double mean = 0.0, double sigma = 30.0); // 添加高斯噪声
    void AddGaussianWhiteNoise(double sigma = 30.0); // 添加高斯白噪声
    void MeanFilter(int filterSize);    // 均值滤波
    void MedianFilter(int filterSize);  // 中值滤波
    void MaxFilter(int filterSize);     // 最大值滤波
public:
    
    BYTE* pDib;
    BITMAPFILEHEADER* pBFH;
    BITMAPINFOHEADER* pBIH;
    RGBQUAD* pQUAD;
    BYTE* pBits;
    int nWidth;
    int nHeight;
    int nBitCount;
    bool m_bIs565Format;

    void GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC);
    void GetColor4bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor8bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor16bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor24bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor32bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);

    bool IsValid() const { return m_hDib != NULL && pDib != NULL; }


    void CleanUp();
};