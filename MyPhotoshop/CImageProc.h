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
	int nNumColors;
public:
	void OpenFile();
	void LoadBmp(CString stFileName);
	void ShowBMP(CDC* pDC);
	void GetColor(CClientDC* pDC, int x, int y);
};

