#pragma once
class CImageProc
{
public:
	CImageProc();
	~CImageProc();
	HGLOBAL m_hDib;
	BYTE* pDib;
	BITMAPFILEHEADER* pBFH;
	BITMAPINFOHEADER* pBIH;
	RGBQUAD* pQUAD;
	BYTE* pBits;
	int nWidth;
	int nHeight;
	int nBitCount;
	int rowSize;
public:
	void OpenFile();
	void LoadBmp(CString stFileName);
	void ShowBMP(CDC* pDC);
	void GetColor(CClientDC* pDC, int x, int y);
private:
	void GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC);
	void GetColor4bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
	void GetColor8bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x);
	void GetColor16bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
	void GetColor24bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
	void GetColor32bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue);
	bool m_bIs565Format; 
};

