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
    std::vector<int> CalculateGrayHistogram();
    HANDLE m_hDib;

    void ApplyVintageStyle();  // 复古风格 修改后的主入口
    void ApplyVintageToTrueColor(); // 24/32位处理
    void ApplyVintageToPalette();   // 1/4/8位处理
    void ApplyVintageTo16Bit();     // 16位处理

    // 调色板操作
    void CreateVintagePalette();

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

    void CleanUp();
    bool IsValid() const { return m_hDib != NULL && pDib != NULL; }
private:
    void InitializeMembers();
};