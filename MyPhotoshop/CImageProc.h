// CImageProc.h
#pragma once
#include "pch.h"
#include <vector>

class CImageProc {
public:
    CImageProc();
    ~CImageProc();
    bool IsValid() const { return m_hDib != NULL && pDib != NULL; }
    void CleanUp();

    //加载与显示
    void OpenFile();
    void LoadBmp(CString stFileName);
    void ShowBMP(CDC* pDC);
    void DisplayColor(CClientDC* pDC, int x, int y);
    void GetColor(int x, int y,BYTE& red, BYTE& green, BYTE& blue);

    // 获取不同位宽图像颜色
    void GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC);
    void GetColor4bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor8bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
    void GetColor16bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor24bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor32bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);

    //灰度处理
	std::vector<int> CalculateHistogramMix(); // 计算灰度直方图
    std::vector<std::vector<int>> CalculateHistogramRGB();// 计算RGB直方图
	std::vector<std::vector<int>> Balance_Transformations(CClientDC& dc);    // 直方图均衡化
    bool HistogramMatching(CImageProc& targetImageProc);// 直方图规格化
	void InvertGrayscale(); // 反转灰度图像
    void ApplyBlackAndWhiteStyle();// 黑白风格
	void ApplyVintageStyle();  // 复古风格

    //空域滤波
    void ApplySobelEdgeDetection();// Sobel算子边缘检测
    void ApplyPrewittEdgeDetection();// Prewitt算子边缘检测
	void ApplyRobertEdgeDetection();// Robert算子边缘检测
	void ApplyCannyEdgeDetection();// Canny算子边缘检测
	void ApplyLaplaceEdgeDetection();

    // 拉普拉斯算子边缘检测

    // 图像操作
    void Add(CImageProc& img, double weight1, double weight2); // 图像相加
    void Multiply(CImageProc& img);    // 图像相乘
    void PowerTransform(double gamma); // 幂律变换
    void ApplyMeanFilter(); // 均值滤波
public:
    BYTE* pDib;
    BITMAPFILEHEADER* pBFH;
    BITMAPINFOHEADER* pBIH;
    RGBQUAD* pQUAD;
    BYTE* pBits;
    HANDLE m_hDib;

    int nWidth;
    int nHeight;
    int nBitCount;
    bool m_bIs565Format;
    bool isPaletteDarkToLight;

public:
    // 深拷贝赋值运算符
    CImageProc& operator=(const CImageProc& other);
    void CopyDIBData(const CImageProc& other);
private:
    void FreeImageData() {
        if (m_hDib != NULL) {
            ::GlobalFree(m_hDib);
            m_hDib = NULL;
            pBits = NULL;
            // ... 其他指针置空 ...
        }
    }
};