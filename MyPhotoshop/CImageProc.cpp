#include "pch.h"
#include "CImageProc.h"
#include <afxdlgs.h>
#include <vector>

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
    nBitCount = 0;
    m_bIs565Format = true; // 默认假设为565格式
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
		nBitCount = pBIH->biBitCount;   //获取位深度
   
        if (pBIH->biCompression == BI_RGB && nBitCount == 16) {
            //16位图默认为555格式
            m_bIs565Format = false;
        }
        else if (pBIH->biCompression == BI_BITFIELDS && nBitCount == 16) {
            // 检查颜色掩码是否为565
            DWORD* masks = reinterpret_cast<DWORD*>(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
            DWORD redMask = masks[0];
            DWORD greenMask = masks[1];
            DWORD blueMask = masks[2];

            m_bIs565Format = (redMask == 0xF800 && greenMask == 0x07E0 && blueMask == 0x001F);
        }
        else {
            m_bIs565Format = false;
        }
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
    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;


    //根据位深度计算出每个像素的起始位置

    float  bytePerPixel = float(nBitCount)/ 8;
    // 每个像素占用的字节数，nNumColors 为每个像素的位数【浮点数兼容低于8bit位图】

    int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
    // 偏移量 = (图像高度 - 1 - 纵坐标) * 每行字节数 + 横坐标 * 每个像素占用的字节数   【y的范围是[0,nHeight-1]】 
    // 【强制类型转换，对于低于8bit图像，pixel指向当前像素所在字节的起始位置】

    BYTE* pixel = pBits + offset;// 获取像素在位图数据中的位置（起始点+偏移量）

    //  RGB 值
    BYTE red = 0, green = 0, blue = 0;

    switch (nBitCount)
    {
    case 1: // 1位位图
    {
        CImageProc::GetColor1bit(pixel,red,green,blue,x,y,pDC);

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
        CImageProc::GetColor16bit(pixel, red, green, blue);
        break;
    }
    case 24: // 24位位图
    {
        CImageProc::GetColor24bit(pixel, red, green, blue);
        break;
    }
    case 32: // 32位位图
    {
        CImageProc::GetColor32bit(pixel, red, green, blue);
        break;
    }
    default:
        return; // 不支持的颜色深度
    }

    // 使用 GetPixel 获取像素颜色
    COLORREF pixelColor = pDC->GetPixel(x, y);
    BYTE getPixelRed = GetRValue(pixelColor);
    BYTE getPixelGreen = GetGValue(pixelColor);
    BYTE getPixelBlue = GetBValue(pixelColor);

    // 设置文本背景不透明
    pDC->SetBkMode(OPAQUE);

    // 设置文本颜色为黑色
    pDC->SetTextColor(RGB(0, 0, 0));

    // 格式化 RGB 值和坐标信息
    CString str;
    str.Format(L"RGB: (%d, %d, %d)", red, green, blue);

    CString getPixelStr;
    getPixelStr.Format(L"GetPixel RGB: (%d, %d, %d)", getPixelRed, getPixelGreen, getPixelBlue);

    CString location;
    location.Format(L"location：(%d, %d)", x, y);

    // 在鼠标点击位置显示 RGB 值
    pDC->TextOutW(x, y, str);

    // 获取文本高度
    CSize textSize = pDC->GetTextExtent(str);

    // 在下一行显示 GetPixel 获取的 RGB 值
    pDC->TextOutW(x, y + textSize.cy, getPixelStr);

    // 在下一行显示坐标
    pDC->TextOutW(x, y + textSize.cy * 2, location);

}
void CImageProc::GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC)
{
    BYTE index = (*pixel >> (7 - x % 8)) & 0x01;
    red = pQUAD[index].rgbRed;
    green = pQUAD[index].rgbGreen;
    blue = pQUAD[index].rgbBlue;

    // 由于原始图像直接转换成1bit图像之后，黑白像素交错分布，难以确定是否正确显示
    // 以下代码用于查看当前像素值，从而验证是否显示正确

    //CString str2;
    //str2.Format(L"pixel：(%u);index：(%d)", *pixel, index);
    //// 获取文本高度
    //CSize textSize = pDC->GetTextExtent(str2);
    //pDC->TextOutW(x, y + textSize.cy * 2, str2);
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
void CImageProc::GetColor16bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue)
{
    WORD pixelValue = *((WORD*)pixel);
    if (m_bIs565Format) {
        // 提取565格式的RGB分量
        red = (pixelValue & 0xF800) >> 11;    
        green = (pixelValue & 0x07E0) >> 5;   
        blue = pixelValue & 0x001F;           

        // 将分量扩展到8位
        red = (red << 3) | (red >> 2);        
        green = (green << 2) | (green >> 4);  
        blue = (blue << 3) | (blue >> 2);    
    } else {
        // 提取555格式的RGB分量
        red = (pixelValue & 0x7C00) >> 10;   
        green = (pixelValue & 0x03E0) >> 5;   
        blue = pixelValue & 0x001F;           

        // 将分量扩展到8位
        red = (red << 3) | (red >> 2);
        green = (green << 3) | (green >> 2);
        blue = (blue << 3) | (blue >> 2);
    }
}
void CImageProc::GetColor24bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue)
{
    red = pixel[2];
    green = pixel[1];
    blue = pixel[0];
}
void CImageProc::GetColor32bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue)
{
    red = pixel[2];
    green = pixel[1];
    blue = pixel[0];
}

// 计算灰度直方图
std::vector<int> CImageProc::CalculateGrayHistogram()
{
    std::vector<int> histogram(256, 0);

    if (m_hDib == NULL)
    {
        return histogram;
    }

    // 每行字节数 = (每行的bit数 + 31) / 32 * 4
    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;

    float bytePerPixel = float(nBitCount) / 8;

    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
            BYTE* pixel = pBits + offset;

            BYTE red = 0, green = 0, blue = 0;

            switch (nBitCount)
            {
            case 1:
                GetColor1bit(pixel, red, green, blue, x, y, nullptr);
                break;
            case 4:
                GetColor4bit(pixel, red, green, blue, x);
                break;
            case 8:
                GetColor8bit(pixel, red, green, blue, x);
                break;
            case 16:
                GetColor16bit(pixel, red, green, blue);
                break;
            case 24:
                GetColor24bit(pixel, red, green, blue);
                break;
            case 32:
                GetColor32bit(pixel, red, green, blue);
                break;
            default:
                continue;
            }

            // 计算灰度值
            int gray = static_cast<int>(0.299 * red + 0.587 * green + 0.114 * blue);
            histogram[gray]++;
        }
    }

    return histogram;
}
