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
    void ApplyBlackAndWhiteStyle();// 黑白风格
    HANDLE m_hDib;

	void ApplyVintageStyle();  // 复古风格
    void ApplyVintageToTrueColor();  // 处理真彩色的图片
    void ApplyVintageToPalette();   // 处理调色板格式的图片
    void ApplyVintageTo16Bit();      // 处理16位格式的图片

    void CreateVintagePalette();// 创建复古调色板

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