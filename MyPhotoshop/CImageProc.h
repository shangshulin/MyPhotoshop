#pragma once

class CImageProc
{
public:
    CImageProc();
    ~CImageProc();

private:
    HGLOBAL m_hDib;
    BYTE* pDib;
    BITMAPFILEHEADER* pBFH;
    BITMAPINFOHEADER* pBIH;
    RGBQUAD* pQUAD;
    BYTE* pBits;
    int nWidth;
    int nHeight;
    int nNumColors;

public:
    void OpenFile();
    void LoadBmp(CString stFileName);
    void ShowBMP(CDC* pDC);
    void GetColor(CClientDC* pDC, int x, int y);

private:
    void GetColor16(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor24(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
    void GetColor32(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
};
