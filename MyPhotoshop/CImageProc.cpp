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

// 打开文件
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

// 加载 BMP 文件
void CImageProc::LoadBmp(CString stFileName)
{
    // TODO: 在此处添加实现代码.
    CFile file;//文件对象
    CFileException e;//文件异常对象
    if (!file.Open(stFileName, CFile::modeRead | CFile::shareExclusive, &e))
    {
#ifdef _DEBUG
        afxDump << "File could not be opened " << e.m_cause << "\n";
#endif
    }
    else
    {
        ULONGLONG nFileSize;    //匹配GetLength函数的数据类型
        nFileSize = file.GetLength();   //获取文件大小
        m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, nFileSize);   //分配内存
        pDib = (BYTE*)::GlobalLock(m_hDib);   //锁定内存
        file.Read(pDib, nFileSize);   //读取文件
        pBFH = (BITMAPFILEHEADER*)pDib;     //指向文件头
        pBIH = (BITMAPINFOHEADER*)&pDib[sizeof(BITMAPFILEHEADER)];   //指向信息头
        pQUAD = (RGBQUAD*)&pDib[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];   //指向调色板
        pBits = (BYTE*)&pDib[pBFH->bfOffBits];   //指向位图数据
        nWidth = pBIH->biWidth;     // 获取图像的宽高
        nHeight = pBIH->biHeight;
        nNumColors = pBIH->biBitCount;   // 获取bmp位深度
    }
}


void CImageProc::ShowBMP(CDC* pDC)
{
    // TODO: 在此处添加实现代码.
    if (m_hDib != NULL)
    {
        ::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);      // 设置拉伸模式为 COLORONCOLOR
        ::StretchDIBits(pDC->m_hDC, 0, 0, pBIH->biWidth, pBIH->biHeight, 0, 0, pBIH->biWidth, pBIH->biHeight, pBits, (BITMAPINFO*)pBIH, DIB_RGB_COLORS, SRCCOPY);       // 将位图数据复制到目标DC的指定位置
    }
}

// 获取像素颜色
void CImageProc::GetColor(CClientDC* pDC, int x, int y)
{
    //检查坐标以及图像是否有效
    if (m_hDib == NULL || x < 0 || x >= nWidth || y < 0 || y >= nHeight)
    {
        return; // 无效坐标或未加载图像
    }

    // 每行字节数 = (每行的bit数 + 31) / 32 * 4     【每行字节数必须是4的倍数，即bit数是32的倍数，向上取整】
    int rowSize = ((nWidth * nNumColors + 31) / 32) * 4;


    //根据位深度计算出每个像素的起始位置

    float  bytePerPixel = nNumColors / 8;
    // 每个像素占用的字节数，nNumColors 为每个像素的位数【浮点数兼容低于8bit位图】

    int offset = (nHeight - 1 - y) * rowSize + x * int(bytePerPixel);
    // 偏移量 = (图像高度 - 1 - 纵坐标) * 每行字节数 + 横坐标 * 每个像素占用的字节数   【y的范围是[0,nHeight-1]】 
    // 【强制类型转换，对于低于8bit图像，pixel指向当前像素所在字节的起始位置】

    BYTE* pixel = pBits + offset;// 获取像素在位图数据中的位置（起始点+偏移量）

    //  RGB 值
    BYTE red = 0, green = 0, blue = 0;

    switch (nNumColors)
    {
    case 1: // 1位位图
    {
        CImageProc::GetColor1bit(pixel,red,green,blue,x,y,pDC);

        break;
    }
    case 2: // 2位位图
    {
        CImageProc::GetColor2bit(pixel, red, green, blue, x);

        break;
    }
    case 4: // 4位位图
    {
        CImageProc::GetColor4bit(pixel, red, green, blue, x);
        break;
    }
    case 8: // 8位位图
    {
        CImageProc::GetColor8bit(pixel, red, green, blue, x);
        break;
    }
    case 16: // 16位位图
    {
        WORD pixelValue = *((WORD*)pixel);
        red = (pixelValue & 0x7C00) >> 10;
        green = (pixelValue & 0x03E0) >> 5;
        blue = pixelValue & 0x001F;
        red <<= 3;
        green <<= 3;
        blue <<= 3;
        break;
    }
    case 24: // 24位位图
    {
        red = pixel[2];
        green = pixel[1];
        blue = pixel[0];
        break;
    }
    case 32: // 32位位图
    {
        red = pixel[2];
        green = pixel[1];
        blue = pixel[0];
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



void CImageProc::GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC)
{
    BYTE index = (*pixel >> (7 - x % 8)) & 0x01;
    red = pQUAD[index].rgbRed;
    green = pQUAD[index].rgbGreen;
    blue = pQUAD[index].rgbBlue;

    // 由于原始图像直接转换成1bit图像之后，黑白像素交错分布，难以确定是否正确显示
    // 以下代码用于查看当前像素值，从而验证是否显示正确

    CString str2;
    str2.Format(L"pixel：(%u);index：(%d)", *pixel, index);
    // 获取文本高度
    CSize textSize = pDC->GetTextExtent(str2);
    pDC->TextOutW(x, y + textSize.cy * 2, str2);
}

void CImageProc::GetColor2bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x)
{
    BYTE index = (*pixel >> (7 - x % 4)) & 0x03;
    red = pQUAD[index].rgbRed;
    green = pQUAD[index].rgbGreen;
    blue = pQUAD[index].rgbBlue;
}

void CImageProc::GetColor4bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x)
{
    BYTE index = (x % 2 == 0) ? (*pixel >> 4) : (*pixel & 0x0F);
    red = pQUAD[index].rgbRed;
    green = pQUAD[index].rgbGreen;
    blue = pQUAD[index].rgbBlue;
}

void CImageProc::GetColor8bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x)
{
    BYTE index = *pixel;
    red = pQUAD[index].rgbRed;
    green = pQUAD[index].rgbGreen;
    blue = pQUAD[index].rgbBlue;
}