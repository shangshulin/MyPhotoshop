#include "pch.h"
#include "CImageProc.h"
CImageProc::CImageProc()
{
    m_hDib = NULL;
    pDib = new BYTE;
    pBFH = new BITMAPFILEHEADER;
    pBIH = new BITMAPINFOHEADER;
    pQUAD = new RGBQUAD;
    pBits = new BYTE;
    nWidth = 0;
    nHeight = 0;
    nNumColors = 0;
}
CImageProc::~CImageProc()
{
    delete pDib;
    delete pBFH;
    delete pBIH;
    delete pQUAD;
    delete pBits;
    if (m_hDib != NULL)
        GlobalUnlock(m_hDib);
}


void CImageProc::OpenFile()
{
    // TODO: 在此处添加实现代码.
    CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"Bmp File(*.bmp)|*.bmp|JPG File(*.jpg)|*.jpg|All Files(*.*)|*.*||", NULL);
    if (fileDlg.DoModal() == IDOK)
    {
        CString stpathname = fileDlg.GetPathName();
        LoadBmp(stpathname);
    }
    else
        return;
}

void CImageProc::LoadBmp(CString stFileName)
{
    // TODO: 在此处添加实现代码.
    CFile file;
    CFileException e;
    if (!file.Open(stFileName, CFile::modeRead | CFile::shareExclusive, &e))
    {
#ifdef _DEBUG
        afxDump << "File could not be opened " << e.m_cause << "\n";
#endif
    }
    else
    {
        long nFileSize;
        nFileSize = file.GetLength();
        m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, nFileSize);
        pDib = (BYTE*)::GlobalLock(m_hDib);
        file.Read(pDib, nFileSize);
        pBFH = (BITMAPFILEHEADER*)pDib;
        pBIH = (BITMAPINFOHEADER*)&pDib[sizeof(BITMAPFILEHEADER)];
        pQUAD = (RGBQUAD*)&pDib[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];
        pBits = (BYTE*)&pDib[pBFH->bfOffBits];
        nWidth = pBIH->biWidth;
        nHeight = pBIH->biHeight;
        nNumColors = pBIH->biBitCount;
    }
}


void CImageProc::ShowBMP(CDC* pDC)
{
    // TODO: 在此处添加实现代码.
    if (m_hDib != NULL)
    {
        ::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
        ::StretchDIBits(pDC->m_hDC, 0, 0, pBIH->biWidth, pBIH->biHeight, 0, 0, pBIH->biWidth, pBIH->biHeight, pBits, (BITMAPINFO*)pBIH, DIB_RGB_COLORS, SRCCOPY);
    }
}


void CImageProc::GetColor(CClientDC* pDC, int x, int y)
{
    if (m_hDib == NULL || x < 0 || x >= nWidth || y < 0 || y >= nHeight)
    {
        return; // 无效坐标或未加载图像
    }

    // 计算像素在 pBits 中的位置
    int bytePerPixel = nNumColors / 8;
    if (nNumColors < 8)
        bytePerPixel = 1;

    int rowSize = ((nWidth * nNumColors + 31) / 32) * 4; // 每行的字节数
    int offset = (nHeight - 1 - y) * rowSize + x * bytePerPixel; // 计算偏移量

    BYTE* pixel = pBits + offset;

    BYTE red = 0, green = 0, blue = 0;

    switch (nNumColors)
    {
    case 1: // 1位位图
    {
        BYTE mask = 0x80 >> (x % 8);
        BYTE index = (*pixel & mask) ? 1 : 0;
        red = pQUAD[index].rgbRed;
        green = pQUAD[index].rgbGreen;
        blue = pQUAD[index].rgbBlue;
        break;
    }
    case 4: // 4位位图
    {
        BYTE index = (x % 2 == 0) ? (*pixel >> 4) : (*pixel & 0x0F);
        red = pQUAD[index].rgbRed;
        green = pQUAD[index].rgbGreen;
        blue = pQUAD[index].rgbBlue;
        break;
    }
    case 8: // 8位位图
    {
        BYTE index = *pixel;
        red = pQUAD[index].rgbRed;
        green = pQUAD[index].rgbGreen;
        blue = pQUAD[index].rgbBlue;
        break;
    }
    case 16: // 16位位图
    {
        GetColor16(pixel, red, green, blue);
        break;
    }
    case 24: // 24位位图
    {
        GetColor24(pixel, red, green, blue);
        break;
    }
    case 32: // 32位位图
    {
        GetColor32(pixel, red, green, blue);
        break;
    }
    default:
        return; // 不支持的颜色深度
    }

    // 设置文本背景不透明
    pDC->SetBkMode(OPAQUE);

    // 设置文本颜色为黑色
    pDC->SetTextColor(RGB(0, 0, 0));

    // 格式化 RGB 值和坐标信息
    CString str;
    str.Format(L"RGB: (%d, %d, %d)", red, green, blue);

    CString location;
    location.Format(L"坐标：(%d, %d)", x, y);

    // 在鼠标点击位置显示 RGB 值
    pDC->TextOutW(x, y, str);

    // 获取文本高度
    CSize textSize = pDC->GetTextExtent(str);

    // 在下一行显示坐标
    pDC->TextOutW(x, y + textSize.cy, location);
}

void CImageProc::GetColor16(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue)
{
    WORD pixelValue = *((WORD*)pixel);
    red = (pixelValue & 0x7C00) >> 10;
    green = (pixelValue & 0x03E0) >> 5;
    blue = pixelValue & 0x001F;
    red <<= 3;
    green <<= 3;
    blue <<= 3;
}

void CImageProc::GetColor24(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue)
{
    red = pixel[2];
    green = pixel[1];
    blue = pixel[0];
}

void CImageProc::GetColor32(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue)
{
    red = pixel[2];
    green = pixel[1];
    blue = pixel[0];
}
