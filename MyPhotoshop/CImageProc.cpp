#pragma execution_character_set("utf-8")
#include "pch.h"
#include "CImageProc.h"
#include <afxdlgs.h>
#include <vector>
#include <algorithm> // 用于排序
#include <omp.h>
#include <numeric>
#include <complex>
#include <valarray>
#include <fftw3.h>
#include <cmath>
#include <random> // 用于生成高斯分布随机数
#include <iostream>
#include <utility>
#include "CSpectrumDlg.h"
#include <queue>
#include <map>
#include <fstream>
#include <unordered_map>
#include "locale.h"

// 为 std::vector<BYTE> 添加哈希函数
namespace std {
    template<>
    struct hash<vector<BYTE>> {
        size_t operator()(const vector<BYTE>& v) const {
            size_t hash = v.size();
            for (auto& i : v) {
                hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CImageProc::CImageProc()  
{  
   m_hDib = NULL;  
   pDib = NULL;  
   pBFH = NULL;  
   pBIH = NULL;  
   pQUAD = NULL;  
   pBits = NULL;  
   nWidth = nHeight = nBitCount = 0;  
   m_bIs565Format = true;  
   isPaletteDarkToLight = false;  
   m_bIFFTPerformed = false;  
   setlocale(LC_CTYPE, "chs"); // 修复未定义标识符错误，替换中文引号为英文引号  
}
CImageProc::~CImageProc()
{
    if (m_hDib) {
        if (pDib) {
            GlobalUnlock(m_hDib);
        }
        GlobalFree(m_hDib);
    }
}

void CImageProc::CleanUp()
{
    if (m_hDib != NULL)
    {
        if (pDib != NULL)
        {
            ::GlobalUnlock(m_hDib);
            pDib = NULL;
        }
        ::GlobalFree(m_hDib);
        m_hDib = NULL;
    }

    // 清理FFT相关数据
    ResetFFTState();

    pBFH = NULL;
    pBIH = NULL;
    pQUAD = NULL;
    pBits = NULL;
    nWidth = 0;
    nHeight = 0;
    nBitCount = 0;
    m_bIs565Format = false;
}

CImageProc& CImageProc::operator=(const CImageProc& other) {
    if (this == &other) return *this;

    // 释放当前资源
    if (m_hDib) {
        GlobalFree(m_hDib);
        m_hDib = NULL;
    }

    // 复制基本参数
    nWidth = other.nWidth;
    nHeight = other.nHeight;
    nBitCount = other.nBitCount;
    m_bIFFTPerformed = other.m_bIFFTPerformed;

    if (other.m_hDib) {
        // 计算源图像数据大小
        DWORD dwSize = ::GlobalSize(other.m_hDib);

        // 分配新内存
        m_hDib = ::GlobalAlloc(GHND, dwSize);
        if (!m_hDib) AfxThrowMemoryException();

        // 锁定内存
        LPBYTE pSrc = (LPBYTE)::GlobalLock(other.m_hDib);
        LPBYTE pDst = (LPBYTE)::GlobalLock(m_hDib);

        if (!pSrc || !pDst) {
            if (pSrc) ::GlobalUnlock(other.m_hDib);
            if (pDst) ::GlobalUnlock(m_hDib);
            AfxThrowMemoryException();
        }

        // 完整拷贝数据
        memcpy(pDst, pSrc, dwSize);

        // 解锁内存
        ::GlobalUnlock(other.m_hDib);
        ::GlobalUnlock(m_hDib);

        // 重新设置指针
        pDib = (LPBYTE)::GlobalLock(m_hDib);
        pBFH = (LPBITMAPFILEHEADER)pDib;
        pBIH = (LPBITMAPINFOHEADER)(pDib + sizeof(BITMAPFILEHEADER));

        // 重新计算像素数据位置
        int nColorTableSize = 0;
        if (nBitCount <= 8) {
            nColorTableSize = (1 << nBitCount) * sizeof(RGBQUAD);
            pQUAD = (LPRGBQUAD)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        }
        else {
            pQUAD = NULL;
        }

        // 计算像素数据起始位置
        if (pBFH) {
            pBits = pDib + pBFH->bfOffBits;
        }
        else {
            pBits = pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + nColorTableSize;
        }
    }
    return *this;
}

//打开文件
void CImageProc::OpenFile()
{
    CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        L"Bmp File(*.bmp)|*.bmp|JPG File(*.jpg)|*.jpg|All Files(*.*)|*.*||", NULL);
    if (fileDlg.DoModal() == IDOK)
    {
        CString stpathname = fileDlg.GetPathName();
        LoadBmp(stpathname);

        // 加载成功后重置显示状态
        if (m_hDib != NULL) {
            ResetFFTState();
        }
    }
}

// 加载图片
void CImageProc::LoadBmp(CString stFileName)
{
    // 重置FFT相关状态
    ResetFFTState();

    CleanUp(); // 清空内存

    CFile file;
    CFileException e;

    if (!file.Open(stFileName, CFile::modeRead | CFile::shareDenyWrite, &e))
    {
#ifdef _DEBUG
        afxDump << "File could not be opened " << e.m_cause << "\n";
#endif
        return;
    }

    ULONGLONG nFileSize = file.GetLength();
    if (nFileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
    {
        file.Close();
        return;
    }

    m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, nFileSize);
    if (!m_hDib)
    {
        file.Close();
        return;
    }

    pDib = (BYTE*)::GlobalLock(m_hDib);
    if (!pDib)
    {
        GlobalFree(m_hDib);
        m_hDib = NULL;
        file.Close();
        return;
    }

    UINT nBytesRead = file.Read(pDib, (UINT)nFileSize);
    file.Close();

    if (nBytesRead != nFileSize)
    {
        CleanUp();
        return;
    }

    pBFH = (BITMAPFILEHEADER*)pDib;
    if (pBFH->bfType != 0x4D42)
    {
        CleanUp();
        return;
    }

    pBIH = (BITMAPINFOHEADER*)&pDib[sizeof(BITMAPFILEHEADER)];
    if (pBIH->biSize < sizeof(BITMAPINFOHEADER))
    {
        CleanUp();
        return;
    }

    pQUAD = (RGBQUAD*)&pDib[sizeof(BITMAPFILEHEADER) + pBIH->biSize];

    if (pBFH->bfOffBits >= nFileSize)
    {
        CleanUp();
        return;
    }
    pBits = &pDib[pBFH->bfOffBits];
    nWidth = pBIH->biWidth;
    nHeight = abs(pBIH->biHeight);
    nBitCount = pBIH->biBitCount;

    DWORD dwImageSize = ((nWidth * nBitCount + 31) / 32) * 4 * nHeight;
    if (pBFH->bfOffBits + dwImageSize > nFileSize)
    {
        CleanUp();
        return;
    }

    if (pBIH->biCompression == BI_RGB && nBitCount == 16)
    {
        m_bIs565Format = false;
    }
    else if (pBIH->biCompression == BI_BITFIELDS && nBitCount == 16)
    {
        if (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD) <= nFileSize)
        {
            DWORD* masks = reinterpret_cast<DWORD*>(&pDib[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)]);
            DWORD redMask = masks[0];
            DWORD greenMask = masks[1];
            DWORD blueMask = masks[2];
            if (redMask == 0xF800 && greenMask == 0x07E0 && blueMask == 0x001F)
            {
                m_bIs565Format = true;
            }
        }
        else
        {
            m_bIs565Format = false;
        }
    }
    else
    {
        m_bIs565Format = false;
    }
}

// 显示图片
void CImageProc::ShowBMP(CDC* pDC, int x, int y, int destWidth, int destHeight)
{
    if (m_hDib != NULL && pBIH != NULL && pBits != NULL)
    {
        ::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);
        ::StretchDIBits(
            pDC->m_hDC,
            x, y, destWidth, destHeight, // 目标区域（显示区域）
            0, 0, pBIH->biWidth, pBIH->biHeight, // 源区域（原图大小）
            pBits,
            (BITMAPINFO*)pBIH,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }
}

BYTE* CImageProc::GetPixelPtr(int x, int y) {
    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
    float bytePerPixel = float(nBitCount) / 8;
    int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
    return pBits + offset;
}

//获取像素颜色
void CImageProc::GetColor(int x, int y, BYTE& red, BYTE& green, BYTE& blue)
{
    if (m_hDib == NULL || x < 0 || x >= nWidth || y < 0 || y >= nHeight)
    {
        return;
    }
    // pixel指向当前像素
    BYTE* pixel = GetPixelPtr(x, y);
    // 获取像素颜色
    switch (nBitCount)
    {
    case 1:
        CImageProc::GetColor1bit(pixel, red, green, blue, x);
        break;
    case 4:
        CImageProc::GetColor4bit(pixel, red, green, blue, x);
        break;
    case 8:
        CImageProc::GetColor8bit(pixel, red, green, blue, x);
        break;
    case 16:
        CImageProc::GetColor16bit(pixel, red, green, blue);
        break;
    case 24:
        CImageProc::GetColor24bit(pixel, red, green, blue);
        break;
    case 32:
        CImageProc::GetColor32bit(pixel, red, green, blue);
        break;
    default:
        AfxMessageBox(_T("不支持的图像位深"));
        return;
    }
}

void CImageProc::SetColor(BYTE* pixel, int x, int y, BYTE r, BYTE g, BYTE b)
{
    switch (nBitCount)
    {
    case 16:
        if (m_bIs565Format)
            *((WORD*)pixel) = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); // 565格式：5位R, 6位G, 5位B
        else
            *((WORD*)pixel) = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3); // 555格式：5位R, 5位G, 5位B
        break;
    case 24:
        pixel[0] = b;
        pixel[1] = g;
        pixel[2] = r;
        break;
    case 32:
        pixel[0] = b;
        pixel[1] = g;
        pixel[2] = r;
        // pixel[3] 保持alpha不变
        break;
    default:
        AfxMessageBox(_T("不支持的图像位深"));
        break;
    }
}

// 获取并显示像素颜色
void CImageProc::DisplayColor(CClientDC* pDC, int imgX, int imgY, int winX, int winY)
{
    if (m_hDib == NULL || imgX < 0 || imgX >= nWidth || imgY < 0 || imgY >= nHeight)
    {
        return;
    }
    // pixel指向当前像素
    BYTE* pixel = GetPixelPtr(imgX, imgY);
    // 获取像素颜色
    BYTE red = 0, green = 0, blue = 0;
    // 根据位深获取像素颜色
    GetColor(imgX, imgY, red, green, blue);

    COLORREF pixelColor = pDC->GetPixel(winX, winY);
    BYTE getPixelRed = GetRValue(pixelColor);
    BYTE getPixelGreen = GetGValue(pixelColor);
    BYTE getPixelBlue = GetBValue(pixelColor);

    pDC->SetBkMode(OPAQUE);//设置背景色为不透明
    pDC->SetTextColor(RGB(0, 0, 0));//设置文本颜色为黑色

    CString str;
    str.Format(L"RGB: (%d, %d, %d)", red, green, blue);

    CString getPixelStr;
    getPixelStr.Format(L"GetPixel RGB: (%d, %d, %d)", getPixelRed, getPixelGreen, getPixelBlue);

    CString location;
    location.Format(L"坐标:(%d, %d)", imgX, imgY);

    pDC->TextOutW(winX, winY, str);//显示像素颜色

    CSize textSize = pDC->GetTextExtent(str);

    pDC->TextOutW(winX, winY + textSize.cy, getPixelStr);//显示GetPixel颜色

    pDC->TextOutW(winX, winY + textSize.cy * 2, location);//显示像素位置
}

void CImageProc::GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x)
{
    BYTE index = (*pixel >> (7 - x % 8)) & 0x01;
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


void CImageProc::GetColor16bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue)
{
    WORD pixelValue = *((WORD*)pixel);
    if (m_bIs565Format) // 处理565格式
    {

        red = (pixelValue & 0xF800) >> 11;    
        green = (pixelValue & 0x07E0) >> 5;   
        blue = pixelValue & 0x001F;           


        red = (red << 3) | (red >> 2);        
        green = (green << 2) | (green >> 4);  
        blue = (blue << 3) | (blue >> 2);    
    } else // 处理555格式
    {
        red = (pixelValue & 0x7C00) >> 10;   
        green = (pixelValue & 0x03E0) >> 5;   
        blue = pixelValue & 0x001F;           

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

// 计算混合直方图
std::vector<int> CImageProc::CalculateHistogramMix()
{
    std::vector<int> histogram(256, 0);//创建一个256大小的向量，用于存储每个灰度值的像素数量

    if (m_hDib == NULL)
    {
        return histogram;//如果位图数据为空，则返回空直方图
    }
    // 遍历每个像素
    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            BYTE* pixel = GetPixelPtr(x, y);
            BYTE red = 0, green = 0, blue = 0;

            GetColor(x, y, red, green, blue);

            int gray = static_cast<int>(0.299 * red + 0.587 * green + 0.114 * blue);//计算灰度值
			histogram[gray]++;//将灰度值对应的像素数量加1
        }
    }

    return histogram;
}

// 计算RGB直方图
std::vector<std::vector<int>> CImageProc::CalculateHistogramRGB()
{
    std::vector<std::vector<int>> histograms(3, std::vector<int>(256, 0));//创建一个3x256的二维向量，用于存储每个通道的直方图

    if (m_hDib == NULL)
    {
        return histograms;
    }
    // 遍历每个像素
    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE red = 0, green = 0, blue = 0;

            GetColor(x, y, red, green, blue);

            //将每个通道的灰度值对应的像素数量加1
            histograms[0][red]++;
            histograms[1][green]++;
            histograms[2][blue]++;
        }
    }

    return histograms;
}


// 应用复古样式
void CImageProc::ApplyVintageStyle()
{
    if (!IsValid()) return;

    if (nBitCount <= 8) {       
        if (!pQUAD) return;
       
        int paletteSize = 1 << nBitCount;

        for (int i = 0; i < paletteSize; i++) {
            BYTE r = pQUAD[i].rgbRed;
            BYTE g = pQUAD[i].rgbGreen;
            BYTE b = pQUAD[i].rgbBlue;

            pQUAD[i].rgbRed = min(255, static_cast<int>(r * 1.1 + 15));
            pQUAD[i].rgbGreen = min(255, static_cast<int>(g * 0.9 + 10));
            pQUAD[i].rgbBlue = min(255, static_cast<int>(b * 0.8));
            pQUAD[i].rgbReserved = 0;
        }

        if (nBitCount == 1) {

            for (int i = 0; i < 2; i++) {
                BYTE gray = static_cast<BYTE>(0.299 * pQUAD[i].rgbRed + 0.587 * pQUAD[i].rgbGreen + 0.114 * pQUAD[i].rgbBlue);
                
                pQUAD[i] = {
                    static_cast<BYTE>(gray * 0.7),
                    static_cast<BYTE>(gray * 0.6),
                    static_cast<BYTE>(gray * 0.4),
                    0
                };
            }
        }
    }
    else{  
        // 统一处理16/24/32位图像
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                BYTE r, g, b;
                GetColor(x, y, r, g, b);
                int newR = min(255, static_cast<int>(r * 1.15 + 15));
                int newG = min(255, static_cast<int>(g * 0.85 + 5));
                int newB = min(255, static_cast<int>(b * 0.7));
                int noise = (rand() % 11) - 5;
                newR = max(0, min(255, newR + noise));
                newG = max(0, min(255, newG + noise));
                newB = max(0, min(255, newB + noise));
                newR = static_cast<int>(newR * 0.95 + 10);
                newG = static_cast<int>(newG * 0.95 + 5);
                newB = static_cast<int>(newB * 0.95);
                // 计算像素指针
                BYTE* pixel = GetPixelPtr(x, y);
                SetColor(pixel, x, y, (BYTE)newR, (BYTE)newG, (BYTE)newB);
            }
        }
    }
}


// 函数定义：直方图均衡化处理，返回均衡化后的RGB三通道直方图数据
// 输入参数：dc为设备上下文对象，用于像素操作
std::vector<std::vector<int>> CImageProc::Balance_Transformations()
{
    //创建一个 ​​3 × 256 的二维数组(一个嵌套的二维动态数组)​​：(外层有 ​​3 个元素​​（对应 RGB 三个颜色通道）,外层 vector 中每个元素的初始化模板。)
    // 每个外层元素是一个长度为 ​​256​​ 的 vector<int>（对应 0~255 的灰度级）
    std::vector<std::vector<int>> balancedRgbHistograms(3, std::vector<int>(256, 0)); //存储均衡化后的RGB三通道直方图数据

    // 定义概率密度数组、累积分布数组、映射函数数组
    float p[256] = { 0 };      
    float S[256] = { 0 };       
    int F[256] = { 0 };        

    // 调用成员函数获取混合直方图数据
    std::vector<int> intensity_paint = CalculateHistogramMix();

    // 获取图像宽高
    int w = nWidth;
    int h = nHeight;

   //w * h 计算图像的总像素数，将结果转换为float类型，避免计算概率密度时整数除法截断误差
    float pixel_count = static_cast<float>(w * h); 

    // 计算每个灰度级的概率密度 p[i] = 灰度i的像素数/总像素数
    for (int i = 0; i < 256; i++) {
        p[i] = intensity_paint[i] / pixel_count;
    }

    // 计算累积分布函数S[n]（CDF）
    S[0] = 255.0f * p[0];//初始化累积分布的起点
    for (int n = 1; n < 256; n++) {
        S[n] = 255.0f * p[n] + S[n - 1];//递推计算累积分布
    }

    // 灰度映射函数F[j]：将CDF四舍五入并约束到0-255
    for (int j = 0; j < 256; j++) {
        F[j] = static_cast<int>(S[j] + 0.5f);  // 浮点转整型，四舍五入(：C++浮点转整数会直接截断小数部分)
        F[j] = max(0, min(255, F[j]));         // 防止越界
    }
    // 遍历图像每个像素,进行均衡化处理
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // 获取像素指针（pBits为类内图像数据指针）
            BYTE* pixel = GetPixelPtr(x, y);
            // 定义RGB分量变量
            BYTE red, green, blue;
            // 根据位深度调用不同的颜色解析方法
            GetColor(x, y, red, green, blue);

            // 根据人眼对颜色的敏感度（绿 > 红 > 蓝），使用加权系数计算亮度（ITU-R BT.601标准加权：Y=0.299R+0.587G+0.114B）
            int Y = static_cast<int>(0.3f * red + 0.59f * green + 0.11f * blue + 0.5f);
            Y = max(0, min(255, Y)); // 约束到0-255范围

            // 特殊处理纯黑像素,直接跳过缩放计算，​​保持像素为黑色
            if (Y == 0) {
                // 直接写入黑色
                if (nBitCount <= 8)
                {
                    pixel[0] = 0;
                }
                else
                {
                    SetColor(pixel, x, y, 0, 0, 0);
                }
                balancedRgbHistograms[0][0]++;      // 记录R通道直方图
                balancedRgbHistograms[1][0]++;      // 记录G通道直方图
                balancedRgbHistograms[2][0]++;      // 记录B通道直方图
                continue;
            }

            // 计算缩放系数：F[Y]/Y（基于均衡化后的亮度与原亮度的比例）
            float scale = F[Y] / static_cast<float>(Y);

            // 计算新的RGB值（四舍五入并约束范围）
            int new_r = static_cast<int>(red * scale + 0.5f);
            int new_g = static_cast<int>(green * scale + 0.5f);
            int new_b = static_cast<int>(blue * scale + 0.5f);
            new_r = max(0, min(255, new_r));
            new_g = max(0, min(255, new_g));
            new_b = max(0, min(255, new_b));

            // 更新均衡化后的直方图数据
            balancedRgbHistograms[0][new_r]++;
            balancedRgbHistograms[1][new_g]++;
            balancedRgbHistograms[2][new_b]++;

            // 写入均衡化后的像素值
            if (nBitCount <= 8)
            {
                pixel[0] = static_cast<BYTE>(F[Y]);
            }
            else
            {
                SetColor(pixel, x, y, new_r, new_g, new_b);
            }
        }
    }

    // 返回均衡化后的三通道直方图数据
    return balancedRgbHistograms;
}

// 转换为黑白风格
void CImageProc::ApplyBlackAndWhiteStyle()
{
    if (m_hDib == NULL)
    {
        AfxMessageBox(_T("No valid image is loaded."));
        return;
    }
    if (nBitCount <= 8) {
        if (!pQUAD) return;

        int paletteSize = 1 << nBitCount;

        for (int i = 0; i < paletteSize; i++) {
            BYTE r = pQUAD[i].rgbRed;
            BYTE g = pQUAD[i].rgbGreen;
            BYTE b = pQUAD[i].rgbBlue;

            BYTE gray = 0.299 * r + 0.587 * g + 0.114 * b;

            pQUAD[i].rgbRed = gray;
            pQUAD[i].rgbGreen = gray;
            pQUAD[i].rgbBlue = gray;
            pQUAD[i].rgbReserved = 0;
        }
    }
    else
    {
        // 遍历图像的每个像素点
        for (int y = 0; y < nHeight; ++y)
        {
            for (int x = 0; x < nWidth; ++x)
            {
                BYTE* pixel = GetPixelPtr(x, y);
                BYTE red = 0, green = 0, blue = 0;
                GetColor(x, y, red, green, blue);
                // 计算灰度值
                BYTE grayValue = static_cast<BYTE>((red * 0.299) + (green * 0.587) + (blue * 0.114));
                // 根据灰度值设置像素
                SetColor(pixel, x, y, grayValue, grayValue, grayValue);
            }
        }
    }
}

//直方图规格化
void CImageProc::HistogramMatching(CImageProc& targetImageProc)
{
    if (!IsValid() || !targetImageProc.IsValid())
    {
        AfxMessageBox(_T("图像数据无效"));
        return;
    }

    int width = nWidth;
    int height = nHeight;
    int rowSize = ((width * nBitCount + 31) / 32) * 4;
    int targetWidth = targetImageProc.nWidth;
    int targetHeight = targetImageProc.nHeight;
    int targetRowSize = ((targetWidth * targetImageProc.nBitCount + 31) / 32) * 4;

    CWaitCursor waitCursor;

    int numSourceChannels = (nBitCount == 1) ? 1 : (nBitCount / 8);
    int numTargetChannels = (targetImageProc.nBitCount == 1) ? 1 : (targetImageProc.nBitCount / 8);

    for (int channel = 0; channel < numSourceChannels; channel++)
    {
        std::vector<int> sourceHist(256, 0);
        std::vector<int> targetHist(256, 0);

        // 计算源图像直方图（提取RGB值）
        for (int y = 0; y < height; y++)
        {
            BYTE* pSource = pBits + (height - 1 - y) * rowSize;
            if (!pSource) continue;

            for (int x = 0; x < width; x++)
            {
                BYTE red, green, blue;
                GetColor(x, y, red, green, blue);

                BYTE val = (channel == 0) ? red : (channel == 1 ? green : blue);
                sourceHist[val]++;
            }
        }

        // 计算目标图像直方图（提取RGB值）
        for (int y = 0; y < targetHeight; y++)
        {
            BYTE* pTarget = targetImageProc.pBits + (targetHeight - 1 - y) * targetRowSize;
            if (!pTarget) continue;

            for (int x = 0; x < targetWidth; x++)
            {
                BYTE red, green, blue;
                switch (targetImageProc.nBitCount)
                {
                case 1:
                    GetColor1bit(&pTarget[x / 8], red, green, blue, x % 8);
                    break;
                case 4:
                    GetColor4bit(&pTarget[x / 2], red, green, blue, x % 2);
                    break;
                case 8:
                    GetColor8bit(&pTarget[x], red, green, blue, x);
                    break;
                case 16:
                    GetColor16bit(&pTarget[x * 2], red, green, blue);
                    break;
                case 24:
                    GetColor24bit(&pTarget[x * 3], red, green, blue);
                    break;
                case 32:
                    GetColor32bit(&pTarget[x * 4], red, green, blue);
                    break;
                }

                BYTE val = (channel % numTargetChannels == 0) ? red :
                    (channel % numTargetChannels == 1 ? green : blue);
                targetHist[val]++;
            }
        }

        // 计算CDF
        std::vector<double> sourceCDF(256, 0);
        std::vector<double> targetCDF(256, 0);

        sourceCDF[0] = sourceHist[0];
        targetCDF[0] = targetHist[0];

        for (int i = 1; i < 256; i++)
        {
            sourceCDF[i] = sourceCDF[i - 1] + sourceHist[i];
            targetCDF[i] = targetCDF[i - 1] + targetHist[i];
        }

        // 归一化CDF
        double sourceTotal = width * height;
        double targetTotal = targetWidth * targetHeight;
        for (int i = 0; i < 256; i++)
        {
            sourceCDF[i] /= sourceTotal;
            targetCDF[i] /= targetTotal;
        }

        // 创建映射表
        std::vector<BYTE> mapping(256, 0);
        for (int i = 0; i < 256; i++)
        {
            double cdf = sourceCDF[i];
            int left = 0, right = 255;
            int result = 0;
            while (left <= right) //j 是目标 CDF 中第一个小于等于源 CDF [i] 的灰度级
            {
                int mid = left + (right - left) / 2;
                if (targetCDF[mid] <= cdf)
                {
                    result = mid;
                    left = mid + 1;
                }
                else
                {
                    right = mid - 1;
                }
            }
            mapping[i] = static_cast<BYTE>(result);
        }

        // 应用映射（区分调色板图像和真彩色图像）
        for (int y = 0; y < height; y++)
        {
            BYTE* pSource = pBits + (height - 1 - y) * rowSize;
            if (!pSource) continue;

            for (int x = 0; x < width; x++)
            {
                BYTE red, green, blue;
                switch (nBitCount)
                {
                case 1:
                    GetColor1bit(&pSource[x / 8], red, green, blue, x % 8);
                    break;
                case 4:
                    GetColor4bit(&pSource[x / 2], red, green, blue, x % 2);
                    break;
                case 8:
                    GetColor8bit(&pSource[x], red, green, blue, x);
                    break;
                case 16:
                    GetColor16bit(&pSource[x * 2], red, green, blue);
                    break;
                case 24:
                    GetColor24bit(&pSource[x * 3], red, green, blue);
                    break;
                case 32:
                    GetColor32bit(&pSource[x * 4], red, green, blue);
                    break;
                }

                BYTE newRed = red, newGreen = green, newBlue = blue;
                switch (channel)
                {
                case 0: newRed = mapping[red]; break;
                case 1: newGreen = mapping[green]; break;
                case 2: newBlue = mapping[blue]; break;
                }

                // 处理调色板索引图像（1位/4位/8位）
                if (nBitCount <= 8)
                {
                    if (TRUE)
                    {
                        int paletteSize = (nBitCount == 1) ? 2 : ((nBitCount == 4) ? 16 : 256);
                        RGBQUAD* pPal = pQUAD; // 调色板指针

                        // 遍历调色板所有索引
                        for (int idx = 0; idx < paletteSize; idx++)
                        {
                            BYTE& r = pPal[idx].rgbRed;
                            BYTE& g = pPal[idx].rgbGreen;
                            BYTE& b = pPal[idx].rgbBlue;
                            // 对每个颜色通道应用映射
                            r = mapping[r];
                            g = mapping[g];
                            b = mapping[b];
                        }
                    }
                }
                else // 真彩色图像直接修改像素值
                {
                    int index = x * (nBitCount / 8);
                    SetColor(&pSource[index], x, y, newRed, newGreen, newBlue);
                }
            }
        }
    }
    return;
}

// 添加椒盐噪声
void CImageProc::AddSaltAndPepperNoise(double noiseRatio, double saltRatio)
{
    if (!IsValid()) {
        AfxMessageBox(_T("No valid image is loaded."));
        return;
    }
    noiseRatio = max(0.0, min(1.0, noiseRatio));
    int totalPixels = nWidth * nHeight;
    int noisePixels = static_cast<int>(totalPixels * noiseRatio);
    srand(static_cast<unsigned int>(time(nullptr)));

    for (int i = 0; i < noisePixels; ++i) {
        int x = rand() % nWidth;
        int y = rand() % nHeight;
        BYTE* pixel = GetPixelPtr(x, y);
        bool isSalt = (rand() % 100) < (saltRatio * 100);

        switch (nBitCount) {
        case 1: {
            int bitPos = 7 - (x % 8);
            if (isSalt)
                *pixel |= (1 << bitPos);
            else
                *pixel &= ~(1 << bitPos);
            break;
        }
        case 4: {
            BYTE newValue = isSalt ? 0x0F : 0x00;
            if (x % 2 == 0)
                *pixel = (*pixel & 0x0F) | (newValue << 4);
            else
                *pixel = (*pixel & 0xF0) | newValue;
            break;
        }
        case 8:
            //这里取决于图像调色板的顺序，与设置的盐噪声比例不同
            *pixel = isSalt ? 255 : 0;
            break;
        case 16: {
            WORD newValue = 0;
            if (isSalt)
                newValue = m_bIs565Format ? 0xFFFF : 0x7FFF;
            *reinterpret_cast<WORD*>(pixel) = newValue;
            break;
        }
        case 24:
            memset(pixel, isSalt ? 255 : 0, 3);
            break;
        case 32:
            pixel[0] = pixel[1] = pixel[2] = isSalt ? 255 : 0;
            // pixel[3] alpha保持不变
            break;
        default:
            AfxMessageBox(_T("Unsupported bit count."));
            return;
        }
    }
}

//添加脉冲噪声
void CImageProc::AddImpulseNoise(double noiseRatio, BYTE noiseValue1, BYTE noiseValue2)
{
    if (!IsValid())
    {
        AfxMessageBox(_T("No valid image is loaded."));
        return;
    }

    // 确保噪声比例在合理范围内(0-1)
    noiseRatio = max(0.0, min(1.0, noiseRatio));

    // 计算要添加噪声的像素数量
    int totalPixels = nWidth * nHeight;
    int noisePixels = static_cast<int>(totalPixels * noiseRatio);

    // 获取随机数种子
    srand(static_cast<unsigned int>(time(nullptr)));

    for (int i = 0; i < noisePixels; ++i)
    {
        // 随机选择像素位置
        int x = rand() % nWidth;
        int y = rand() % nHeight;

        // 计算像素偏移量
        BYTE* pixel = GetPixelPtr(x, y);

        // 随机决定使用哪个噪声值
        BYTE noiseValue = (rand() % 2) ? noiseValue1 : noiseValue2;

        // 根据位深度处理噪声
        switch (nBitCount)
        {
        case 1: // 1位图像
        {
            // 将噪声值转换为1位(0或1)
            BYTE bitValue = (noiseValue > 127) ? 1 : 0;

            // 获取当前像素值
            BYTE currentByte = *pixel;
            int bitPos = 7 - (x % 8);
            int currentBit = (currentByte >> bitPos) & 0x01;

            // 设置新值
            if (currentBit != bitValue)
            {
                *pixel ^= (1 << bitPos);
            }
        }
        break;

        case 4: // 4位图像
        {
            BYTE currentByte = *pixel;
            bool isHighNibble = (x % 2) == 0;
            BYTE newValue = noiseValue >> 4; // 将8位值转换为4位

            if (isHighNibble)
            {
                *pixel = (newValue << 4) | (currentByte & 0x0F);
            }
            else
            {
                *pixel = (currentByte & 0xF0) | newValue;
            }
        }
        break;

        case 8: // 8位灰度图像
            *pixel = noiseValue;
            break;

        case 16: // 16位图像
        {
            WORD newValue;
            if (m_bIs565Format)
            {
                // RGB565格式
                BYTE r = (noiseValue >> 3) & 0x1F;  // 5-bit red
                BYTE g = (noiseValue >> 2) & 0x3F;  // 6-bit green
                BYTE b = (noiseValue >> 3) & 0x1F;  // 5-bit blue
                newValue = (r << 11) | (g << 5) | b;
            }
            else
            {
                // RGB555格式
                BYTE r = (noiseValue >> 3) & 0x1F;  // 5-bit red
                BYTE g = (noiseValue >> 3) & 0x1F;  // 5-bit green
                BYTE b = (noiseValue >> 3) & 0x1F;  // 5-bit blue
                newValue = (r << 10) | (g << 5) | b;
            }
            *reinterpret_cast<WORD*>(pixel) = newValue;
        }
        break;

        case 24: // 24位真彩色
            pixel[0] = noiseValue; // B
            pixel[1] = noiseValue; // G
            pixel[2] = noiseValue; // R
            break;

        case 32: // 32位图像
            pixel[0] = noiseValue; // B
            pixel[1] = noiseValue; // G
            pixel[2] = noiseValue; // R
            // pixel[3]保持不变(Alpha)
            break;

        default:
            // 不支持的位深度
            AfxMessageBox(_T("Unsupported bit count."));
            return;
        }
    }
}

// 添加高斯噪声
void CImageProc::AddGaussianNoise(double mean, double sigma)
{
    if (!IsValid())
    {
        AfxMessageBox(_T("No valid image is loaded."));
        return;
    }

    // 设置随机数生成器
    std::random_device rd; //随机数生成设备
    std::mt19937 gen(rd()); //伪随机数生成器，基于梅森旋转算法
    std::normal_distribution<> dist(mean, sigma); //用于生成符合正态分布的随机数

    // 遍历所有像素
    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            // 计算像素偏移量
            BYTE* pixel = GetPixelPtr(x, y);

            // 获取当前像素颜色
            BYTE red = 0, green = 0, blue = 0;
            GetColor(x, y, red, green, blue);

            // 为每个颜色通道添加高斯噪声
            int newRed = red + static_cast<int>(dist(gen));
            int newGreen = green + static_cast<int>(dist(gen));
            int newBlue = blue + static_cast<int>(dist(gen));

            // 确保值在0-255范围内
            newRed = max(0, min(255, newRed));
            newGreen = max(0, min(255, newGreen));
            newBlue = max(0, min(255, newBlue));

            // 设置新像素值
            switch (nBitCount)
            {
            case 1: // 1位图像转换为灰度后处理
            {
                // 计算灰度值
                BYTE gray = static_cast<BYTE>(0.299 * newRed + 0.587 * newGreen + 0.114 * newBlue);
                // 转换为1位
                BYTE bitValue = (gray > 127) ? 1 : 0;
                // 获取当前像素值
                BYTE currentByte = *pixel;
                int bitPos = 7 - (x % 8);
                int currentBit = (currentByte >> bitPos) & 0x01;
                // 设置新值
                if (currentBit != bitValue)
                {
                    *pixel ^= (1 << bitPos);
                }
            }
            break;

            case 4: // 4位图像
            {
                BYTE gray = static_cast<BYTE>(0.299 * newRed + 0.587 * newGreen + 0.114 * newBlue);
                BYTE quantized = gray >> 4; // 转换为4位
                BYTE currentByte = *pixel;
                bool isHighNibble = (x % 2) == 0;

                if (isHighNibble)
                {
                    *pixel = (quantized << 4) | (currentByte & 0x0F);
                }
                else
                {
                    *pixel = (currentByte & 0xF0) | quantized;
                }
            }
            break;

            case 8: // 8位灰度图像
            {
                BYTE gray = static_cast<BYTE>(0.299 * newRed + 0.587 * newGreen + 0.114 * newBlue);
                *pixel = gray;
            }
            break;

            case 16: // 16位图像
            case 24: // 24位真彩色
            case 32: // 32位图像
                SetColor(pixel, x, y, newRed, newGreen, newBlue);
                break;
            }
        }
    }
}


// 添加高斯白噪声
void CImageProc::AddGaussianWhiteNoise(double sigma)
{
    if (!IsValid())
    {
        AfxMessageBox(_T("No valid image is loaded."));
        return;
    }

    // 设置随机数生成器(零均值高斯分布)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(0.0, sigma);

    // 遍历所有像素
    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            // 计算像素偏移量
            BYTE* pixel = GetPixelPtr(x, y);

            // 获取当前像素颜色
            BYTE red = 0, green = 0, blue = 0;
            switch (nBitCount)
            {
            case 1:
                GetColor1bit(pixel, red, green, blue, x);
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

            // 为每个颜色通道添加独立的高斯白噪声
            int newRed = red + static_cast<int>(dist(gen));
            int newGreen = green + static_cast<int>(dist(gen));
            int newBlue = blue + static_cast<int>(dist(gen));

            // 确保值在0-255范围内
            newRed = max(0, min(255, newRed));
            newGreen = max(0, min(255, newGreen));
            newBlue = max(0, min(255, newBlue));

            // 设置新像素值
            switch (nBitCount)
            {
            case 1: // 1位图像转换为灰度后处理
            {
                // 计算灰度值
                BYTE gray = static_cast<BYTE>(0.299 * newRed + 0.587 * newGreen + 0.114 * newBlue);
                // 转换为1位
                BYTE bitValue = (gray > 127) ? 1 : 0;
                // 获取当前像素值
                BYTE currentByte = *pixel;
                int bitPos = 7 - (x % 8);
                int currentBit = (currentByte >> bitPos) & 0x01;
                // 设置新值
                if (currentBit != bitValue)
                {
                    *pixel ^= (1 << bitPos);
                }
            }
            break;

            case 4: // 4位图像
            {
                BYTE gray = static_cast<BYTE>(0.299 * newRed + 0.587 * newGreen + 0.114 * newBlue);
                BYTE quantized = gray >> 4; // 转换为4位
                BYTE currentByte = *pixel;
                bool isHighNibble = (x % 2) == 0;

                if (isHighNibble)
                {
                    *pixel = (quantized << 4) | (currentByte & 0x0F);
                }
                else
                {
                    *pixel = (currentByte & 0xF0) | quantized;
                }
            }
            break;

            case 8: // 8位灰度图像
            {
                BYTE gray = static_cast<BYTE>(0.299 * newRed + 0.587 * newGreen + 0.114 * newBlue);
                *pixel = gray;
            }
            break;

            case 16: // 16位图像
            {
                WORD newPixel;
                if (m_bIs565Format)
                {
                    // RGB565格式
                    BYTE r = (newRed >> 3) & 0x1F;  // 5-bit red
                    BYTE g = (newGreen >> 2) & 0x3F; // 6-bit green
                    BYTE b = (newBlue >> 3) & 0x1F;  // 5-bit blue
                    newPixel = (r << 11) | (g << 5) | b;
                }
                else
                {
                    // RGB555格式
                    BYTE r = (newRed >> 3) & 0x1F;  // 5-bit red
                    BYTE g = (newGreen >> 3) & 0x1F; // 5-bit green
                    BYTE b = (newBlue >> 3) & 0x1F;  // 5-bit blue
                    newPixel = (r << 10) | (g << 5) | b;
                }
                *reinterpret_cast<WORD*>(pixel) = newPixel;
            }
            break;

            case 24: // 24位真彩色
                pixel[0] = static_cast<BYTE>(newBlue);
                pixel[1] = static_cast<BYTE>(newGreen);
                pixel[2] = static_cast<BYTE>(newRed);
                break;

            case 32: // 32位图像
                pixel[0] = static_cast<BYTE>(newBlue);
                pixel[1] = static_cast<BYTE>(newGreen);
                pixel[2] = static_cast<BYTE>(newRed);
                // pixel[3]保持不变(Alpha)
                break;
            }
        }
    }
}

void CImageProc::SpatialFilter(int filterSize, FilterType type)
{
    if (nBitCount != 8 && nBitCount != 16 && nBitCount != 24 && nBitCount != 32) {
        AfxMessageBox(_T("不支持的图像格式"));
        return;
    }
    int channels = 1;
    if (nBitCount == 16) channels = 3; // 16位转为RGB处理
    if (nBitCount == 24) channels = 3;
    if (nBitCount == 32) channels = 4;
    int radius = filterSize / 2;
    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
    BYTE* tempBuffer = new BYTE[nHeight * rowSize];
    memcpy(tempBuffer, pBits, nHeight * rowSize);

    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            // 8位灰度图特殊处理
            if (nBitCount == 8) {
                std::vector<BYTE> window;
                int sum = 0;
                BYTE maxVal = 0;
                int count = 0;
                for (int dy = -radius; dy <= radius; ++dy) {
                    int ny = y + dy;
                    if (ny < 0 || ny >= nHeight) continue;
                    for (int dx = -radius; dx <= radius; ++dx) {
                        int nx = x + dx;
                        if (nx < 0 || nx >= nWidth) continue;
                        BYTE* neighbor = tempBuffer + ny * rowSize + nx;
                        BYTE val = neighbor[0];
                        if (type == FilterType::Mean) { sum += val; count++; }
                        if (type == FilterType::Median) window.push_back(val);
                        if (type == FilterType::Max) if (val > maxVal) maxVal = val;
                    }
                }
                BYTE* pixel = pBits + y * rowSize + x;
                if (type == FilterType::Mean) {
                    pixel[0] = static_cast<BYTE>(sum / count);
                }
                else if (type == FilterType::Median) {
                    std::nth_element(window.begin(), window.begin() + window.size() / 2, window.end());
                    pixel[0] = window[window.size() / 2];
                }
                else if (type == FilterType::Max) {
                    pixel[0] = maxVal;
                }
            }
            // 16位图像，转为RGB处理
            else if (nBitCount == 16) {
                std::vector<BYTE> winR, winG, winB;
                int sumR = 0, sumG = 0, sumB = 0, count = 0;
                BYTE maxR = 0, maxG = 0, maxB = 0;
                for (int dy = -radius; dy <= radius; ++dy) {
                    int ny = y + dy;
                    if (ny < 0 || ny >= nHeight) continue;
                    for (int dx = -radius; dx <= radius; ++dx) {
                        int nx = x + dx;
                        if (nx < 0 || nx >= nWidth) continue;
                        BYTE* neighbor = tempBuffer + ny * rowSize + nx * 2;
                        BYTE r, g, b;
                        GetColor16bit(neighbor, r, g, b);
                        if (type == FilterType::Mean) { sumR += r; sumG += g; sumB += b; count++; }
                        if (type == FilterType::Median) { winR.push_back(r); winG.push_back(g); winB.push_back(b); }
                        if (type == FilterType::Max) {
                            if (r > maxR) maxR = r;
                            if (g > maxG) maxG = g;
                            if (b > maxB) maxB = b;
                        }
                    }
                }
                BYTE* pixel = pBits + y * rowSize + x * 2;
                BYTE r, g, b;
                if (type == FilterType::Mean) {
                    r = static_cast<BYTE>(sumR / count);
                    g = static_cast<BYTE>(sumG / count);
                    b = static_cast<BYTE>(sumB / count);
                }
                else if (type == FilterType::Median) {
                    std::nth_element(winR.begin(), winR.begin() + winR.size() / 2, winR.end());
                    std::nth_element(winG.begin(), winG.begin() + winG.size() / 2, winG.end());
                    std::nth_element(winB.begin(), winB.begin() + winB.size() / 2, winB.end());
                    r = winR[winR.size() / 2];
                    g = winG[winG.size() / 2];
                    b = winB[winB.size() / 2];
                }
                else if (type == FilterType::Max) {
                    r = maxR; g = maxG; b = maxB;
                }
                SetColor(pixel, x, y, r, g, b);
            }
            // 24/32位图像
            else {
                for (int c = 0; c < channels; ++c) {
                    std::vector<BYTE> window;
                    int sum = 0, count = 0;
                    BYTE maxVal = 0;
                    for (int dy = -radius; dy <= radius; ++dy) {
                        int ny = y + dy;
                        if (ny < 0 || ny >= nHeight) continue;
                        for (int dx = -radius; dx <= radius; ++dx) {
                            int nx = x + dx;
                            if (nx < 0 || nx >= nWidth) continue;
                            BYTE* neighbor = tempBuffer + ny * rowSize + nx * channels;
                            BYTE val = neighbor[c];
                            if (type == FilterType::Mean) { sum += val; count++; }
                            if (type == FilterType::Median) window.push_back(val);
                            if (type == FilterType::Max) if (val > maxVal) maxVal = val;
                        }
                    }
                    BYTE* pixel = pBits + y * rowSize + x * channels;
                    if (type == FilterType::Mean) {
                        pixel[c] = static_cast<BYTE>(sum / count);
                    }
                    else if (type == FilterType::Median) {
                        std::nth_element(window.begin(), window.begin() + window.size() / 2, window.end());
                        pixel[c] = window[window.size() / 2];
                    }
                    else if (type == FilterType::Max) {
                        pixel[c] = maxVal;
                    }
                }
            }
        }
    }
    delete[] tempBuffer;
}

// 定义递归函数
void TraceWeakEdge(int x, int y, int nWidth, int nHeight, std::vector<BYTE>& edgeImage, std::vector<bool>& visited) {
    visited[y * nWidth + x] = true;
    const int dx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    const int dy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    for (int k = 0; k < 8; ++k) {
        int nx = x + dx[k], ny = y + dy[k];
        if (nx >= 0 && nx < nWidth && ny >= 0 && ny < nHeight) {
            int idx = ny * nWidth + nx;
            if (!visited[idx] && edgeImage[idx] == 128) {
                edgeImage[idx] = 255;
                TraceWeakEdge(nx, ny, nWidth, nHeight, edgeImage, visited);
            }
        }
    }
}

//Canny边缘检测
void CImageProc::ApplyCannyEdgeDetection()
{
    int kernelSize = 3;
    if (!IsValid() || nBitCount < 8)
    {
        AfxMessageBox(_T("不支持此图像格式"));
        return;
    }
    // 创建临时缓冲区存储灰度图像
    std::vector<BYTE> grayImage(nWidth * nHeight);
    // 转换为灰度图像
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);
            BYTE red = 0, green = 0, blue = 0;
            GetColor(x, y, red, green, blue);
            grayImage[y * nWidth + x] = static_cast<BYTE>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    }
    // 高斯模糊
    std::vector<BYTE> blurredImage(nWidth * nHeight);
    const int gaussianKernel[5][5] = {
            { 2, 4, 5, 4, 2 },
            { 4, 9, 12, 9, 4 },
            { 5, 12, 15, 12, 5 },
            { 4, 9, 12, 9, 4 },
            { 2, 4, 5, 4, 2 }
    };
    const int gaussianKernelSum = 159;
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    sum += grayImage[(y + ky) * nWidth + (x + kx)] * gaussianKernel[ky + 1][kx + 1];
                }
            }
            blurredImage[y * nWidth + x] = static_cast<BYTE>(sum / gaussianKernelSum);
        }
    }

    // 利用Sobel算子计算梯度和方向
    const int sobelX[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    const int sobelY[3][3] = { {1, 2, 1}, {0, 0, 0}, {-1, -2, -1} };
    std::vector<double> gradientMagnitude(nWidth * nHeight, 0);
    std::vector<double> gradientDirection(nWidth * nHeight, 0);
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            double gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    BYTE val = blurredImage[(y + ky) * nWidth + (x + kx)];
                    gx += val * sobelX[ky + 1][kx + 1];
                    gy += val * sobelY[ky + 1][kx + 1];
                }
            }
            gradientMagnitude[y * nWidth + x] = sqrt(gx * gx + gy * gy);
            gradientDirection[y * nWidth + x] = atan2(gy, gx) * 180.0 / 3.14159265358979323846;
        }
    }
    // 计算梯度幅值直方图
    std::vector<int> gradHist(256, 0);
    for (int i = 0; i < nWidth * nHeight; ++i) {
        int mag = static_cast<int>(gradientMagnitude[i] + 0.5); // 四舍五入
        mag = max(0, min(255, mag)); // 先min再max，保证类型一致
        gradHist[mag]++;
    }

    // Otsu法自动确定highThreshold
    int total = nWidth * nHeight;
    double sum = 0, sumB = 0, wB = 0, wF = 0, varMax = 0;
    int threshold = 0;
    for (int t = 0; t < 256; ++t) sum += t * gradHist[t];
    for (int t = 0; t < 256; ++t) {
        wB += gradHist[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;
        sumB += t * gradHist[t];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double varBetween = wB * wF * (mB - mF) * (mB - mF);
        if (varBetween > varMax) {
            varMax = varBetween;
            threshold = t;
        }
    }
    double highThreshold = threshold;
    double lowThreshold = highThreshold * 0.5;

    // 非极大值抑制
    std::vector<BYTE> nonMaxSuppressed(nWidth * nHeight, 0);
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            double direction = gradientDirection[y * nWidth + x];
            double magnitude = gradientMagnitude[y * nWidth + x];

            // 将角度归一化到0-180度
            if (direction < 0) direction += 180.0;

            // 计算插值权重和邻居位置
            double weight;
            int x1, y1, x2, y2;

            // 根据角度确定要比较的像素
            if ((direction >= 0 && direction < 22.5) || (direction >= 157.5 && direction <= 180)) {
                // 0度方向（水平）
                x1 = x + 1; y1 = y;
                x2 = x - 1; y2 = y;
            }
            else if (direction >= 22.5 && direction < 67.5) {
                // 45度方向
                x1 = x + 1; y1 = y - 1;
                x2 = x - 1; y2 = y + 1;
            }
            else if (direction >= 67.5 && direction < 112.5) {
                // 90度方向（垂直）
                x1 = x; y1 = y - 1;
                x2 = x; y2 = y + 1;
            }
            else {
                // 135度方向
                x1 = x - 1; y1 = y - 1;
                x2 = x + 1; y2 = y + 1;
            }

            // 确保邻居在图像范围内
            if (x1 >= 0 && x1 < nWidth && y1 >= 0 && y1 < nHeight &&
                x2 >= 0 && x2 < nWidth && y2 >= 0 && y2 < nHeight) {

                double mag1 = gradientMagnitude[y1 * nWidth + x1];
                double mag2 = gradientMagnitude[y2 * nWidth + x2];

                // 如果当前像素是局部最大值，则保留
                if (magnitude >= mag1 && magnitude >= mag2) {
                    nonMaxSuppressed[y * nWidth + x] = static_cast<BYTE>(min(255, magnitude));
                }
            }
        }
    }

    // 双阈值滞后处理
    std::vector<BYTE> edgeImage(nWidth * nHeight, 0);
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            int magnitude = nonMaxSuppressed[y * nWidth + x];
            if (magnitude >= highThreshold) {
                edgeImage[y * nWidth + x] = 255; // 强边缘
            }
            else if (magnitude >= lowThreshold) {
                edgeImage[y * nWidth + x] = 128; // 弱边缘
            }
            else {
                edgeImage[y * nWidth + x] = 0; // 非边缘
            }
        }
    }

    // 递归方式连接弱边缘
    std::vector<bool> visited(nWidth * nHeight, false);
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            if (edgeImage[y * nWidth + x] == 255 && !visited[y * nWidth + x]) {
                TraceWeakEdge(x, y, nWidth, nHeight, edgeImage, visited);
            }
        }
    }
    // 剩余未连接的弱边缘全部置0
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            if (edgeImage[y * nWidth + x] == 128)
                edgeImage[y * nWidth + x] = 0;
        }
    }


    //更新图像数据
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);
            BYTE value = edgeImage[y * nWidth + x];
            switch (nBitCount) {
            case 8:
                *pixel = value;
                break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    newPixel = (value << 11) | (value << 5) | value;
                }
                else {
                    newPixel = (value << 10) | (value << 5) | value;
                }
                *reinterpret_cast<WORD*>(pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = pixel[1] = pixel[2] = value;
                break;
            }
        }
    }
}

//LoG算子边缘检测

void CImageProc::ApplyLoGEdgeDetection()
{
    if (!IsValid() || nBitCount < 8)
    {
        AfxMessageBox(_T("不支持此图像格式"));
        return;
    }

    // 1. 转灰度
    std::vector<BYTE> grayImage(nWidth * nHeight);
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);
            BYTE red = 0, green = 0, blue = 0;
            switch (nBitCount) {
            case 8: GetColor8bit(pixel, red, green, blue, x); break;
            case 16: GetColor16bit(pixel, red, green, blue); break;
            case 24: GetColor24bit(pixel, red, green, blue); break;
            case 32: GetColor32bit(pixel, red, green, blue); break;
            }
            grayImage[y * nWidth + x] = static_cast<BYTE>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    }

    // 2. 高斯平滑（7*7核，sigma=1.4）
    const int gaussianKernel[7][7] = {
        {0, 0, 1, 2, 1, 0, 0},
        {0, 3, 13, 22, 13, 3, 0},
        {1, 13, 59, 97, 59, 13, 1},
        {2, 22, 97, 159, 97, 22, 2},
        {1, 13, 59, 97, 59, 13, 1},
        {0, 3, 13, 22, 13, 3, 0},
        {0, 0, 1, 2, 1, 0, 0}
    };
    const int gaussianSum = 1003;
    std::vector<BYTE> smoothImage(nWidth * nHeight, 0);
    for (int y = 3; y < nHeight - 3; ++y) {
        for (int x = 3; x < nWidth - 3; ++x) {
            int sum = 0;
            for (int ky = -3; ky <= 3; ++ky) {
                for (int kx = -3; kx <= 3; ++kx) {
                    sum += grayImage[(y + ky) * nWidth + (x + kx)] * gaussianKernel[ky + 3][kx + 3];
                }
            }
            smoothImage[y * nWidth + x] = static_cast<BYTE>(sum / gaussianSum);
        }
    }
    // 边界直接复制
    for (int y = 0; y < nHeight; ++y)
        for (int x = 0; x < nWidth; ++x)
            if (y < 2 || y >= nHeight - 2 || x < 2 || x >= nWidth - 2)
                smoothImage[y * nWidth + x] = grayImage[y * nWidth + x];

    // 3. LoG算子（5x5核）
    const int logKernel[5][5] = {
        { 0,  0, -1,  0,  0},
        { 0, -1, -2, -1,  0},
        {-1, -2, 16, -2, -1},
        { 0, -1, -2, -1,  0},
        { 0,  0, -1,  0,  0}
    };
    std::vector<int> logImage(nWidth * nHeight, 0);
    for (int y = 2; y < nHeight - 2; ++y) {
        for (int x = 2; x < nWidth - 2; ++x) {
            int sum = 0;
            for (int ky = -2; ky <= 2; ++ky) {
                for (int kx = -2; kx <= 2; ++kx) {
                    sum += smoothImage[(y + ky) * nWidth + (x + kx)] * logKernel[ky + 2][kx + 2];
                }
            }
            logImage[y * nWidth + x] = sum;
        }
    }

    // 4. 零交叉检测
    std::vector<BYTE> edgeImage(nWidth * nHeight, 0);
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            int idx = y * nWidth + x;
            int v = logImage[idx];
            bool isEdge = false;
            // 检查8邻域是否有符号变化
            for (int ky = -1; ky <= 1 && !isEdge; ++ky) {
                for (int kx = -1; kx <= 1 && !isEdge; ++kx) {
                    if (ky == 0 && kx == 0) continue;
                    int neighbor = logImage[(y + ky) * nWidth + (x + kx)];
                    if ((v > 0 && neighbor < 0) || (v < 0 && neighbor > 0)) {
                        if (abs(v - neighbor) > 60) // 阈值可调，防止噪声
                            isEdge = true;
                    }
                }
            }
            edgeImage[idx] = isEdge ? 255 : 0;
        }
    }

    //5. 去除孤立点
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            if (edgeImage[y * nWidth + x] == 255) {
                int count = 0;
                for (int ky = -1; ky <= 1; ++ky)
                    for (int kx = -1; kx <= 1; ++kx)
                        if (!(ky == 0 && kx == 0) && edgeImage[(y + ky) * nWidth + (x + kx)] == 255)
                            count++;
                if (count <= 1)
                    edgeImage[y * nWidth + x] = 0;
            }
        }
    }

    // 6. 写回图像
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);
            BYTE value = edgeImage[y * nWidth + x];
            switch (nBitCount) {
            case 8:
                *pixel = value;
                break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    newPixel = (value << 11) | (value << 5) | value;
                }
                else {
                    newPixel = (value << 10) | (value << 5) | value;
                }
                *reinterpret_cast<WORD*>(pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = pixel[1] = pixel[2] = value;
                break;
            }
        }
    }
}


// 实现Sobel边缘检测
void CImageProc::ApplySobelEdgeDetection()
{
    if (!IsValid() || nBitCount < 8)
    {
        AfxMessageBox(_T("不支持此图像格式"));
        return;
    }

    // 创建临时缓冲区存储灰度图像
    std::vector<BYTE> grayImage(nWidth * nHeight);

    // 转换为灰度图像
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE red = 0, green = 0, blue = 0;
            switch (nBitCount) {
            case 8: GetColor8bit(pixel, red, green, blue, x); break;
            case 16: GetColor16bit(pixel, red, green, blue); break;
            case 24: GetColor24bit(pixel, red, green, blue); break;
            case 32: GetColor32bit(pixel, red, green, blue); break;
            }

            grayImage[y * nWidth + x] = static_cast<BYTE>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    }

    // Sobel算子
    const int sobelX[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    const int sobelY[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

    // 应用Sobel算子
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            int gx = 0, gy = 0;

            // 计算梯度
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    BYTE val = grayImage[(y + i) * nWidth + (x + j)];
                    gx += val * sobelX[i + 1][j + 1];
                    gy += val * sobelY[i + 1][j + 1];
                }
            }

            // 计算梯度幅值
            int magnitude = static_cast<int>(sqrt(gx * gx + gy * gy));
            magnitude = min(255, max(0, magnitude));// 防止溢出

            // 更新像素
            BYTE* pixel = GetPixelPtr(x, y);

            switch (nBitCount) {
            case 8:
                *pixel = static_cast<BYTE>(magnitude);
                break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 11) | (v << 5) | v;
                }
                else {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 10) | (v << 5) | v;
                }
                *reinterpret_cast<WORD*>(pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = pixel[1] = pixel[2] = static_cast<BYTE>(magnitude);
                break;
            }
        }
    }
}

// 实现Prewitt边缘检测
void CImageProc::ApplyPrewittEdgeDetection()
{
    if (!IsValid() || nBitCount < 8)
    {
        AfxMessageBox(_T("不支持此图像格式"));
        return;
    }

    // 创建临时缓冲区存储灰度图像
    std::vector<BYTE> grayImage(nWidth * nHeight);

    // 转换为灰度图像
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE red = 0, green = 0, blue = 0;
            switch (nBitCount) {
            case 8: GetColor8bit(pixel, red, green, blue, x); break;
            case 16: GetColor16bit(pixel, red, green, blue); break;
            case 24: GetColor24bit(pixel, red, green, blue); break;
            case 32: GetColor32bit(pixel, red, green, blue); break;
            }

            grayImage[y * nWidth + x] = static_cast<BYTE>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    }

    // Prewitt算子
    const int prewittX[3][3] = { {-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1} };
    const int prewittY[3][3] = { {-1, -1, -1}, {0, 0, 0}, {1, 1, 1} };

    // 应用Prewitt算子
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            int gx = 0, gy = 0;

            // 计算梯度
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    BYTE val = grayImage[(y + i) * nWidth + (x + j)];
                    gx += val * prewittX[i + 1][j + 1];
                    gy += val * prewittY[i + 1][j + 1];
                }
            }

            // 计算梯度幅值
            int magnitude = static_cast<int>(sqrt(gx * gx + gy * gy));
            magnitude = min(255, max(0, magnitude));

            // 更新像素           
            BYTE* pixel = GetPixelPtr(x, y);

            switch (nBitCount) {
            case 8:
                *pixel = static_cast<BYTE>(magnitude);
                break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 11) | (v << 5) | v;
                }
                else {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 10) | (v << 5) | v;
                }
                *reinterpret_cast<WORD*>(pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = pixel[1] = pixel[2] = static_cast<BYTE>(magnitude);
                break;
            }
        }
    }
}

// 实现Robert边缘检测
void CImageProc::ApplyRobertEdgeDetection()
{
    if (!IsValid() || nBitCount < 8)
    {
        AfxMessageBox(_T("不支持此图像格式"));
        return;
    }

    // 创建临时缓冲区存储灰度图像
    std::vector<BYTE> grayImage(nWidth * nHeight);

    // 转换为灰度图像
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE red = 0, green = 0, blue = 0;
            switch (nBitCount) {
            case 8: GetColor8bit(pixel, red, green, blue, x); break;
            case 16: GetColor16bit(pixel, red, green, blue); break;
            case 24: GetColor24bit(pixel, red, green, blue); break;
            case 32: GetColor32bit(pixel, red, green, blue); break;
            }
            grayImage[y * nWidth + x] = static_cast<BYTE>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    }

    // 应用Robert算子
    for (int y = 0; y < nHeight - 1; ++y) {
        for (int x = 0; x < nWidth - 1; ++x) {
            // 计算对角线梯度
            int gx = grayImage[y * nWidth + x] - grayImage[(y + 1) * nWidth + (x + 1)];
            int gy = grayImage[(y + 1) * nWidth + x] - grayImage[y * nWidth + (x + 1)];

            // 计算梯度幅值
            int magnitude = static_cast<int>(sqrt(gx * gx + gy * gy));
            magnitude = min(255, max(0, magnitude));

            // 更新像素
            BYTE* pixel = GetPixelPtr(x, y);

            switch (nBitCount) {
            case 8:
                *pixel = static_cast<BYTE>(magnitude);
                break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 11) | (v << 5) | v;
                }
                else {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 10) | (v << 5) | v;
                }
                *reinterpret_cast<WORD*>(pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = pixel[1] = pixel[2] = static_cast<BYTE>(magnitude);
                break;
            }
        }
    }
}


// 边缘检测-拉普拉斯算子
void CImageProc::ApplyLaplaceEdgeDetection()
{
    if (!IsValid() || nBitCount < 8)
    {
        AfxMessageBox(_T("不支持此图像格式"));
        return;
    }

    // 创建临时缓冲区存储灰度图像
    std::vector<BYTE> grayImage(nWidth * nHeight);

    // 转换为灰度图像
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE red = 0, green = 0, blue = 0;
            switch (nBitCount) {
            case 8: GetColor8bit(pixel, red, green, blue, x); break;
            case 16: GetColor16bit(pixel, red, green, blue); break;
            case 24: GetColor24bit(pixel, red, green, blue); break;
            case 32: GetColor32bit(pixel, red, green, blue); break;
            }

            grayImage[y * nWidth + x] = static_cast<BYTE>(0.299 * red + 0.587 * green + 0.114 * blue);
        }
    }

    // 拉普拉斯算子
    const int invertedLaplacianKernel[3][3] = {
        { -1,  -1,  -1},
        { -1, 8,  -1},
        { -1,  -1,  -1}
    };

    // 应用拉普拉斯算子
    for (int y = 1; y < nHeight - 1; ++y) {
        for (int x = 1; x < nWidth - 1; ++x) {
            int sum = 0;

            // 计算卷积
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    sum += grayImage[(y + ky) * nWidth + (x + kx)] * invertedLaplacianKernel[ky + 1][kx + 1];
                }
            }

            // 计算绝对值并约束到0-255范围
            int magnitude = abs(sum);
            magnitude = min(255, max(0, magnitude));

            // 更新像素
            BYTE* pixel = GetPixelPtr(x, y);

            switch (nBitCount) {
            case 8:
                *pixel = static_cast<BYTE>(magnitude);
                break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 11) | (v << 5) | v;
                }
                else {
                    BYTE v = magnitude >> 3;
                    newPixel = (v << 10) | (v << 5) | v;
                }
                *reinterpret_cast<WORD*>(pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = pixel[1] = pixel[2] = static_cast<BYTE>(magnitude);
                break;
            }
        }
    }
}

void CImageProc::Add(CImageProc& img, double weight1, double weight2)
{
    if (!IsValid() || !img.IsValid() ||
        nWidth != img.nWidth || nHeight != img.nHeight ||
        nBitCount != img.nBitCount)
    {
        AfxMessageBox(_T("图像尺寸或位深不匹配"));
        return;
    }


    // 逐像素相加
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE r1, g1, b1, r2, g2, b2;
            GetColor(x, y, r1, g1, b1);

            img.GetColor(x, y, r2, g2, b2);

            // 加权相加并限制在0-255范围
            BYTE r = min(255, static_cast<int>(r1 * weight1 + r2 * weight2));
            BYTE g = min(255, static_cast<int>(g1 * weight1 + g2 * weight2));
            BYTE b = min(255, static_cast<int>(b1 * weight1 + b2 * weight2));

            // 根据位深设置像素
            switch (nBitCount) {
            case 8: *pixel = 0.299 * r + 0.587 * g + 0.114 * b; break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    newPixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                }
                else {
                    newPixel = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                }
                *((WORD*)pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
                break;
            }
        }
    }
}

void CImageProc::Multiply(CImageProc& img)
{
    if (!IsValid() || !img.IsValid() ||
        nWidth != img.nWidth || nHeight != img.nHeight ||
        nBitCount != img.nBitCount)
    {
        AfxMessageBox(_T("图像尺寸或位深不匹配"));
        return;
    }

    // 逐像素相乘
    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE r1, g1, b1, r2, g2, b2;
            GetColor(x, y, r1, g1, b1);
            img.GetColor(x, y, r2, g2, b2);

            // 相乘并归一化到0-255范围
            BYTE r = (r1 * r2) / 255;
            BYTE g = (g1 * g2) / 255;
            BYTE b = (b1 * b2) / 255;

            // 根据位深设置像素
            switch (nBitCount) {
            case 8: *pixel = 0.299 * r + 0.587 * g + 0.114 * b; break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    newPixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                }
                else {
                    newPixel = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                }
                *((WORD*)pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
                break;
            }
        }
    }
}

void CImageProc::PowerTransform(double gamma)
{
    if (!IsValid()) return;

    // 创建查找表加速计算
    BYTE lut[256];
    for (int i = 0; i < 256; ++i) {
        lut[i] = min(255, (int)(pow(i / 255.0, gamma) * 255 + 0.5));
    }

    for (int y = 0; y < nHeight; ++y) {
        for (int x = 0; x < nWidth; ++x) {
            BYTE* pixel = GetPixelPtr(x, y);

            BYTE r, g, b;
            GetColor(x, y, r, g, b);

            // 应用幂律变换
            r = lut[r];
            g = lut[g];
            b = lut[b];

            // 根据位深设置像素
            switch (nBitCount) {
            case 8: *pixel = 0.299 * r + 0.587 * g + 0.114 * b; break;
            case 16: {
                WORD newPixel;
                if (m_bIs565Format) {
                    newPixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                }
                else {
                    newPixel = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                }
                *((WORD*)pixel) = newPixel;
                break;
            }
            case 24:
            case 32:
                pixel[0] = b;
                pixel[1] = g;
                pixel[2] = r;
                break;
            }
        }
    }
}

// 二维FFT
bool CImageProc::FFT2D(bool bForward, bool bSaveState) {
    if (!IsValid()) return false;

    // 保存原始图像数据
    int dataSize = nWidth * nHeight * (nBitCount / 8);
    m_originalImageData.resize(dataSize);
    memcpy(m_originalImageData.data(), pBits, dataSize);

    if (bSaveState) {
        SaveCurrentState();
    }

    try {
        // 计算补零后的尺寸
        int paddedWidth = nextPowerOfTwo(nWidth);
        int paddedHeight = nextPowerOfTwo(nHeight);
        m_paddedSize = { paddedWidth, paddedHeight };

        // 准备复数数据（每个颜色通道单独处理）
        m_fullSpectrumRGB.resize(3); // R,G,B通道
        for (int ch = 0; ch < 3; ch++) {
            m_fullSpectrumRGB[ch].resize(paddedWidth * paddedHeight);
        }

        // 处理每个颜色通道
        for (int ch = 0; ch < 3; ch++) {
            // 填充数据（注意Y坐标从下往上存储）
            for (int y = 0; y < nHeight; y++) {
                int srcY = nHeight - 1 - y; // 修正：从图像底部开始存储
                for (int x = 0; x < nWidth; x++) {
                    BYTE r, g, b;
                    GetColor(x, y, r, g, b);
                    double val = 0.0;
                    switch (ch) {
                    case 0: val = r / 255.0; break;
                    case 1: val = g / 255.0; break;
                    case 2: val = b / 255.0; break;
                    }
                    m_fullSpectrumRGB[ch][srcY * paddedWidth + x] = { val, 0.0 };
                }
            }

            // 补零区域初始化
            for (int y = nHeight; y < paddedHeight; y++) {
                for (int x = 0; x < paddedWidth; x++) {
                    m_fullSpectrumRGB[ch][y * paddedWidth + x] = { 0.0, 0.0 };
                }
            }
            for (int y = 0; y < nHeight; y++) {
                for (int x = nWidth; x < paddedWidth; x++) {
                    m_fullSpectrumRGB[ch][y * paddedWidth + x] = { 0.0, 0.0 };
                }
            }

            // 执行2D FFT
            for (int y = 0; y < paddedHeight; y++) {
                FFT1D(&m_fullSpectrumRGB[ch][y * paddedWidth], paddedWidth, bForward ? 1 : -1);
            }

            std::vector<std::complex<double>> column(paddedHeight);
            for (int x = 0; x < paddedWidth; x++) {
                for (int y = 0; y < paddedHeight; y++) {
                    column[y] = m_fullSpectrumRGB[ch][y * paddedWidth + x];
                }
                FFT1D(column.data(), paddedHeight, bForward ? 1 : -1);
                for (int y = 0; y < paddedHeight; y++) {
                    m_fullSpectrumRGB[ch][y * paddedWidth + x] = column[y];
                }
            }

            // 频谱移中
            FFTShift(m_fullSpectrumRGB[ch].data(), paddedWidth, paddedHeight);
        }

        // 频谱显示数据处理（对数变换增强）
        m_fftDisplayData.resize(nWidth * nHeight);
        for (int i = 0; i < nWidth * nHeight; i++) {
            double mag = std::abs(m_fullSpectrumRGB[0][i]); // 使用R通道的幅度
            m_fftDisplayData[i] = std::log(1.0 + mag);     // 对数变换
        }

        // 归一化显示数据（修正后的版本）
        auto [minIt, maxIt] = std::minmax_element(
            m_fftDisplayData.begin(),
            m_fftDisplayData.end(),
            [](const std::complex<double>& a, const std::complex<double>& b) {
                return std::abs(a) < std::abs(b);
            });

        double minMag = std::abs(*minIt);
        double maxMag = std::abs(*maxIt);
        double range = maxMag - minMag;

        if (range > 0) {
            for (auto& val : m_fftDisplayData) {
                double magnitude = std::abs(val);
                val = (magnitude - minMag) / range * 255.0;
            }
        }

        m_bFFTPerformed = true;
        return true;
    }
    catch (...) {
        return false;
    }
}

// 一维FFT实现（基2时间抽取算法）
void CImageProc::FFT1D(std::complex<double>* data, int n, int direction) {
    // 检查是否为2的幂次，如果不是则补零
    if (!isPowerOfTwo(n)) {
        int newSize = nextPowerOfTwo(n);
        std::vector<std::complex<double>> newData(newSize);

        std::copy(data, data + n, newData.begin());
        std::fill(newData.begin() + n, newData.end(), std::complex<double>(0, 0));

        FFT1D(newData.data(), newSize, direction);
        std::copy(newData.begin(), newData.begin() + n, data);
        return;
    }

    // 位反转重排
    BitReverse(data, n);

    // 蝶形运算
    for (int s = 1; s <= (int)(log((double)n) / log(2.0)); s++) {
        int m = 1 << s;
        int m2 = m >> 1;
        std::complex<double> wm(cos(2.0 * M_PI / m), direction * sin(2.0 * M_PI / m));

        for (int k = 0; k < n; k += m) {
            std::complex<double> w(1.0, 0.0);
            for (int j = 0; j < m2; j++) {
                std::complex<double> t = w * data[k + j + m2];
                std::complex<double> u = data[k + j];
                data[k + j] = u + t;
                data[k + j + m2] = u - t;
                w *= wm;
            }
        }
    }
}

// 位反转重排
void CImageProc::BitReverse(std::complex<double>* data, int n) {
    for (int i = 0; i < n; i++)
    {
        int target = FindTargetBit(i, n);
        if (i < target)
        {
            std::swap(data[i], data[target]);
        }
    }
}
int CImageProc::FindTargetBit(int i, int n)
{
    int numBits = 0;
    int temp = n;
    // 计算 n 的二进制位数
    while (temp > 1) {
        temp >>= 1;
        numBits++;
    }

    int target = 0;
    for (int j = 0; j < numBits; j++) {
        target = (target << 1) | ((i >> j) & 1);
    }
    return target;
}


void CImageProc::FFTShift(std::complex<double>* data, int width, int height) {
    // 计算中心位置（兼容奇偶尺寸）
    int cx = width / 2;
    int cy = height / 2;

    // 临时存储左上象限
    std::vector<std::complex<double>> temp(cx * cy);
    for (int y = 0; y < cy; ++y) {
        for (int x = 0; x < cx; ++x) {
            temp[y * cx + x] = data[y * width + x];
        }
    }

    // 交换象限（左上←→右下）
    for (int y = 0; y < cy; ++y) {
        for (int x = 0; x < cx; ++x) {
            data[y * width + x] = data[(y + cy) * width + (x + cx)];
            data[(y + cy) * width + (x + cx)] = temp[y * cx + x];
        }
    }

    // 交换象限（右上←→左下）
    for (int y = 0; y < cy; ++y) {
        for (int x = 0; x < cx; ++x) {
            std::swap(data[y * width + (x + cx)], data[(y + cy) * width + x]);
        }
    }
}

void CImageProc::SaveCurrentState() {
    if (!IsValid()) return;

    int dataSize = nWidth * nHeight * (nBitCount / 8);
    m_originalPixels.resize(dataSize);
    memcpy(m_originalPixels.data(), pBits, dataSize);
    m_bStateSaved = true;
}

bool CImageProc::RestoreState() {
    if (!m_bStateSaved || !IsValid()) {
        return false;
    }

    if (m_originalPixels.size() != nWidth * nHeight * (nBitCount / 8)) {
        return false;
    }

    memcpy(pBits, m_originalPixels.data(), m_originalPixels.size());
    m_bFFTPerformed = false;
    return true;
}


bool CImageProc::IFFT2D(bool bSaveState) {
    if (!m_bFFTPerformed) return false;

    try {
        int paddedWidth = m_paddedSize.first;
        int paddedHeight = m_paddedSize.second;

        // 处理每个颜色通道
        std::vector<std::vector<std::complex<double>>> ifftDataRGB = m_fullSpectrumRGB;

        for (int ch = 0; ch < 3; ch++) {
            // 反移中
            FFTShift(ifftDataRGB[ch].data(), paddedWidth, paddedHeight);

            // 执行2D IFFT
            std::vector<std::complex<double>> column(paddedHeight);
            for (int x = 0; x < paddedWidth; x++) {
                for (int y = 0; y < paddedHeight; y++) {
                    column[y] = ifftDataRGB[ch][y * paddedWidth + x];
                }
                FFT1D(column.data(), paddedHeight, -1);
                for (int y = 0; y < paddedHeight; y++) {
                    ifftDataRGB[ch][y * paddedWidth + x] = column[y];
                }
            }

            for (int y = 0; y < paddedHeight; y++) {
                FFT1D(&ifftDataRGB[ch][y * paddedWidth], paddedWidth, -1);
            }

            // 归一化
            const double scale = 1.0 / (paddedWidth * paddedHeight);
            for (auto& val : ifftDataRGB[ch]) {
                val *= scale;
            }
        }

        // 生成图像数据（修正Y方向）
        int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
        m_ifftResult.resize(nHeight * rowSize);

        for (int y = 0; y < nHeight; y++) {
            int srcY = nHeight - 1 - y; // 修正：从存储的底部恢复
            for (int x = 0; x < nWidth; x++) {
                double r = std::clamp(ifftDataRGB[0][srcY * paddedWidth + x].real(), 0.0, 1.0);
                double g = std::clamp(ifftDataRGB[1][srcY * paddedWidth + x].real(), 0.0, 1.0);
                double b = std::clamp(ifftDataRGB[2][srcY * paddedWidth + x].real(), 0.0, 1.0);

                BYTE r8 = static_cast<BYTE>(r * 255.0);
                BYTE g8 = static_cast<BYTE>(g * 255.0);
                BYTE b8 = static_cast<BYTE>(b * 255.0);

                int offset = y * rowSize + x * (nBitCount / 8);
                BYTE* pixel = m_ifftResult.data() + offset;

                switch (nBitCount) {
                case 8:  *pixel = static_cast<BYTE>(0.299 * r8 + 0.587 * g8 + 0.114 * b8); break;
                case 16: // 16位处理
                    if (m_bIs565Format) { // 565格式
                        *reinterpret_cast<WORD*>(pixel) =
                            (static_cast<WORD>(r * 31) << 11) |
                            (static_cast<WORD>(g * 63) << 5) |
                            static_cast<WORD>(b * 31);
                    }
                    else { // 555格式
                        *reinterpret_cast<WORD*>(pixel) =
                            (static_cast<WORD>(r * 31) << 10) |
                            (static_cast<WORD>(g * 31) << 5) |
                            static_cast<WORD>(b * 31);
                    }
                    break;
                case 24:
                    pixel[0] = b8;
                    pixel[1] = g8;
                    pixel[2] = r8;
                    break;
                case 32:
                    pixel[0] = b8;
                    pixel[1] = g8;
                    pixel[2] = r8;
                    pixel[3] = 255;
                    break;
                }
            }
        }

        m_bIFFTPerformed = true;
        return true;
    }
    catch (...) {
        return false;
    }
}

void CImageProc::SetFFTData(const std::vector<std::complex<double>>& data, int w, int h) {
    if (data.size() != w * h) return;

    nWidth = w;
    nHeight = h;
    m_fftData = data;
    m_bFFTPerformed = true;
}

void CImageProc::DisplayIFFTResult(CDC* pDC, int xOffset, int yOffset,
    int destWidth, int destHeight)
{
    if (m_ifftResult.empty()) {
        printf("m_ifftResult is empty!");
        return;
    }

    // 检查数据是否全为0（调试用）
    bool allZero = std::all_of(m_ifftResult.begin(), m_ifftResult.end(),
        [](BYTE b) { return b == 0; });
    if (allZero) {
        printf("m_ifftResult contains all zeros!");
    }

    int srcW = nWidth;
    int srcH = nHeight;

    if (destWidth <= 0) destWidth = srcW;
    if (destHeight <= 0) destHeight = srcH;

    double scaleX = (double)srcW / destWidth;
    double scaleY = (double)srcH / destHeight;

    int rowSize = ((srcW * nBitCount + 31) / 32) * 4;

    for (int y = 0; y < destHeight; y++) {
        // 关键修改：移除(srcH - 1 - )的翻转操作
        int srcY = (int)(y * scaleY);
        if (srcY >= srcH) srcY = srcH - 1;

        for (int x = 0; x < destWidth; x++) {
            int srcX = (int)(x * scaleX);
            if (srcX >= srcW) srcX = srcW - 1;

            // 关键修改：直接使用srcY，不再翻转
            int offset = srcY * rowSize + srcX * (nBitCount / 8);
            BYTE* pixel = m_ifftResult.data() + offset;

            COLORREF color;
            switch (nBitCount) {
            case 8:
                BYTE r, g, b;
                GetColor(srcX, srcY, r, g, b);
                color = RGB(r, g, b);
                break;
            case 16: {
                WORD pixelValue = *reinterpret_cast<WORD*>(pixel);
                if (m_bIs565Format) { // 565格式
                    color = RGB(
                        ((pixelValue & 0xF800) >> 11) * 255 / 31,
                        ((pixelValue & 0x07E0) >> 5) * 255 / 63,
                        (pixelValue & 0x001F) * 255 / 31);
                }
                else { // 555格式
                    color = RGB(
                        ((pixelValue & 0x7C00) >> 10) * 255 / 31,
                        ((pixelValue & 0x03E0) >> 5) * 255 / 31,
                        (pixelValue & 0x001F) * 255 / 31);
                }
                break;
            }
            case 24:
                color = RGB(pixel[2], pixel[1], pixel[0]);
                break;
            case 32:
                color = RGB(pixel[2], pixel[1], pixel[0]);
                break;
            default:
                color = RGB(0, 0, 0);
            }

            pDC->SetPixel(x + xOffset, y + yOffset, color);
        }
    }
}

bool CImageProc::HasOriginalImageData() const {
    return !m_originalImageData.empty();
}

bool CImageProc::HasIFFTResult() const {
    return !m_ifftResult.empty() && m_bIFFTPerformed; // 确保 m_bIFFTPerformed 为 true
}

void CImageProc::DisplayOriginalImage(CDC* pDC, int xOffset, int yOffset,
    int destWidth, int destHeight) {
    if (m_originalImageData.empty()) return;

    int srcW = nWidth;
    int srcH = nHeight;

    if (destWidth <= 0) destWidth = srcW;
    if (destHeight <= 0) destHeight = srcH;

    double scaleX = (double)srcW / destWidth;
    double scaleY = (double)srcH / destHeight;

    int rowSize = ((srcW * nBitCount + 31) / 32) * 4;

    for (int y = 0; y < destHeight; y++) {
        int srcY = (int)(y * scaleY);
        if (srcY >= srcH) srcY = srcH - 1;

        for (int x = 0; x < destWidth; x++) {
            int srcX = (int)(x * scaleX);
            if (srcX >= srcW) srcX = srcW - 1;

            int offset = (srcH - 1 - srcY) * rowSize + srcX * (nBitCount / 8);
            BYTE* pixel = m_originalImageData.data() + offset;

            COLORREF color;
            switch (nBitCount) {
            case 8: 
                BYTE r, g, b;
                GetColor(srcX, srcY, r, g, b);
                color = RGB(r, g, b);
                break;
            case 24: color = RGB(pixel[2], pixel[1], pixel[0]); break;
            case 16: {
                WORD pixelValue = *reinterpret_cast<WORD*>(pixel);
                BYTE r, g, b;
                GetColor16bit(reinterpret_cast<BYTE*>(&pixelValue), r, g, b);
                color = RGB(r, g, b);
                break;
            }
            case 32: color = RGB(pixel[2], pixel[1], pixel[0]); break;
            default: color = RGB(0, 0, 0);
            }

            pDC->SetPixel(x + xOffset, y + yOffset, color);
        }
    }
}

void CImageProc::ResetFFTState() {
    m_bFFTPerformed = false;
    m_bIFFTPerformed = false;
    m_originalImageData.clear();
    m_fftData.clear();
    m_fftDisplayData.clear();
    m_ifftResult.clear();
    m_originalFFTData.clear();
    m_originalPixels.clear();
    m_bStateSaved = false;
}

void CImageProc::FreqPassFilter(double D0, int n, int filterType)
{
    if (!IsValid() || (nBitCount != 8 && nBitCount != 16 && nBitCount != 24 && nBitCount != 32)) {
        AfxMessageBox(_T("仅支持8/16/24/32位图像!"));
        return;
    }

    int w = nWidth, h = nHeight, N = w * h;//  图像尺寸

    if (nBitCount == 8) {
        // 分配输入输出数组
        fftw_complex* in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);//in用于存储输入数据
        fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);//out用于存储输出数据

        // 填充输入数据并应用(-1)^(x+y)进行频谱中心化
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int offset = (h - 1 - y) * GetAlignedWidthBytes() + x;
                // 应用(-1)^(x+y)进行频谱中心化
                double factor = ((x + y) % 2 == 0) ? 1.0 : -1.0;//factor是(-1)^(x+y)
                in[y * w + x][0] = pBits[offset] * factor;//归一化的输入数据
                in[y * w + x][1] = 0.0;
            }
        }

        // 正变换
        fftw_plan plan = fftw_plan_dft_2d(h, w, in, out, FFTW_FORWARD, FFTW_ESTIMATE);//创建正变换计划
        fftw_execute(plan);//执行正变换

        int cx = w / 2, cy = h / 2;

        switch (filterType) {
        case 0:
            // 理想高通滤波
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));//计算与中心点的距离
                    //应用理想高通滤波
                    if (D < D0) {
                        out[y * w + x][0] = 0;
                        out[y * w + x][1] = 0;
                    }
                }
            }
            break;
        case 1:
            // Butterworth高通滤波
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    double H = 1.0 / (1.0 + pow(D0 / D, 2 * n));
                    out[y * w + x][0] *= H;
                    out[y * w + x][1] *= H;
                }
            }
            break;
        case 2:
            // 理想低通滤波
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    if (D > D0) {
                        out[y * w + x][0] = 0;
                        out[y * w + x][1] = 0;
                    }
                }
            }
            break;
        case 3:
            //butterworth低通滤波
			for (int y = 0; y < h; ++y) {
				for (int x = 0; x < w; ++x) {
					double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
					double H = 1.0 / (1.0 + pow(D / D0, 2 * n));
					out[y * w + x][0] *= H;
					out[y * w + x][1] *= H;
				}
			}
            break;
        }

        // 逆变换
        fftw_plan iplan = fftw_plan_dft_2d(h, w, out, in, FFTW_BACKWARD, FFTW_ESTIMATE);//创建逆变换计划
        fftw_execute(iplan);//执行逆变换

        // 中心化恢复
        double minVal = 1e20, maxVal = -1e20;// 用于记录灰度值的最小值和最大值
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                double factor = ((x + y) % 2 == 0) ? 1.0 : -1.0;
                double val = in[y * w + x][0] / N * factor;// 频谱中心化恢复
                if (val < minVal) minVal = val;
                if (val > maxVal) maxVal = val;//  记录灰度值范围
            }
        }

        // 归一化并映射到[0, 255]
        double range = maxVal - minVal;
        if (range < 1e-6) range = 1.0; // 防止除零

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int offset = (h - 1 - y) * GetAlignedWidthBytes() + x;
                double factor = ((x + y) % 2 == 0) ? 1.0 : -1.0;
                double val = in[y * w + x][0] / N * factor;
                val = (val - minVal) * 255.0 / range;
                val = min(255.0, max(0.0, val));
                pBits[offset] = static_cast<BYTE>(val);//将结果写回到图像数据中
            }
        }

        fftw_destroy_plan(plan);
        fftw_destroy_plan(iplan);   // 销毁逆变换计划
        fftw_free(in);
        fftw_free(out);     // 释放内存
        return;
    }

    // 16位、24位、32位彩色图像
    int bytesPerPixel = nBitCount / 8;
    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;

    // 处理每个通道
    for (int channel = 0; channel < 3; ++channel) { // 0:B, 1:G, 2:R
        fftw_complex* in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
        fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);

        for (int y = 0; y < h; ++y) {
            BYTE* pPixel = pBits + (h - 1 - y) * rowSize;
            for (int x = 0; x < w; ++x) {
                int idx = y * w + x;
                int val = 0;
                if (nBitCount == 16) {
                    WORD* pixel = (WORD*)(pPixel + x * 2);
                    WORD color = *pixel;
                    if (m_bIs565Format) {
                        if (channel == 2) val = ((color >> 11) & 0x1F) << 3; // R
                        else if (channel == 1) val = ((color >> 5) & 0x3F) << 2; // G
                        else val = (color & 0x1F) << 3; // B
                    }
                    else {
                        if (channel == 2) val = ((color >> 10) & 0x1F) << 3; // R
                        else if (channel == 1) val = ((color >> 5) & 0x1F) << 3; // G
                        else val = (color & 0x1F) << 3; // B
                    }
                }
                else if (nBitCount == 24 || nBitCount == 32) {
                    BYTE* pixel = pPixel + x * bytesPerPixel;
                    val = pixel[channel];
                }
                double factor = ((x + y) % 2 == 0) ? 1.0 : -1.0;
                in[idx][0] = val * factor;
                in[idx][1] = 0.0;
            }
        }

        fftw_plan plan = fftw_plan_dft_2d(h, w, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_execute(plan);
        
        int cx = w / 2, cy = h / 2;

        switch (filterType) {
        case 0:
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    if (D < D0) {
                        out[y * w + x][0] = 0;
                        out[y * w + x][1] = 0;
                    }
                }
            }
            break;
        case 1:
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    double H = 1.0 / (1.0 + pow(D0 / D, 2 * n));
                    out[y * w + x][0] *= H;
                    out[y * w + x][1] *= H;
                }
            }
            break;
        case 2:
            // 理想低通滤波
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    if (D > D0) {
                        out[y * w + x][0] = 0;
                        out[y * w + x][1] = 0;
                    }
                }
            }
            break;
        case 3:
            //butterworth低通滤波
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    double D = sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    double H = 1.0 / (1.0 + pow(D / D0, 2 * n));
                    out[y * w + x][0] *= H;
                    out[y * w + x][1] *= H;
                }
            }
            break;
        }


        fftw_plan iplan = fftw_plan_dft_2d(h, w, out, in, FFTW_BACKWARD, FFTW_ESTIMATE);
        fftw_execute(iplan);

        // 归一化
        double minVal = 1e20, maxVal = -1e20;
        for (int i = 0; i < N; ++i) {
            double val = in[i][0] / N;
            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }
        double range = maxVal - minVal;
        if (range < 1e-6) range = 1.0;

        for (int y = 0; y < h; ++y) {
            BYTE* pPixel = pBits + (h - 1 - y) * rowSize;
            for (int x = 0; x < w; ++x) {
                int idx = y * w + x;
                double factor = ((x + y) % 2 == 0) ? 1.0 : -1.0;
                double val = in[idx][0] / N * factor;
                val = (val - minVal) * 255.0 / range;
                val = min(255.0, max(0.0, val));
                if (nBitCount == 16) {
                    WORD* pixel = (WORD*)(pPixel + x * 2);
                    WORD color = *pixel;
                    if (m_bIs565Format) {
                        if (channel == 2) color = (color & 0x07FF) | (((int(val) >> 3) & 0x1F) << 11); // R
                        else if (channel == 1) color = (color & 0xF81F) | (((int(val) >> 2) & 0x3F) << 5); // G
                        else color = (color & 0xFFE0) | ((int(val) >> 3) & 0x1F); // B
                    }
                    else {
                        if (channel == 2) color = (color & 0x03FF) | (((int(val) >> 3) & 0x1F) << 10); // R
                        else if (channel == 1) color = (color & 0x7C1F) | (((int(val) >> 3) & 0x1F) << 5); // G
                        else color = (color & 0xFFE0) | ((int(val) >> 3) & 0x1F); // B
                    }
                    *pixel = color;
                }
                else if (nBitCount == 24 || nBitCount == 32) {
                    BYTE* pixel = pPixel + x * bytesPerPixel;
                    pixel[channel] = static_cast<BYTE>(val);
                }
            }
        }

        fftw_destroy_plan(plan);
        fftw_destroy_plan(iplan);
        fftw_free(in);
        fftw_free(out);
    }

}

// 同态滤波
void CImageProc::HomomorphicFiltering() {
    if (!IsValid()) {
        AfxMessageBox(_T("No valid image is loaded."));
        return;
    }

    int M = nHeight;
    int N = nWidth;
    int rowSize = ((N * nBitCount + 31) / 32) * 4;

    // 取对数
    std::vector<double> img_log(M * N);
    for (int y = 0; y < M; ++y) {
        for (int x = 0; x < N; ++x) {
            int offset = (M - 1 - y) * rowSize + x * (nBitCount / 8);
            BYTE* pixel = pBits + offset;
            double gray = 0;

            switch (nBitCount) {
            case 8:
                gray = *pixel;
                break;
            case 16: {
                WORD value = *reinterpret_cast<WORD*>(pixel);
                if (m_bIs565Format) {
                    BYTE r = (value >> 11) & 0x1F;
                    BYTE g = (value >> 5) & 0x3F;
                    BYTE b = value & 0x1F;
                    gray = 0.299 * (r << 3) + 0.587 * (g << 2) + 0.114 * (b << 3);
                }
                else {
                    BYTE r = (value >> 10) & 0x1F;
                    BYTE g = (value >> 5) & 0x1F;
                    BYTE b = value & 0x1F;
                    gray = 0.299 * (r << 3) + 0.587 * (g << 3) + 0.114 * (b << 3);
                }
                break;
            }
            case 24:
            case 32:
                gray = 0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0];
                break;
            default:
                AfxMessageBox(_T("Unsupported bit count."));
                return;
            }

            img_log[y * N + x] = std::log(gray + 1);
        }
    }

    // 平移到中心
    std::vector<double> img_py(M * N);
    for (int y = 0; y < M; ++y) {
        for (int x = 0; x < N; ++x) {
            img_py[y * N + x] = ((y + x) % 2 == 0) ? img_log[y * N + x] : -img_log[y * N + x];
        }
    }

    // 傅里叶变换
    fftw_complex* in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * M * N);
    fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * M * N);
    for (int i = 0; i < M * N; ++i) {
        in[i][0] = img_py[i];
        in[i][1] = 0;
    }
    fftw_plan p = fftw_plan_dft_2d(M, N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);

    // 同态滤波函数
    double rH = 1.1, rL = 0.3, c = 0.4, D0 = 15;
    std::vector<double> img_tt(M * N);
    double deta_r = rH - rL, D = D0 * D0;
    int m_mid = M / 2, n_mid = N / 2;
    for (int y = 0; y < M; ++y) {
        for (int x = 0; x < N; ++x) {
            double dis = (y - m_mid) * (y - m_mid) + (x - n_mid) * (x - n_mid);
            img_tt[y * N + x] = deta_r * (1 - exp(-c * dis / D)) + rL;
        }
    }

    // 滤波
    for (int i = 0; i < M * N; ++i) {
        out[i][0] *= img_tt[i];
        out[i][1] *= img_tt[i];
    }

    // 反变换
    fftw_complex* in2 = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * M * N);
    fftw_complex* out2 = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * M * N);
    for (int i = 0; i < M * N; ++i) {
        in2[i][0] = out[i][0];
        in2[i][1] = out[i][1];
    }
    fftw_plan p2 = fftw_plan_dft_2d(M, N, in2, out2, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(p2);

    // 取实部
    std::vector<double> img_temp(M * N);
    for (int i = 0; i < M * N; ++i) {
        img_temp[i] = std::abs(out2[i][0]) / (M * N);
    }

    // 指数化
    for (int i = 0; i < M * N; ++i) {
        img_temp[i] = exp(img_temp[i]) - 1;
    }

    // 归一化处理
    double max_num = img_temp[0], min_num = img_temp[0];
    for (int i = 0; i < M * N; ++i) {
        max_num = max(max_num, img_temp[i]);
        min_num = min(min_num, img_temp[i]);
    }
    double range = (max_num - min_num) == 0 ? 1e-6 : max_num - min_num;

    // 像素赋值
    for (int y = 0; y < M; ++y) {
        for (int x = 0; x < N; ++x) {
            int offset = (M - 1 - y) * rowSize + x * (nBitCount / 8);
            BYTE* pixel = pBits + offset;
            double newValue = 255.0 * (img_temp[y * N + x] - min_num) / range;
            newValue = max(0.0, min(255.0, newValue));

            switch (nBitCount) {
            case 8:
                *pixel = static_cast<BYTE>(newValue);
                break;
            case 16: {
                BYTE r = static_cast<BYTE>(newValue);
                BYTE g = static_cast<BYTE>(newValue);
                BYTE b = static_cast<BYTE>(newValue);
                WORD value = (m_bIs565Format) ?
                    ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) :
                    ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                *reinterpret_cast<WORD*>(pixel) = value;
                break;
            }
            case 24:
            case 32:
                pixel[0] = static_cast<BYTE>(newValue); // B 通道
                pixel[1] = static_cast<BYTE>(newValue); // G 通道
                pixel[2] = static_cast<BYTE>(newValue); // R 通道
                if (nBitCount == 32) pixel[3] = 255; // Alpha 通道
                break;
            }
        }
    }

    // 释放 FFTW 内存
    fftw_free(in);
    fftw_free(out);
    fftw_free(in2);
    fftw_free(out2);
    fftw_destroy_plan(p);
    fftw_destroy_plan(p2);
}

void CImageProc::ShowSpectrumDialog(CWnd* pParent)
{
    CSpectrumDlg dlg(pParent, this);
    dlg.DoModal();
}

void CImageProc::DisplayFullSpectrum(CDC* pDC, int xOffset, int yOffset,
    int destWidth, int destHeight)
{
    if (!m_bFFTPerformed || m_fullSpectrumRGB.empty()) return;

    int srcW = m_paddedSize.first;
    int srcH = m_paddedSize.second;

    if (destWidth <= 0) destWidth = srcW;
    if (destHeight <= 0) destHeight = srcH;

    // 计算缩放比例
    double scaleX = (double)srcW / destWidth;
    double scaleY = (double)srcH / destHeight;

    // 创建临时位图
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(pDC, destWidth, destHeight);
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // 绘制完整频谱
    for (int y = 0; y < destHeight; y++) {
        int srcY = (int)(y * scaleY);
        if (srcY >= srcH) srcY = srcH - 1;

        for (int x = 0; x < destWidth; x++) {
            int srcX = (int)(x * scaleX);
            if (srcX >= srcW) srcX = srcW - 1;

            // 计算幅度（使用R通道）
            double mag = std::abs(m_fullSpectrumRGB[0][srcY * srcW + srcX]);
            int intensity = static_cast<int>(255 * std::log(1 + mag) / std::log(1 + 255));
            intensity = std::clamp(intensity, 0, 255);

            memDC.SetPixel(x, y, RGB(intensity, intensity, intensity));
        }
    }

    // 将位图绘制到目标DC
    pDC->BitBlt(xOffset, yOffset, destWidth, destHeight, &memDC, 0, 0, SRCCOPY);

    // 清理资源
    memDC.SelectObject(pOldBitmap);
    bitmap.DeleteObject();
    memDC.DeleteDC();
}

// 递归生成编码表
void BuildHuffmanCodeTable(HuffmanNode* node, std::map<BYTE, std::vector<bool>>& table, std::vector<bool>& path) {
    if (!node->left && !node->right) {
        table[node->value] = path;// 如果是叶子节点，则将路径保存到编码表中
        return;
    }
    if (node->left) {
        path.push_back(false);//  左子树的路径添加“0”
        BuildHuffmanCodeTable(node->left, table, path);//  递归处理左子树
        path.pop_back();
    }
    if (node->right) {
        path.push_back(true);//  右子树的路径添加“1”
        BuildHuffmanCodeTable(node->right, table, path);//  递归处理右子树
        path.pop_back();
    }
}

// 释放霍夫曼树
void FreeHuffmanTree(HuffmanNode* node) {
    if (!node) return;
    FreeHuffmanTree(node->left);
    FreeHuffmanTree(node->right);
    delete node;
}

// 霍夫曼编码主函数
bool CImageProc::HuffmanEncodeImage(const CString& savePath) {
    if (!IsValid()) {
        AfxMessageBox(_T("没有有效的图像可编码"));
        return false;
    }

    // 计算像素数据大小
    int bytesPerPixel = nBitCount / 8;
    int dataSize = nWidth * nHeight * bytesPerPixel;
    BYTE* data = pBits;

    // 1. 统计频率
    std::map<BYTE, int> freq;
    for (int i = 0; i < dataSize; ++i) {
        freq[data[i]]++;
    }

    // 2. 构建霍夫曼树
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNode> pq;// 优先队列
    for (auto& kv : freq)//  遍历频率表，将kv的键值对加入优先队列
    {
        pq.push(new HuffmanNode(kv.first, kv.second));//kv.first为像素值，kv.second为频率
    }
    if (pq.empty()) {
        AfxMessageBox(_T("图像数据为空"));
        return false;
    }
    // 构建霍夫曼树
    while (pq.size() > 1) {
        HuffmanNode* l = pq.top(); pq.pop();
        HuffmanNode* r = pq.top(); pq.pop();
        pq.push(new HuffmanNode(l, r));
    }
    HuffmanNode* root = pq.top();

    // 3. 生成编码表
    std::map<BYTE, std::vector<bool>> codeTable;
    std::vector<bool> path;
    BuildHuffmanCodeTable(root, codeTable, path);// 生成编码表

    // 4. 写文件
    std::ofstream ofs(CW2A(savePath), std::ios::binary);// 二进制文件
    if (!ofs) {
        FreeHuffmanTree(root);
        AfxMessageBox(_T("无法打开保存文件"));
        return false;
    }

    // 写文件头（宽、高、位深、编码表大小）
    ofs.write((char*)&nWidth, sizeof(nWidth));
    ofs.write((char*)&nHeight, sizeof(nHeight));
    ofs.write((char*)&nBitCount, sizeof(nBitCount));

    int clrUsed = (pBIH && pBIH->biClrUsed > 0) ? pBIH->biClrUsed : (1 << nBitCount);
    ofs.write((char*)&clrUsed, sizeof(clrUsed));

    int tableSize = (int)codeTable.size();
    ofs.write((char*)&tableSize, sizeof(tableSize));

    // 写编码表
    for (auto& kv : codeTable) {
        ofs.write((char*)&kv.first, 1); // 像素值
        BYTE len = (BYTE)kv.second.size();
        ofs.write((char*)&len, 1);      // 编码长度
        BYTE buf = 0, cnt = 0;//buf是一个字节的数据，cnt是buf中有效数据的位数，当cnt等于8时，将buf写入文件，buf清零，cnt清零
        for (bool b : kv.second) {
            buf = (buf << 1) | b;// 将当前位添加到buf中
            cnt++;//  计数器加1
            if (cnt == 8) {
                ofs.write((char*)&buf, 1);//  以1个字节为单位写入文件
                buf = 0;
                cnt = 0;
            }
        }
        // 处理剩余的位数
        if (cnt) {
            buf <<= (8 - cnt);
            ofs.write((char*)&buf, 1);
        }
    }

    // 写编码数据
    BYTE buf = 0, cnt = 0;
    for (int i = 0; i < dataSize; ++i) {
        for (bool b : codeTable[data[i]]) {
            buf = (buf << 1) | b;
            cnt++;
            if (cnt == 8) {
                ofs.write((char*)&buf, 1);
                buf = 0;
                cnt = 0;
            }
        }
    }
    if (cnt) {
        buf <<= (8 - cnt);
        ofs.write((char*)&buf, 1);
    }
    if (nBitCount == 16) {
        // 判断是否为565格式（需要从原始BMP文件获取这个信息）
        // 保存格式信息（0表示555，1表示565）
        BYTE formatFlag = m_bIs565Format ? 1 : 0;
        ofs.write((char*)&formatFlag, 1);
    }

    //写调色盘数据
    if (nBitCount <= 8 && pQUAD) {
        // 写入实际使用的调色板大小（以字节为单位）
        int paletteSize = clrUsed * sizeof(RGBQUAD);
        ofs.write((char*)pQUAD, paletteSize);
    }

    ofs.close();
    // 计算压缩率
    DWORD originalSize = nWidth * nHeight * (nBitCount / 8); // 原始像素数据大小
    if (nBitCount <= 8 && pQUAD) {
        originalSize += clrUsed * sizeof(RGBQUAD); // 加上调色板大小
    }

    CFile compressedFile(savePath, CFile::modeRead);
    DWORD compressedSize = compressedFile.GetLength(); // 压缩后文件大小
    compressedFile.Close();

    double compressionRatio = (double)originalSize / compressedSize;
    CString msg;
    msg.Format(_T("霍夫曼编码完成\n原始大小: %d 字节\n压缩后大小: %d 字节\n压缩率: %.2f"),
        originalSize, compressedSize, compressionRatio);
    AfxMessageBox(msg);
    FreeHuffmanTree(root);
    return true;
}

//霍夫曼解码
bool CImageProc::HuffmanDecodeImage(const CString& openPath) {
    // 1. 打开文件
    std::ifstream ifs(CW2A(openPath), std::ios::binary);
    if (!ifs) {
        AfxMessageBox(_T("无法打开解码文件"));
        return false;
    }

    // 2. 读取文件头
    int width = 0, height = 0, bitCount = 0, tableSize = 0, clrUsed = 0;
    ifs.read((char*)&width, sizeof(width));
    ifs.read((char*)&height, sizeof(height));
    ifs.read((char*)&bitCount, sizeof(bitCount));
    ifs.read((char*)&clrUsed, sizeof(clrUsed));
    ifs.read((char*)&tableSize, sizeof(tableSize));
    if (width <= 0 || height <= 0 || bitCount <= 0 || tableSize <= 0) {
        AfxMessageBox(_T("文件头的信息无效"));
        return false;
    }

    // 3. 读取编码表
    struct CodeEntry {
        BYTE value;
        std::vector<bool> code;
    };
    std::vector<CodeEntry> codeEntries;
    for (int i = 0; i < tableSize; ++i) {
        BYTE val = 0, len = 0;
        ifs.read((char*)&val, 1);
        ifs.read((char*)&len, 1);
        std::vector<bool> code;
        int bitsRead = 0;
        while (bitsRead < len) {
            BYTE buf = 0;
            ifs.read((char*)&buf, 1);
            int remain = len - bitsRead;
            int bits = min(8, remain);
            for (int b = 7; b >= 8 - bits; --b) {
                code.push_back((buf >> b) & 1);
            }
            bitsRead += bits;
        }
        codeEntries.push_back({ val, code });
    }

    // 4. 重建Huffman树
    HuffmanNode* root = new HuffmanNode(0, 0);
    for (const auto& entry : codeEntries) {
        HuffmanNode* node = root;
        for (size_t i = 0; i < entry.code.size(); ++i) {
            if (entry.code[i]) {
                if (!node->right) node->right = new HuffmanNode(0, 0);
                node = node->right;
            }
            else {
                if (!node->left) node->left = new HuffmanNode(0, 0);
                node = node->left;
            }
        }
        node->value = entry.value;
    }

    // 5. 读取编码数据并解码
    int bytesPerPixel = bitCount / 8;
    int dataSize = width * height * bytesPerPixel;
    std::vector<BYTE> decodedData;
    decodedData.reserve(dataSize);

    HuffmanNode* node = root;
    BYTE buf = 0;
    int bitsLeft = 0;
    while ((int)decodedData.size() < dataSize && ifs) {
        if (bitsLeft == 0) {
            ifs.read((char*)&buf, 1);
            bitsLeft = 8;
        }
        bool bit = (buf & (1 << (bitsLeft - 1))) != 0;
        bitsLeft--;
        node = bit ? node->right : node->left;
        if (node && !node->left && !node->right) {
            decodedData.push_back(node->value);
            node = root;
        }
    }
    FreeHuffmanTree(root);

    if ((int)decodedData.size() != dataSize) {
        AfxMessageBox(_T("解码数据长度不符"));
        return false;
    }

    // 6. 构建BMP内存结构
    CleanUp();
    int rowSize = ((width * bitCount + 31) / 32) * 4;
    DWORD dibSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
        ((bitCount <= 8) ? (1 << bitCount) * sizeof(RGBQUAD) : 0) + rowSize * height;
    m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, dibSize);
    if (!m_hDib) {
        AfxMessageBox(_T("内存分配失败"));
        return false;
    }
    pDib = (BYTE*)::GlobalLock(m_hDib);
    pBFH = (BITMAPFILEHEADER*)pDib;
    pBIH = (BITMAPINFOHEADER*)(pDib + sizeof(BITMAPFILEHEADER));
    nWidth = width;
    nHeight = height;
    nBitCount = bitCount;

    // 填写文件头
    pBFH->bfType = 0x4D42;
    pBFH->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
        ((bitCount <= 8) ? (1 << bitCount) * sizeof(RGBQUAD) : 0);
    pBFH->bfSize = dibSize;
    pBFH->bfReserved1 = 0;
    pBFH->bfReserved2 = 0;

    // 填写信息头
    pBIH->biSize = sizeof(BITMAPINFOHEADER);
    pBIH->biWidth = width;
    pBIH->biHeight = height;
    pBIH->biPlanes = 1;
    pBIH->biBitCount = bitCount;
    pBIH->biCompression = 0;
    pBIH->biSizeImage = rowSize * height;
    pBIH->biXPelsPerMeter = 0;
    pBIH->biYPelsPerMeter = 0;
    pBIH->biClrUsed = 0;
    pBIH->biClrImportant = 0;

    // 填写调色板
    if (bitCount <= 8) {
        pQUAD = (RGBQUAD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

        // 读取实际使用的调色板大小（以字节为单位）
        int paletteSize = clrUsed * sizeof(RGBQUAD);
        ifs.read((char*)pQUAD, paletteSize);

        // 设置 BMP 信息头中的 biClrUsed
        pBIH->biClrUsed = clrUsed;
    }
    else {
        pQUAD = nullptr;
    }

    if (bitCount == 16) {
        // 读取格式标记
        BYTE formatFlag = 0;
        ifs.read((char*)&formatFlag, 1);
        bool is565Format = (formatFlag == 1);

        if (is565Format) {
            // 设置正确的BITMAPINFOHEADER结构，表明这是565格式
            pBIH->biCompression = BI_BITFIELDS;

            // 添加掩码信息
            DWORD* masks = (DWORD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
            masks[0] = 0xF800; // R掩码
            masks[1] = 0x07E0; // G掩码
            masks[2] = 0x001F; // B掩码

            // 更新文件头中的bfOffBits
            pBFH->bfOffBits += 3 * sizeof(DWORD); // 增加三个掩码的大小
        }
    }
    // 填写像素数据
    pBits = pDib + pBFH->bfOffBits;
    memcpy(pBits, decodedData.data(), dataSize);
    return true;
}

// LZW编码
bool CImageProc::LZWEncodeImage(const CString& savePath)
{
    if (!IsValid()) return false;

    // 获取图像数据
    int width = nWidth;
    int height = nHeight;
    int bitCount = nBitCount;
    int bytesPerPixel = bitCount / 8;
    int alignedWidth = GetAlignedWidthBytes(); // 对齐后的宽度

    // 创建输入数据流
    std::vector<BYTE> inputData;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            BYTE* pixel = GetPixelPtr(x, y);

            // 根据位深度处理像素数据
            switch (bitCount) {
            case 1:
            case 4:
            case 8:
                inputData.push_back(*pixel);
                break;
            case 16: {
                // 16位图像 - 按WORD读取
                WORD pixelValue = *((WORD*)pixel);
                inputData.push_back(pixelValue & 0xFF);        // 低字节
                inputData.push_back((pixelValue >> 8) & 0xFF); // 高字节
                break;
            }
            case 24:
            case 32:
                // 24/32位图像 - 按通道读取
                for (int c = 0; c < bytesPerPixel; c++) {
                    inputData.push_back(pixel[c]);
                }
                break;
            default:
                AfxMessageBox(_T("该位深的图像不支持LZW编码"));
                return false;
            }
        }
    }

    // LZW编码
	std::vector<int> outputCodes; // 存储输出的编码
    std::unordered_map<std::vector<BYTE>, int> dictionary;

    //-读取下一个字符
    //    - 检查当前字符串加上新字符是否在字典中
    //    - 如果在字典中：将新字符添加到当前字符串中，继续读取下一个字符
    //    - 如果不在字典中：
    //    - 输出当前字符串的编码值
    //    - 将当前字符串加上新字符添加到字典中，并分配下一个可用编码值
    //    - 将当前字符串重置为新读取的字符
    //    - 重复上述过程，直到处理完所有输入数据

    // 初始化字典 (0-255为单字节值)
    int nextCode = 256; // 0-255已经被单字节占用
    for (int i = 0; i < 256; i++) {
        std::vector<BYTE> singleByte = { static_cast<BYTE>(i) };
        dictionary[singleByte] = i;
    }

	std::vector<BYTE> currentSequence; // 当前处理的序列 逐字节添加
    for (size_t i = 0; i < inputData.size(); i++) {
        std::vector<BYTE> newSequence = currentSequence;
        newSequence.push_back(inputData[i]);

        if (dictionary.find(newSequence) != dictionary.end()) {
            // 序列在字典中，继续添加
            currentSequence = newSequence;
        }
        else {
            // 序列不在字典中，输出当前序列的编码，并添加新序列到字典
            outputCodes.push_back(dictionary[currentSequence]);

            // 如果字典未满，添加新序列
            if (nextCode < 4096) { // 限制字典大小为12位
                dictionary[newSequence] = nextCode++;
            }

            // 重置当前序列为当前字节
            currentSequence = { inputData[i] };
        }
    }

    // 输出最后一个序列的编码
    if (!currentSequence.empty()) {
        outputCodes.push_back(dictionary[currentSequence]);
    }

    // 写入文件
    std::ofstream outFile(savePath, std::ios::binary);
    if (!outFile) {
        AfxMessageBox(_T("Failed to create output file!"));
        return false;
    }

    // 写入图像信息头
    outFile.write("LZW", 3);
    outFile.write(reinterpret_cast<const char*>(&nWidth), sizeof(nWidth));
    outFile.write(reinterpret_cast<const char*>(&nHeight), sizeof(nHeight));
    outFile.write(reinterpret_cast<const char*>(&nBitCount), sizeof(nBitCount));
    // 保存16位图像格式标志
    if (nBitCount == 16) {
        BYTE formatFlag = m_bIs565Format ? 1 : 0;
        outFile.write(reinterpret_cast<const char*>(&formatFlag), sizeof(formatFlag));
    }
    // 如果是8位或更低位深度的图像，保存颜色表信息
    if (nBitCount <= 8 && pQUAD != NULL) {
        // 写入颜色表大小
        int paletteSize = 1 << nBitCount;
        outFile.write(reinterpret_cast<const char*>(&paletteSize), sizeof(paletteSize));
        
        // 写入颜色表数据
        for (int i = 0; i < paletteSize; i++) {
            outFile.write(reinterpret_cast<const char*>(&pQUAD[i]), sizeof(RGBQUAD));
        }
    } else {
        // 如果没有颜色表，写入0
        int paletteSize = 0;
        outFile.write(reinterpret_cast<const char*>(&paletteSize), sizeof(paletteSize));
    }

    // 写入编码数据长度
    int codeCount = static_cast<int>(outputCodes.size());
    outFile.write(reinterpret_cast<const char*>(&codeCount), sizeof(codeCount));

    // 写入编码数据 
    std::vector<BYTE> compressedData; // 存储压缩后的字节数据
    int bitBuffer = 0; // 位缓冲区，存储待写入的位
    int bitCountBuffer = 0; // 记录缓冲区中有效位的数量

    for (int code : outputCodes) {
        // 将12位编码添加到缓冲区 将12位的LZW编码值打包成8位的字节序列
        bitBuffer |= (code << bitCountBuffer); // 将 12 位编码左移当前缓冲区的偏移量并合并缓冲区
        bitCountBuffer += 12; // 更新缓冲区中的有效位数

        // 当缓冲区中有足够的位时，写入字节
        while (bitCountBuffer >= 8) {
            compressedData.push_back(bitBuffer & 0xFF);
            bitBuffer >>= 8;
            bitCountBuffer -= 8;
        }
    }

    // 写入剩余的位
    if (bitCountBuffer > 0) {
        compressedData.push_back(bitBuffer & 0xFF);
    }

    // 写入压缩后的数据
    outFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
    outFile.close();

    // 计算压缩率
    double originalSize = inputData.size();
    double compressedSize = compressedData.size();
    double ratio = (originalSize /compressedSize );

    // 确保压缩率不显示为负值
    if (ratio < 0) {
        ratio = 0;
        CString message;
        message.Format(_T("LZW encoding completed, but compression increased size!\nOriginal size: %.2f KB\nCompressed size: %.2f KB"),
            originalSize / 1024.0, compressedSize / 1024.0);
        AfxMessageBox(message);
    }
    else {
        CString message;
        message.Format(_T("LZW encoding completed!\nOriginal size: %.2f KB\nCompressed size: %.2f KB\nCompression ratio: %.2f"),
            originalSize / 1024.0, compressedSize / 1024.0, ratio);
        AfxMessageBox(message);
    }

    return true;
}

// LZW解码
bool CImageProc::LZWDecodeImage(const CString& openPath)
{
    // 打开文件
    std::ifstream inFile(openPath, std::ios::binary);
    if (!inFile) {
        AfxMessageBox(_T("Failed to open input file!"));
        return false;
    }

    // 读取并验证文件头
    char header[4] = { 0 };
    if (!inFile.read(header, 3) || inFile.gcount() != 3) {
        AfxMessageBox(_T("File Read Error"));
        inFile.close();
        return false;
    }

    if (strcmp(header, "LZW") != 0) {
        AfxMessageBox(_T("Not a valid LZW encoded file!"));
        inFile.close();
        return false;
    }

    // 读取图像信息
    int width, height, bitCount;
    inFile.read(reinterpret_cast<char*>(&width), sizeof(width));
    inFile.read(reinterpret_cast<char*>(&height), sizeof(height));
    inFile.read(reinterpret_cast<char*>(&bitCount), sizeof(bitCount));
    
    // 读取16位图像格式标志
    bool is565Format = false;
    if (bitCount == 16) {
        BYTE formatFlag;
        inFile.read(reinterpret_cast<char*>(&formatFlag), sizeof(formatFlag));
        is565Format = (formatFlag == 1);
        m_bIs565Format = is565Format;
    }
    
    // 读取颜色表信息
    int paletteSize = 0;
    inFile.read(reinterpret_cast<char*>(&paletteSize), sizeof(paletteSize));
    
    // 创建颜色表数据缓存
    std::vector<RGBQUAD> palette;
    if (paletteSize > 0) {
        palette.resize(paletteSize);
        for (int i = 0; i < paletteSize; i++) {
            inFile.read(reinterpret_cast<char*>(&palette[i]), sizeof(RGBQUAD));
        }
    }

    // 验证图像参数
    if (width <= 0 || height <= 0 ||
        (bitCount != 1 && bitCount != 4 && bitCount != 8 &&
            bitCount != 16 && bitCount != 24 && bitCount != 32)) {
        AfxMessageBox(_T("Invalid image parameters"));
        inFile.close();
        return false;
    }

    // 读取编码数据长度
    int codeCount;
    if (!inFile.read(reinterpret_cast<char*>(&codeCount), sizeof(codeCount)) || codeCount <= 0) {
        AfxMessageBox(_T("Invalid encoded data length"));
        inFile.close();
        return false;
    }

    // 读取压缩数据
    std::vector<BYTE> compressedData;
    BYTE byte;
    while (inFile.get(reinterpret_cast<char&>(byte))) {
        compressedData.push_back(byte);
    }
    inFile.close();

    // 从压缩数据中提取编码
    std::vector<int> inputCodes;
    inputCodes.reserve(codeCount);

    int bitBuffer = 0; // 用于累积字节的整数缓冲区
    int bitCountBuffer = 0; // bitBuffer中有效位的数量

	// 将压缩的字节流重组为12位编码
    for (BYTE b : compressedData) {
        bitBuffer |= (b << bitCountBuffer); // 将字节添加到缓冲区
        bitCountBuffer += 8; // 更新缓冲区中的有效位数

        while (bitCountBuffer >= 12) {
            int code = bitBuffer & 0xFFF; // 提取低十二位作为编码
			inputCodes.push_back(code); // 存储编码

            if (inputCodes.size() >= codeCount) {
				break; // 达到预期的编码数量时停止
            }

			bitBuffer >>= 12; // 移除已处理的12位
			bitCountBuffer -= 12; // 更新缓冲区中的有效位数
        }
    }

    if (inputCodes.empty()) {
        AfxMessageBox(_T("Decoded data is null"));
        return false;
    }

    // LZW解码
    std::vector<BYTE> outputData; // 存储最终压缩结果
    outputData.reserve(width * height * (bitCount / 8 + 1)); // 确保足够空间
    std::unordered_map<int, std::vector<BYTE>> dictionary;

    //-读取下一个编码值
    //    - 查找字典，获取对应的字符串：
    //    - 如果编码在字典中：直接获取对应的字符串
    //    - 如果编码不在字典中（特殊情况）：
    //    - 这种情况只会在编码过程中遇到形如"ww"的模式时发生
    //    - 解决方法：将前一个字符串加上前一个字符串的第一个字符作为当前字符串
    //    - 输出当前字符串
    //    - 将前一个字符串加上当前字符串的第一个字符添加到字典中
    //    - 设置前一个字符串为当前字符串
    //    - 重复上述过程，直到处理完所有编码值

    // 初始化字典 (0-255为单字节值)
	int nextCode = 256; // 下一个待分配的编码号
    for (int i = 0; i < 256; i++) {
        dictionary[i] = { static_cast<BYTE>(i) };
    }

    // 处理第一个编码
    int oldCode = inputCodes[0]; // 上一个处理的编码值，用于构建新的字典条目
    if (oldCode >= 256) {
        AfxMessageBox(_T("The first code is invalid"));
        return false;
    }

    std::vector<BYTE> output = dictionary[oldCode]; // 临时变量，存储当前解码得到的字节序列
    outputData.insert(outputData.end(), output.begin(), output.end());
    BYTE character = output[0]; // 该序列的第一个字节 用于构建新字典条目的字符

    // 处理剩余编码
    for (size_t i = 1; i < inputCodes.size(); i++) {
		int code = inputCodes[i]; // 当前编码

        std::vector<BYTE> sequence; // 当前编码对应的字节序列
        if (dictionary.find(code) != dictionary.end()) {
            // 编码在字典中
            sequence = dictionary[code]; 
        }
        else if (code == nextCode) {
            // 特殊情况：编码不在字典中，但是可以推导
            sequence = dictionary[oldCode];
            sequence.push_back(character);
        }
        else {
            // 无效编码
            CString errMsg;
            errMsg.Format(_T("Decoding error: Invalid code %d at position %d!"), code, i);
            AfxMessageBox(errMsg);
            return false;
        }

        // 输出解码序列
        outputData.insert(outputData.end(), sequence.begin(), sequence.end());

        // 更新字典
        if (!sequence.empty()) {
            character = sequence[0];
            if (nextCode < 4096) { // 限制字典大小为12位
                std::vector<BYTE> newEntry = dictionary[oldCode];
                newEntry.push_back(character);
                dictionary[nextCode++] = newEntry;
            }
        }

        oldCode = code;
    }

    // 清理现有图像资源
    CleanUp();

    // 创建新的DIB
    int bytesPerPixel = (bitCount + 7) / 8; // 向上取整
    int alignedWidth = ((width * bitCount + 31) / 32) * 4; // 确保每行是4字节的倍数
    int imageSize = alignedWidth * height;

    // 计算DIB所需的总内存大小
    DWORD dibSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    if (bitCount <= 8) {
        dibSize += (1 << bitCount) * sizeof(RGBQUAD); // 颜色表
    }
    dibSize += imageSize;

    // 分配内存
    m_hDib = GlobalAlloc(GHND, dibSize);
    if (!m_hDib) {
        AfxMessageBox(_T("Memory allocation failed!"));
        return false;
    }

    pDib = (BYTE*)GlobalLock(m_hDib);
    if (!pDib) {
        AfxMessageBox(_T("Memory lock failed!"));
        GlobalFree(m_hDib);
        m_hDib = NULL;
        return false;
    }

    // 设置文件头
    pBFH = (BITMAPFILEHEADER*)pDib;
    pBFH->bfType = 0x4D42; // 'BM'
    pBFH->bfSize = dibSize;
    pBFH->bfReserved1 = 0;
    pBFH->bfReserved2 = 0;
    pBFH->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    // 对于16位565格式图像，需要添加位掩码
    if (bitCount == 16 && is565Format) {
        pBFH->bfOffBits += 3 * sizeof(DWORD); // 增加三个掩码的大小
    }
    
    if (bitCount <= 8) {
        pBFH->bfOffBits += (1 << bitCount) * sizeof(RGBQUAD);
    }

    // 设置信息头
    pBIH = (BITMAPINFOHEADER*)(pDib + sizeof(BITMAPFILEHEADER));
    pBIH->biSize = sizeof(BITMAPINFOHEADER);
    pBIH->biWidth = width;
    pBIH->biHeight = height;
    pBIH->biPlanes = 1;
    pBIH->biBitCount = bitCount;
    
    // 对于16位565格式图像，设置正确的压缩类型
    if (bitCount == 16 && is565Format) {
        pBIH->biCompression = BI_BITFIELDS;
        
        // 添加掩码信息
        DWORD* masks = (DWORD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        masks[0] = 0xF800; // R掩码
        masks[1] = 0x07E0; // G掩码
        masks[2] = 0x001F; // B掩码
    } else {
        pBIH->biCompression = BI_RGB;
    }
    
    pBIH->biSizeImage = imageSize;
    pBIH->biXPelsPerMeter = 0;
    pBIH->biYPelsPerMeter = 0;
    pBIH->biClrUsed = 0;
    pBIH->biClrImportant = 0;

    // 设置颜色表 (如果需要)
    if (bitCount <= 8) {
        pQUAD = (RGBQUAD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        
        if (paletteSize > 0) {
            // 使用保存的颜色表
            for (int i = 0; i < paletteSize && i < (1 << bitCount); i++) {
                pQUAD[i] = palette[i];
            }
        } else {
            // 如果没有保存颜色表，创建默认灰度表
            for (int i = 0; i < (1 << bitCount); i++) {
                pQUAD[i].rgbBlue = i;
                pQUAD[i].rgbGreen = i;
                pQUAD[i].rgbRed = i;
                pQUAD[i].rgbReserved = 0;
            }
        }
    }

    // 设置图像数据
    pBits = pDib + pBFH->bfOffBits;

    // 清空图像数据区域
    memset(pBits, 0, imageSize);

    // 填充图像数据 - 修正像素填充方式
    size_t dataIndex = 0;
    
    // 计算每行实际需要的字节数（不考虑对齐）
    int rowBytes = (width * bitCount + 7) / 8;
    // 计算每行需要填充的字节数
    int paddingBytes = alignedWidth - rowBytes;

    for (int y = 0; y < height; y++) {
        // 获取当前行的起始位置 - BMP图像是从下到上存储的
        BYTE* rowStart = pBits + (height - 1 - y) * alignedWidth;
        
        // 处理每一行的像素数据
        for (int x = 0; x < width && dataIndex < outputData.size(); ) {
            switch (bitCount) {
            case 1: {
                // 1位图像 - 8个像素占1个字节
                BYTE pixelByte = 0;
                for (int bit = 7; bit >= 0 && x < width; bit--, x++) {
                    if (dataIndex < outputData.size()) {
                        // 如果像素值大于0，则设置对应位
                        if (outputData[dataIndex++] > 0) {
                            pixelByte |= (1 << bit);
                        }
                    }
                }
                *rowStart++ = pixelByte;
                break;
            }
            case 4: {
				// 4位图像 - 2个像素占1个字节 高四位第一个像素 第四位第二个像素
                BYTE pixelByte = 0;
                for (int nibble = 1; nibble >= 0 && x < width; nibble--, x++) {
                    if (dataIndex < outputData.size()) {
                        BYTE pixelValue = outputData[dataIndex++] & 0x0F; // 确保只取低4位
                        pixelByte |= (pixelValue << (nibble * 4));
                    }
                }
                *rowStart++ = pixelByte;
                break;
            }
            case 8:
                // 8位图像 - 1个像素占1个字节
                if (dataIndex < outputData.size()) {
                    *rowStart++ = outputData[dataIndex++];
                    x++;
                }
                break;
            case 16: {
                // 16位图像 - 1个像素占2个字节
                if (dataIndex + 1 < outputData.size()) {
                    // 低字节在前，高字节在后
                    *rowStart++ = outputData[dataIndex++]; // 低字节
                    *rowStart++ = outputData[dataIndex++]; // 高字节
                    x++;
                }
                else if (dataIndex < outputData.size()) {
                    // 如果只剩一个字节，填充低字节，高字节置0
                    *rowStart++ = outputData[dataIndex++];
                    *rowStart++ = 0;
                    x++;
                }
                break;
            }
            case 24: {
                // 24位图像 - 1个像素占3个字节 (BGR顺序)
                if (dataIndex + 2 < outputData.size()) {
                    // 确保BGR顺序正确
                    *rowStart++ = outputData[dataIndex++]; // B
                    *rowStart++ = outputData[dataIndex++]; // G
                    *rowStart++ = outputData[dataIndex++]; // R
                    x++;
                }
                else {
                    // 数据不足，填充剩余字节
                    for (int i = 0; i < 3 && dataIndex < outputData.size(); i++) {
                        *rowStart++ = outputData[dataIndex++];
                    }
                    // 如果还有未填充的字节，填0
                    while (rowStart < pBits + (height - 1 - y) * alignedWidth + (x + 1) * 3) {
                        *rowStart++ = 0;
                    }
                    x++;
                }
                break;
            }
            case 32: {
                // 32位图像 - 1个像素占4个字节 (BGRA顺序)
                if (dataIndex + 3 < outputData.size()) {
                    // 确保BGRA顺序正确
                    *rowStart++ = outputData[dataIndex++]; // B
                    *rowStart++ = outputData[dataIndex++]; // G
                    *rowStart++ = outputData[dataIndex++]; // R
                    *rowStart++ = outputData[dataIndex++]; // A
                    x++;
                }
                else {
                    // 数据不足，填充剩余字节
                    for (int i = 0; i < 4 && dataIndex < outputData.size(); i++) {
                        *rowStart++ = outputData[dataIndex++];
                    }
                    // 如果还有未填充的字节，填0
                    while (rowStart < pBits + (height - 1 - y) * alignedWidth + (x + 1) * 4) {
                        *rowStart++ = 0;
                    }
                    x++;
                }
                break;
            }
            }
        }
        
        // 添加行填充字节（确保每行是4字节的倍数）
        for (int p = 0; p < paddingBytes; p++) {
            *rowStart++ = 0;
        }
    }

    // 更新图像参数
    nWidth = width;
    nHeight = height;
    nBitCount = bitCount;

    // 重置FFT状态
    ResetFFTState();

    AfxMessageBox(_T("LZW decoding complete!"));
    return true;
}

// 量化表（JPEG标准量化表）
const int quantizationTable[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

// DCT变换函数
void CImageProc::DCT2D(double block[8][8]) {
    // 临时存储DCT结果
    double temp[8][8] = { 0 };
    const double PI = 3.14159265358979323846;

    // 对每一行进行DCT
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            double sum = 0.0;
            double cj = (j == 0) ? 1.0 / sqrt(2.0) : 1.0;
            
            for (int k = 0; k < 8; k++) {
                double cosVal = cos((2.0 * k + 1.0) * j * PI / 16.0);
                sum += block[i][k] * cosVal;
            }
            
            temp[i][j] = cj * sum / 2.0;
        }
    }

    // 对每一列进行DCT
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            double sum = 0.0;
            double ci = (i == 0) ? 1.0 / sqrt(2.0) : 1.0;
            
            for (int k = 0; k < 8; k++) {
                double cosVal = cos((2.0 * k + 1.0) * i * PI / 16.0);
                sum += temp[k][j] * cosVal;
            }
            
            block[i][j] = ci * sum / 2.0;
        }
    }
}

// 逆DCT变换函数
void CImageProc::IDCT2D(double block[8][8]) {
    // 临时存储IDCT结果
    double temp[8][8] = { 0 };
    const double PI = 3.14159265358979323846;

    // 对每一行进行IDCT
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            double sum = 0.0;
            
            for (int k = 0; k < 8; k++) {
                double ck = (k == 0) ? 1.0 / sqrt(2.0) : 1.0;
                double cosVal = cos((2.0 * j + 1.0) * k * PI / 16.0);
                sum += ck * block[i][k] * cosVal;
            }
            
            temp[i][j] = sum / 2.0;
        }
    }

    // 对每一列进行IDCT
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            double sum = 0.0;
            
            for (int k = 0; k < 8; k++) {
                double ck = (k == 0) ? 1.0 / sqrt(2.0) : 1.0;
                double cosVal = cos((2.0 * i + 1.0) * k * PI / 16.0);
                sum += ck * temp[k][j] * cosVal;
            }
            
            block[i][j] = sum / 2.0;
        }
    }
}

// 量化函数 - 使用更保守的量化表
void CImageProc::Quantize(double block[8][8]) {
    // 使用更保守的量化表
    static const int quantTable[8][8] = {
        {2, 2, 2, 2, 3, 4, 5, 6},
        {2, 2, 2, 2, 3, 4, 5, 6},
        {2, 2, 2, 3, 4, 5, 7, 7},
        {2, 2, 3, 4, 5, 7, 8, 8},
        {3, 3, 4, 5, 7, 9, 10, 10},
        {4, 4, 5, 7, 9, 11, 12, 12},
        {5, 5, 7, 8, 10, 12, 14, 14},
        {6, 6, 7, 8, 10, 12, 14, 16}
    };

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            block[i][j] = round(block[i][j] / quantTable[i][j]);
        }
    }
}

// 反量化函数
void CImageProc::Dequantize(double block[8][8]) {
    // 使用与量化相同的表
    static const int quantTable[8][8] = {
        {2, 2, 2, 2, 3, 4, 5, 6},
        {2, 2, 2, 2, 3, 4, 5, 6},
        {2, 2, 2, 3, 4, 5, 7, 7},
        {2, 2, 3, 4, 5, 7, 8, 8},
        {3, 3, 4, 5, 7, 9, 10, 10},
        {4, 4, 5, 7, 9, 11, 12, 12},
        {5, 5, 7, 8, 10, 12, 14, 14},
        {6, 6, 7, 8, 10, 12, 14, 16}
    };

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            block[i][j] *= quantTable[i][j];
        }
    }
}

// DCT编码函数
bool CImageProc::CosineEncodeImage(const CString& savePath) {
    if (!IsValid()) return false;

    // 打开文件进行写入
    FILE* fp = nullptr;
    errno_t err = _tfopen_s(&fp, savePath, _T("wb"));
    if (err != 0 || fp == nullptr) return false;

    // 写入图像基本信息
    fwrite(&nWidth, sizeof(int), 1, fp);
    fwrite(&nHeight, sizeof(int), 1, fp);
    fwrite(&nBitCount, sizeof(int), 1, fp);
    // 保存16位图像格式标志
    if (nBitCount == 16) {
        BYTE formatFlag = m_bIs565Format ? 1 : 0;
        fwrite(&formatFlag, sizeof(BYTE), 1, fp);
    }

    // 如果是8位图像，保存调色板
    if (nBitCount == 8 && pQUAD) {
        fwrite(pQUAD, sizeof(RGBQUAD), 256, fp);
    }
    // 处理灰度图像或彩色图像
    if (nBitCount == 8) {
        // 灰度图像处理
        int alignedWidth = GetAlignedWidthBytes();
        
        // 写入调色板
        if (pQUAD) {
            fwrite(pQUAD, sizeof(RGBQUAD), 256, fp);
        }

        // 创建重叠块处理，减少块边界效应
        // 按8x8块处理图像，但使用重叠区域
        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 提取8x8块
                double block[8][8] = { 0 };
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            // 直接获取灰度值（索引值）
                            BYTE* pixel = GetPixelPtr(x + j, y + i);
                            // 对于8位灰度图像，直接使用像素值作为灰度值
                            block[i][j] = static_cast<double>(*pixel) - 128.0;
                        }
                    }
                }

                // 应用DCT变换
                DCT2D(block);
                
                // 量化
                Quantize(block);
                
                // 写入量化后的DCT系数
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coef = static_cast<short>(block[i][j]);
                        fwrite(&coef, sizeof(short), 1, fp);
                    }
                }
            }
        }
    }
    else if (nBitCount == 16) {
        // 分块处理图像
        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 创建三个8x8块，分别存储R、G、B通道
                double blockR[8][8] = { 0 };
                double blockG[8][8] = { 0 };
                double blockB[8][8] = { 0 };

                // 填充块
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        int py = y + i;
                        int px = x + j;

                        // 边界检查
                        if (py >= nHeight || px >= nWidth) continue;

                        // 获取像素颜色
                        BYTE* pixel = GetPixelPtr(px, py);
                        BYTE r, g, b;
                        GetColor16bit(pixel, r, g, b);

                        // 中心化: 将[0,255]映射到[-128,127]
                        blockR[i][j] = static_cast<double>(r) - 128.0;
                        blockG[i][j] = static_cast<double>(g) - 128.0;
                        blockB[i][j] = static_cast<double>(b) - 128.0;
                    }
                }

                // 对每个通道应用DCT变换
                DCT2D(blockR);
                DCT2D(blockG);
                DCT2D(blockB);

                // 量化
                Quantize(blockR);
                Quantize(blockG);
                Quantize(blockB);

                // 写入量化后的DCT系数
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coefR = static_cast<short>(round(blockR[i][j]));
                        short coefG = static_cast<short>(round(blockG[i][j]));
                        short coefB = static_cast<short>(round(blockB[i][j]));
                        fwrite(&coefR, sizeof(short), 1, fp);
                        fwrite(&coefG, sizeof(short), 1, fp);
                        fwrite(&coefB, sizeof(short), 1, fp);
                    }
                }
            }
        }
    }else if (nBitCount == 24 || nBitCount == 32) {
        // 彩色图像处理 - 分别处理RGB三个通道
        int alignedWidth = GetAlignedWidthBytes();
        
        // 按8x8块处理图像
        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 为RGB三个通道分别创建8x8块
                double blockR[8][8] = { 0 };
                double blockG[8][8] = { 0 };
                double blockB[8][8] = { 0 };
                
                // 提取RGB值
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            BYTE r, g, b;
                            GetColor(x + j, y + i, r, g, b);
                            blockR[i][j] = static_cast<double>(r) - 128.0;
                            blockG[i][j] = static_cast<double>(g) - 128.0;
                            blockB[i][j] = static_cast<double>(b) - 128.0;
                        }
                    }
                }
                
                // 对RGB三个通道分别应用DCT和量化
                DCT2D(blockR);
                DCT2D(blockG);
                DCT2D(blockB);
                
                Quantize(blockR);
                Quantize(blockG);
                Quantize(blockB);
                
                // 写入量化后的DCT系数
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coefR = static_cast<short>(blockR[i][j]);
                        short coefG = static_cast<short>(blockG[i][j]);
                        short coefB = static_cast<short>(blockB[i][j]);
                        fwrite(&coefR, sizeof(short), 1, fp);
                        fwrite(&coefG, sizeof(short), 1, fp);
                        fwrite(&coefB, sizeof(short), 1, fp);
                    }
                }
            }
        }
    }
    else {
        fclose(fp);
        return false; // 不支持的位深度
    }

    fclose(fp);
    return true;
}

// DCT解码函数
bool CImageProc::CosineDecodeImage(const CString& openPath) {
    // 打开文件
    FILE* fp = nullptr;
    errno_t err = _tfopen_s(&fp, openPath, _T("rb"));
    if (err != 0 || fp == nullptr) return false;

    // 读取图像基本信息
    int width, height, bitCount;
    fread(&width, sizeof(int), 1, fp);
    fread(&height, sizeof(int), 1, fp);
    fread(&bitCount, sizeof(int), 1, fp);

    // 读取16位图像格式标志
    bool is565Format = false;
    if (bitCount == 16) {
        BYTE formatFlag = 0;
        fread(&formatFlag, sizeof(BYTE), 1, fp);
        is565Format = (formatFlag == 1);
        m_bIs565Format = is565Format;
    }

    // 读取调色板（如果有）
    RGBQUAD palette[256];
    if (bitCount == 8) {
        fread(palette, sizeof(RGBQUAD), 256, fp);
    }

    // 清理现有资源
    CleanUp();

    // 计算每行字节数（4字节对齐）
    int rowSize = ((width * bitCount + 31) / 32) * 4;
    
    // 计算DIB大小
    DWORD dibSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    // 对于16位565格式图像，需要添加位掩码
    if (bitCount == 16 && is565Format) {
        dibSize += 3 * sizeof(DWORD); // 增加三个掩码的大小
    }
    
    // 添加调色板大小（如果需要）
    if (bitCount <= 8) {
        dibSize += (1 << bitCount) * sizeof(RGBQUAD);
    }
    
    // 添加像素数据大小
    dibSize += rowSize * height;

    // 分配内存
    m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, dibSize);
    if (!m_hDib) return false;
    pDib = (BYTE*)::GlobalLock(m_hDib);
    if (!pDib) {
        ::GlobalFree(m_hDib);
        m_hDib = NULL;
        return false;
    }

    // 设置文件头
    pBFH = (BITMAPFILEHEADER*)pDib;
    pBFH->bfType = 0x4D42; // 'BM'
    pBFH->bfSize = dibSize;
    pBFH->bfReserved1 = 0;
    pBFH->bfReserved2 = 0;
    pBFH->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    // 对于16位565格式图像，需要添加位掩码
    if (bitCount == 16 && is565Format) {
        pBFH->bfOffBits += 3 * sizeof(DWORD); // 增加三个掩码的大小
    }
    
    // 添加调色板偏移（如果需要）
    if (bitCount <= 8) {
        pBFH->bfOffBits += (1 << bitCount) * sizeof(RGBQUAD);
    }

    // 设置信息头
    pBIH = (BITMAPINFOHEADER*)(pDib + sizeof(BITMAPFILEHEADER));
    pBIH->biSize = sizeof(BITMAPINFOHEADER);
    pBIH->biWidth = width;
    pBIH->biHeight = height;
    pBIH->biPlanes = 1;
    pBIH->biBitCount = bitCount;
    
    // 对于16位565格式图像，设置正确的压缩类型和位掩码
    if (bitCount == 16 && is565Format) {
        pBIH->biCompression = BI_BITFIELDS;
        
        // 添加掩码信息
        DWORD* masks = (DWORD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        masks[0] = 0xF800; // R掩码
        masks[1] = 0x07E0; // G掩码
        masks[2] = 0x001F; // B掩码
    } else {
        pBIH->biCompression = BI_RGB;
    }
    
    pBIH->biSizeImage = rowSize * height;
    pBIH->biXPelsPerMeter = 0;
    pBIH->biYPelsPerMeter = 0;
    pBIH->biClrUsed = 0;
    pBIH->biClrImportant = 0;

    // 设置调色板（如果需要）
    if (bitCount == 8) {
        pQUAD = (RGBQUAD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        memcpy(pQUAD, palette, 256 * sizeof(RGBQUAD));
    }
    else {
        pQUAD = nullptr;
    }

    // 设置像素数据指针
    pBits = pDib + pBFH->bfOffBits;

    // 更新图像参数
    nWidth = width;
    nHeight = height;
    nBitCount = bitCount;

    // 创建临时缓冲区用于平滑处理
    std::vector<std::vector<double>> tempBuffer;
    std::vector<std::vector<int>> blockCount;
    
    if (nBitCount == 8) {
        // 初始化临时缓冲区和计数器
        tempBuffer.resize(nHeight, std::vector<double>(nWidth, 0.0));
        blockCount.resize(nHeight, std::vector<int>(nWidth, 0));
        
        // 灰度图像处理
        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 读取量化后的DCT系数
                double block[8][8] = { 0 };
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coef;
                        if (fread(&coef, sizeof(short), 1, fp) != 1) {
                            // 文件读取错误
                            CleanUp();
                            fclose(fp);
                            return false;
                        }
                        block[i][j] = static_cast<double>(coef);
                    }
                }
                
                // 反量化
                Dequantize(block);
                
                // 应用IDCT变换
                IDCT2D(block);
                
                // 将结果累加到临时缓冲区
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            // 反中心化
                            double value = block[i][j] + 128.0;
                            tempBuffer[y + i][x + j] += value;
                            blockCount[y + i][x + j]++;
                        }
                    }
                }
            }
        }
        
        // 计算平均值并写入像素数据
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                if (blockCount[y][x] > 0) {
                    double avgValue = tempBuffer[y][x] / blockCount[y][x];
                    int value = static_cast<int>(round(avgValue));
                    value = max(0, min(255, value));
                    
                    // 设置像素值
                    BYTE* pixel = GetPixelPtr(x, y);
                    *pixel = static_cast<BYTE>(value);
                }
            }
        }
    }
    else if (nBitCount == 16) {
        // 16位图像处理
        std::vector<std::vector<double>> tempBufferR(nHeight, std::vector<double>(nWidth, 0.0));
        std::vector<std::vector<double>> tempBufferG(nHeight, std::vector<double>(nWidth, 0.0));
        std::vector<std::vector<double>> tempBufferB(nHeight, std::vector<double>(nWidth, 0.0));
        std::vector<std::vector<int>> blockCount(nHeight, std::vector<int>(nWidth, 0));
        
        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 读取量化后的DCT系数
                double blockR[8][8] = { 0 };
                double blockG[8][8] = { 0 };
                double blockB[8][8] = { 0 };
                
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coefR, coefG, coefB;
                        if (fread(&coefR, sizeof(short), 1, fp) != 1 ||
                            fread(&coefG, sizeof(short), 1, fp) != 1 ||
                            fread(&coefB, sizeof(short), 1, fp) != 1) {
                            // 文件读取错误
                            CleanUp();
                            fclose(fp);
                            return false;
                        }
                        blockR[i][j] = static_cast<double>(coefR);
                        blockG[i][j] = static_cast<double>(coefG);
                        blockB[i][j] = static_cast<double>(coefB);
                    }
                }
                
                // 反量化
                Dequantize(blockR);
                Dequantize(blockG);
                Dequantize(blockB);
                
                // 应用IDCT变换
                IDCT2D(blockR);
                IDCT2D(blockG);
                IDCT2D(blockB);
                
                // 将结果累加到临时缓冲区
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            // 反中心化
                            tempBufferR[y + i][x + j] += blockR[i][j] + 128.0;
                            tempBufferG[y + i][x + j] += blockG[i][j] + 128.0;
                            tempBufferB[y + i][x + j] += blockB[i][j] + 128.0;
                            blockCount[y + i][x + j]++;
                        }
                    }
                }
            }
        }
        
        // 计算平均值并写入像素数据
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                if (blockCount[y][x] > 0) {
                    int r = static_cast<int>(round(tempBufferR[y][x] / blockCount[y][x]));
                    int g = static_cast<int>(round(tempBufferG[y][x] / blockCount[y][x]));
                    int b = static_cast<int>(round(tempBufferB[y][x] / blockCount[y][x]));
                    
                    r = max(0, min(255, r));
                    g = max(0, min(255, g));
                    b = max(0, min(255, b));
                    
                    // 设置16位像素颜色
                    BYTE* pixel = GetPixelPtr(x, y);
                    WORD pixelValue;
                    
                    if (is565Format) {
                        // RGB565格式
                        BYTE r5 = (r >> 3) & 0x1F;  // 5位红色
                        BYTE g6 = (g >> 2) & 0x3F;  // 6位绿色
                        BYTE b5 = (b >> 3) & 0x1F;  // 5位蓝色
                        pixelValue = (r5 << 11) | (g6 << 5) | b5;
                    } else {
                        // RGB555格式
                        BYTE r5 = (r >> 3) & 0x1F;  // 5位红色
                        BYTE g5 = (g >> 3) & 0x1F;  // 5位绿色
                        BYTE b5 = (b >> 3) & 0x1F;  // 5位蓝色
                        pixelValue = (r5 << 10) | (g5 << 5) | b5;
                    }
                    
                    *reinterpret_cast<WORD*>(pixel) = pixelValue;
                }
            }
        }
    }else if (nBitCount == 24 || nBitCount == 32) {
        // 彩色图像处理
        std::vector<std::vector<double>> tempBufferR(nHeight, std::vector<double>(nWidth, 0.0));
        std::vector<std::vector<double>> tempBufferG(nHeight, std::vector<double>(nWidth, 0.0));
        std::vector<std::vector<double>> tempBufferB(nHeight, std::vector<double>(nWidth, 0.0));
        std::vector<std::vector<int>> blockCount(nHeight, std::vector<int>(nWidth, 0));
        
        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 读取量化后的DCT系数
                double blockR[8][8] = { 0 };
                double blockG[8][8] = { 0 };
                double blockB[8][8] = { 0 };
                
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coefR, coefG, coefB;
                        if (fread(&coefR, sizeof(short), 1, fp) != 1 ||
                            fread(&coefG, sizeof(short), 1, fp) != 1 ||
                            fread(&coefB, sizeof(short), 1, fp) != 1) {
                            // 文件读取错误
                            CleanUp();
                            fclose(fp);
                            return false;
                        }
                        blockR[i][j] = static_cast<double>(coefR);
                        blockG[i][j] = static_cast<double>(coefG);
                        blockB[i][j] = static_cast<double>(coefB);
                    }
                }
                
                // 反量化
                Dequantize(blockR);
                Dequantize(blockG);
                Dequantize(blockB);
                
                // 应用IDCT变换
                IDCT2D(blockR);
                IDCT2D(blockG);
                IDCT2D(blockB);
                
                // 将结果累加到临时缓冲区
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            // 反中心化
                            tempBufferR[y + i][x + j] += blockR[i][j] + 128.0;
                            tempBufferG[y + i][x + j] += blockG[i][j] + 128.0;
                            tempBufferB[y + i][x + j] += blockB[i][j] + 128.0;
                            blockCount[y + i][x + j]++;
                        }
                    }
                }
            }
        }
        
        // 计算平均值并写入像素数据
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                if (blockCount[y][x] > 0) {
                    int r = static_cast<int>(round(tempBufferR[y][x] / blockCount[y][x]));
                    int g = static_cast<int>(round(tempBufferG[y][x] / blockCount[y][x]));
                    int b = static_cast<int>(round(tempBufferB[y][x] / blockCount[y][x]));
                    
                    r = max(0, min(255, r));
                    g = max(0, min(255, g));
                    b = max(0, min(255, b));
                    
                    // 设置像素颜色
                    BYTE* pixel = GetPixelPtr(x, y);
                    SetColor(pixel, x, y, r, g, b);
                }
            }
        }
    }
    else {
        // 不支持的位深度
        CleanUp();
        fclose(fp);
        return false;
    }

    fclose(fp);
    return true;
}
// 综合编码函数
bool CImageProc::ComprehensiveEncodeImage(const CString& savePath) {
    if (!IsValid()) return false;

    // 备份原始图像参数
    int originalWidth = nWidth;
    int originalHeight = nHeight;
    int originalBitCount = nBitCount;

    // 打开临时文件进行写入
    CString tempPath = savePath + _T(".temp");
    FILE* fp = nullptr;
    errno_t err = _tfopen_s(&fp, tempPath, _T("wb"));
    if (err != 0 || fp == nullptr) return false;

    // 写入图像基本信息
    fwrite(&nWidth, sizeof(int), 1, fp);
    fwrite(&nHeight, sizeof(int), 1, fp);
    fwrite(&nBitCount, sizeof(int), 1, fp);

    // 保存16位图像格式标志
    if (nBitCount == 16) {
        BYTE formatFlag = m_bIs565Format ? 1 : 0;
        fwrite(&formatFlag, sizeof(BYTE), 1, fp);
    }

    // 8位图像，保存调色板
    if (nBitCount == 8 && pQUAD) {
        fwrite(pQUAD, sizeof(RGBQUAD), 256, fp);
    }
    
    std::vector<short> allCoefficients;//存储量化后的DCT系数

    // 分8x8块处理图像数据
    for (int y = 0; y < nHeight; y += 8) {
        for (int x = 0; x < nWidth; x += 8) {
            if (nBitCount == 8) {
                // 灰度图像处理
                double block[8][8] = { 0 };
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            BYTE* pixel = GetPixelPtr(x + j, y + i);
                            block[i][j] = static_cast<double>(*pixel) - 128.0;
                        }
                    }
                }

                // 应用DCT变换
                DCT2D(block);

                // 量化
                Quantize(block, luminanceQuantTable);

                // 收集量化后的DCT系数
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coef = static_cast<short>(block[i][j]);
                        allCoefficients.push_back(coef);
                    }
                }
            }
            else if (nBitCount == 16) {
                // 16位图像处理：转换为YUV空间
                double blockY[8][8] = { 0 };
                double blockU[8][8] = { 0 };
                double blockV[8][8] = { 0 };

                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        int py = y + i;
                        int px = x + j;
                        if (py >= nHeight || px >= nWidth) continue;

                        BYTE* pixel = GetPixelPtr(px, py);
                        BYTE r, g, b;
                        GetColor16bit(pixel, r, g, b); // 提取16位RGB分量

                        // RGB转YUV（BT.601标准，适用于8-16位精度）
                        double Y = 0.299 * r + 0.587 * g + 0.114 * b;
                        double U = -0.147 * r - 0.289 * g + 0.436 * b;
                        double V = 0.615 * r - 0.515 * g - 0.100 * b;

                        blockY[i][j] = Y - 128.0; // Y通道中心化
                        blockU[i][j] = U;         // U/V通道无需中心化（范围更窄）
                        blockV[i][j] = V;
                    }
                }

                // 对YUV通道分别应用DCT变换
                DCT2D(blockY);
                DCT2D(blockU);
                DCT2D(blockV);

                // 差异化量化：Y用亮度表，U/V用色度表
                Quantize(blockY, luminanceQuantTable);
                Quantize(blockU, chrominanceQuantTable);
                Quantize(blockV, chrominanceQuantTable);

                // 收集系数（YUV顺序）
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        // 四舍五入转换为short（量化后系数可能为小数）
                        short coefY = static_cast<short>(round(blockY[i][j]));
                        short coefU = static_cast<short>(round(blockU[i][j]));
                        short coefV = static_cast<short>(round(blockV[i][j]));
                        allCoefficients.push_back(coefY);
                        allCoefficients.push_back(coefU);
                        allCoefficients.push_back(coefV);
                    }
                }
            }
            else if (nBitCount == 24 || nBitCount == 32) {
                // 彩色图像处理 - 转换为YUV空间
                double blockY[8][8] = { 0 };
                double blockU[8][8] = { 0 };
                double blockV[8][8] = { 0 };

                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            BYTE r, g, b;
                            GetColor(x + j, y + i, r, g, b);

                            // RGB转YUV (BT.601标准)
                            blockY[i][j] = 0.299 * r + 0.587 * g + 0.114 * b - 128.0;
                            blockU[i][j] = -0.147 * r - 0.289 * g + 0.436 * b;
                            blockV[i][j] = 0.615 * r - 0.515 * g - 0.100 * b;
                        }
                    }
                }

                // 对每个通道应用DCT变换
                DCT2D(blockY);
                DCT2D(blockU);
                DCT2D(blockV);

                // 使用不同量化表进行量化
                Quantize(blockY, luminanceQuantTable);
                Quantize(blockU, chrominanceQuantTable);
                Quantize(blockV, chrominanceQuantTable);

                // 收集量化后的DCT系数（YUV顺序）
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        short coefY = static_cast<short>(round(blockY[i][j]));
                        short coefU = static_cast<short>(round(blockU[i][j]));
                        short coefV = static_cast<short>(round(blockV[i][j]));
                        allCoefficients.push_back(coefY);
                        allCoefficients.push_back(coefU);
                        allCoefficients.push_back(coefV);
                    }
                }
            }
        }
    }

    // 霍夫曼编码
    //存储系数的字节流
    std::vector<BYTE> byteData;
    for (short coef : allCoefficients) {
        //将short型系数转换为字节数组
        BYTE* ptr = reinterpret_cast<BYTE*>(&coef);
        byteData.insert(byteData.end(), ptr, ptr + sizeof(short));
    }
    pBits = byteData.data();
    nWidth = 1;
    //临时设置为1，因霍夫曼编码处理一维字节流
    nHeight = byteData.size();
    nBitCount = 8;

    // 打开另一个临时文件用于霍夫曼编码
    CString huffmanTempPath = savePath + _T(".huffman_temp");
    if (!HuffmanEncodeImage(huffmanTempPath)) {
        fclose(fp);
        _tremove(tempPath);
        return false;
    }

    // 追加霍夫曼编码后的数据到临时文件
    std::ifstream huffmanIfs(CW2A(huffmanTempPath), std::ios::binary);
    if (!huffmanIfs) {
        fclose(fp);
        _tremove(tempPath);
        _tremove(huffmanTempPath);
        return false;
    }
    char buffer[1024];
    while (huffmanIfs.read(buffer, sizeof(buffer))) {
        fwrite(buffer, 1, sizeof(buffer), fp);
    }
    if (huffmanIfs.gcount() > 0) {
        fwrite(buffer, 1, huffmanIfs.gcount(), fp);
    }
    huffmanIfs.close();

    fclose(fp);

    // 删除霍夫曼编码临时文件
    _tremove(huffmanTempPath);

    // 重命名临时文件为最终文件
    if (_trename(tempPath, savePath) != 0) {
        _tremove(tempPath);
        return false;
    }

    // 计算压缩率
    double originalSize = originalWidth * originalHeight * (originalBitCount / 8.0); // 原始大小（字节）
    struct _stat fileStat;
    if (_stat(CW2A(savePath), &fileStat) == 0) {
        double compressedSize = fileStat.st_size;
        double ratio = ( originalSize / compressedSize);
        if (ratio < 0) ratio = 0; // 确保压缩率非负
        CString message;
        message.Format(_T("Comprehensive encoding completed!\nOriginal size: %.2f KB\nCompressed size: %.2f KB\nCompression ratio: %.2f"),
            originalSize / 1024.0, compressedSize / 1024.0, ratio);
        AfxMessageBox(message);
    }

    return true;
}

// 综合解码函数
bool CImageProc::ComprehensiveDecodeImage(const CString& openPath) {
    // 读取图像基本信息
    FILE* fp = nullptr;
    errno_t err = _tfopen_s(&fp, openPath, _T("rb"));
    if (err != 0 || fp == nullptr) return false;

    int width, height, bitCount;
    fread(&width, sizeof(int), 1, fp);
    fread(&height, sizeof(int), 1, fp);
    fread(&bitCount, sizeof(int), 1, fp);

    // 读取16位图像格式标志
    bool is565Format = false;
    if (bitCount == 16) {
        BYTE formatFlag = 0;
        fread(&formatFlag, sizeof(BYTE), 1, fp);
        is565Format = (formatFlag == 1);
        m_bIs565Format = is565Format;
    }

    // 读取调色板（如果有）
    RGBQUAD palette[256];
    if (bitCount == 8) {
        fread(palette, sizeof(RGBQUAD), 256, fp);
    }

    // 保存当前文件指针位置
    long currentPos = ftell(fp);

    // 关闭文件
    fclose(fp);

    // 创建一个临时文件用于霍夫曼解码
    CString huffmanTempPath = openPath + _T(".huffman_temp");
    std::ifstream ifs(CW2A(openPath), std::ios::binary);
    if (!ifs) return false;
    std::ofstream ofs(CW2A(huffmanTempPath), std::ios::binary);
    if (!ofs) return false;

    // 跳过文件头
    ifs.seekg(currentPos, std::ios::beg);

    // 复制霍夫曼编码数据到临时文件
    char buffer[1024];
    while (ifs.read(buffer, sizeof(buffer))) {
        ofs.write(buffer, sizeof(buffer));
    }
    if (ifs.gcount() > 0) {
        ofs.write(buffer, ifs.gcount());
    }
    ifs.close();
    ofs.close();

    // 霍夫曼解码
    pBits = nullptr;
    nWidth = 1;
    nHeight = 0;
    nBitCount = 8;
    if (!HuffmanDecodeImage(huffmanTempPath)) {
        _tremove(huffmanTempPath);
        return false;
    }

    // 删除霍夫曼解码临时文件
    _tremove(huffmanTempPath);

    std::vector<short> allCoefficients;
    for (size_t i = 0; i < nWidth * nHeight; i += sizeof(short)) {
        short coef = *reinterpret_cast<short*>(pBits + i);
        allCoefficients.push_back(coef);
    }

    CleanUp();

    // 计算每行字节数（4字节对齐）
    int rowSize = ((width * bitCount + 31) / 32) * 4;

    // 计算DIB大小
    DWORD dibSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // 对于16位565格式图像，需要添加位掩码
    if (bitCount == 16 && is565Format) {
        dibSize += 3 * sizeof(DWORD); // 增加三个掩码的大小
    }

    // 添加调色板大小
    if (bitCount <= 8) {
        dibSize += (1 << bitCount) * sizeof(RGBQUAD);
    }

    // 添加像素数据大小
    dibSize += rowSize * height;

    // 分配内存
    m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, dibSize);
    if (!m_hDib) return false;
    pDib = (BYTE*)::GlobalLock(m_hDib);
    if (!pDib) {
        ::GlobalFree(m_hDib);
        m_hDib = NULL;
        return false;
    }

    // 设置文件头
    pBFH = (BITMAPFILEHEADER*)pDib;
    pBFH->bfType = 0x4D42; // 'BM'
    pBFH->bfSize = dibSize;
    pBFH->bfReserved1 = 0;
    pBFH->bfReserved2 = 0;
    pBFH->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // 对于16位565格式图像，需要添加位掩码
    if (bitCount == 16 && is565Format) {
        pBFH->bfOffBits += 3 * sizeof(DWORD); // 增加三个掩码的大小
    }

    // 添加调色板偏移
    if (bitCount <= 8) {
        pBFH->bfOffBits += (1 << bitCount) * sizeof(RGBQUAD);
    }

    // 设置信息头
    pBIH = (BITMAPINFOHEADER*)(pDib + sizeof(BITMAPFILEHEADER));
    pBIH->biSize = sizeof(BITMAPINFOHEADER);
    pBIH->biWidth = width;
    pBIH->biHeight = height;
    pBIH->biPlanes = 1;
    pBIH->biBitCount = bitCount;

    // 对于16位565格式图像，设置正确的压缩类型和位掩码
    if (bitCount == 16 && is565Format) {
        pBIH->biCompression = BI_BITFIELDS;

        // 添加掩码信息
        DWORD* masks = (DWORD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        masks[0] = 0xF800; // R掩码
        masks[1] = 0x07E0; // G掩码
        masks[2] = 0x001F; // B掩码
    }
    else {
        pBIH->biCompression = BI_RGB;
    }

    pBIH->biSizeImage = rowSize * height;
    pBIH->biXPelsPerMeter = 0;
    pBIH->biYPelsPerMeter = 0;
    pBIH->biClrUsed = 0;
    pBIH->biClrImportant = 0;

    // 设置调色板
    if (bitCount == 8) {
        pQUAD = (RGBQUAD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        memcpy(pQUAD, palette, 256 * sizeof(RGBQUAD));
    }
    else {
        pQUAD = nullptr;
    }

    // 设置像素数据指针
    pBits = pDib + pBFH->bfOffBits;

    // 更新图像参数
    nWidth = width;
    nHeight = height;
    nBitCount = bitCount;

    // 创建临时缓冲区用于平滑处理
    std::vector<std::vector<double>> tempBufferR;
    std::vector<std::vector<double>> tempBufferG;
    std::vector<std::vector<double>> tempBufferB;
    std::vector<std::vector<int>> blockCount;

    size_t coefIndex = 0;

    if (nBitCount == 8) {
        // 灰度图像处理
        tempBufferR.resize(nHeight, std::vector<double>(nWidth, 0.0));
        blockCount.resize(nHeight, std::vector<int>(nWidth, 0));

        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 读取量化后的DCT系数
                double block[8][8] = { 0 };
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        block[i][j] = static_cast<double>(allCoefficients[coefIndex++]);
                    }
                }

                // 反量化
                Dequantize(block, luminanceQuantTable);

                // 应用IDCT变换
                IDCT2D(block);

                // 将结果累加到临时缓冲区
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            // 反中心化
                            double value = block[i][j] + 128.0;
                            tempBufferR[y + i][x + j] += value;
                            blockCount[y + i][x + j]++;
                        }
                    }
                }
            }
        }

        // 计算平均值并写入像素数据
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                if (blockCount[y][x] > 0) {
                    double avgValue = tempBufferR[y][x] / blockCount[y][x];
                    int value = static_cast<int>(round(avgValue));
                    value = max(0, min(255, value));

                    // 设置像素值
                    BYTE* pixel = GetPixelPtr(x, y);
                    *pixel = static_cast<BYTE>(value);
                }
            }
        }
    }
    else if (nBitCount == 16) {
        // 16位图像处理：从YUV转回RGB
        tempBufferR.resize(nHeight, std::vector<double>(nWidth, 0.0));
        tempBufferG.resize(nHeight, std::vector<double>(nWidth, 0.0));
        tempBufferB.resize(nHeight, std::vector<double>(nWidth, 0.0));
        blockCount.resize(nHeight, std::vector<int>(nWidth, 0));

        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                double blockY[8][8] = { 0 };
                double blockU[8][8] = { 0 };
                double blockV[8][8] = { 0 };

                // 读取YUV系数（顺序与编码时一致）
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        blockY[i][j] = static_cast<double>(allCoefficients[coefIndex++]);
                        blockU[i][j] = static_cast<double>(allCoefficients[coefIndex++]);
                        blockV[i][j] = static_cast<double>(allCoefficients[coefIndex++]);
                    }
                }

                // 反量化（使用对应量化表）
                Dequantize(blockY, luminanceQuantTable);
                Dequantize(blockU, chrominanceQuantTable);
                Dequantize(blockV, chrominanceQuantTable);

                // 应用IDCT变换
                IDCT2D(blockY);
                IDCT2D(blockU);
                IDCT2D(blockV);

                // YUV转RGB并累加至缓冲区
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            // 反中心化Y通道
                            double Y = blockY[i][j] + 128.0;
                            double U = blockU[i][j];
                            double V = blockV[i][j];

                            // YUV转RGB公式（浮点运算，需限制范围）
                            int r = static_cast<int>(Y + 1.402 * V);
                            int g = static_cast<int>(Y - 0.344 * U - 0.714 * V);
                            int b = static_cast<int>(Y + 1.772 * U);

                            // 像素值限制在[0, 255]
                            r = max(0, min(255, r));
                            g = max(0, min(255, g));
                            b = max(0, min(255, b));

                            tempBufferR[y + i][x + j] += r;
                            tempBufferG[y + i][x + j] += g;
                            tempBufferB[y + i][x + j] += b;
                            blockCount[y + i][x + j]++;
                        }
                    }
                }
            }
        }

        // 计算平均值并转换为16位像素
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                if (blockCount[y][x] > 0) {
                    int r = static_cast<int>(tempBufferR[y][x] / blockCount[y][x]);
                    int g = static_cast<int>(tempBufferG[y][x] / blockCount[y][x]);
                    int b = static_cast<int>(tempBufferB[y][x] / blockCount[y][x]);

                    // 转换为16位格式（RGB565/RGB555）
                    BYTE* pixel = GetPixelPtr(x, y);
                    WORD pixelValue;
                    if (is565Format) {
                        // RGB565
                        pixelValue = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                    }
                    else {
                        // RGB555（最高位为0）
                        pixelValue = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
                    }
                    *reinterpret_cast<WORD*>(pixel) = pixelValue;
                }
            }
        }
    }
    else if (nBitCount == 24 || nBitCount == 32) {
        // 彩色图像处理 - 从YUV转回RGB
        tempBufferR.resize(nHeight, std::vector<double>(nWidth, 0.0));
        tempBufferG.resize(nHeight, std::vector<double>(nWidth, 0.0));
        tempBufferB.resize(nHeight, std::vector<double>(nWidth, 0.0));
        blockCount.resize(nHeight, std::vector<int>(nWidth, 0));

        for (int y = 0; y < nHeight; y += 8) {
            for (int x = 0; x < nWidth; x += 8) {
                // 读取量化后的DCT系数（YUV顺序）
                double blockY[8][8] = { 0 };
                double blockU[8][8] = { 0 };
                double blockV[8][8] = { 0 };

                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        blockY[i][j] = static_cast<double>(allCoefficients[coefIndex++]);
                        blockU[i][j] = static_cast<double>(allCoefficients[coefIndex++]);
                        blockV[i][j] = static_cast<double>(allCoefficients[coefIndex++]);
                    }
                }

                // 反量化（使用对应量化表）
                Dequantize(blockY, luminanceQuantTable);
                Dequantize(blockU, chrominanceQuantTable);
                Dequantize(blockV, chrominanceQuantTable);

                IDCT2D(blockY);
                IDCT2D(blockU);
                IDCT2D(blockV);

                // 将结果累加到临时缓冲区（转换回RGB）
                for (int i = 0; i < 8; i++) {
                    for (int j = 0; j < 8; j++) {
                        if (y + i < nHeight && x + j < nWidth) {
                            // 反中心化
                            double Y = blockY[i][j] + 128.0;
                            double U = blockU[i][j];
                            double V = blockV[i][j];

                            // YUV转RGB (BT.601标准)
                            int r = static_cast<int>(Y + 1.402 * V);
                            int g = static_cast<int>(Y - 0.344 * U - 0.714 * V);
                            int b = static_cast<int>(Y + 1.772 * U);

                            // 像素值范围限制
                            r = max(0, min(255, r));
                            g = max(0, min(255, g));
                            b = max(0, min(255, b));

                            tempBufferR[y + i][x + j] += r;
                            tempBufferG[y + i][x + j] += g;
                            tempBufferB[y + i][x + j] += b;
                            blockCount[y + i][x + j]++;
                        }
                    }
                }
            }
        }

        // 计算平均值并写入像素数据
        for (int y = 0; y < nHeight; y++) {
            for (int x = 0; x < nWidth; x++) {
                if (blockCount[y][x] > 0) {
                    int r = static_cast<int>(tempBufferR[y][x] / blockCount[y][x]);
                    int g = static_cast<int>(tempBufferG[y][x] / blockCount[y][x]);
                    int b = static_cast<int>(tempBufferB[y][x] / blockCount[y][x]);

                    r = max(0, min(255, r));
                    g = max(0, min(255, g));
                    b = max(0, min(255, b));

                    // 设置像素颜色
                    BYTE* pixel = GetPixelPtr(x, y);
                    SetColor(pixel, x, y, r, g, b);
                }
            }
        }
    }
    else {
        // 不支持的位深度
        CleanUp();
        return false;
    }

    // 释放内存
    ::GlobalUnlock(m_hDib);

    return true;
}

// 量化表定义
const double CImageProc::luminanceQuantTable[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

const double CImageProc::chrominanceQuantTable[8][8] = {
    {24, 24, 32, 64, 99, 99, 99, 99},
    {24, 24, 32, 64, 99, 99, 99, 99},
    {32, 32, 64, 99, 99, 99, 99, 99},
    {64, 64, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}
};

// 辅助函数：量化
void CImageProc::Quantize(double block[8][8], const double quantTable[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            block[i][j] = round(block[i][j] / quantTable[i][j]);
        }
    }
}

// 辅助函数：反量化
void CImageProc::Dequantize(double block[8][8], const double quantTable[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            block[i][j] = block[i][j] * quantTable[i][j];
        }
    }
}

//行程编码
bool CImageProc::RLEncodeImage(const CString& savePath) {
    if (!IsValid()) {
        AfxMessageBox(_T("没有有效的图像可编码"));
        return false;
    }

    // 计算像素数据大小
    if (nBitCount != 8 && nBitCount != 16 && nBitCount != 24 && nBitCount != 32) {
        AfxMessageBox(_T("8,16,24,32bit only"));
        return false;
    }
    int bytesPerPixel = nBitCount / 8;
    int dataSize = nWidth * nHeight * bytesPerPixel;
    BYTE* data = pBits;

    std::ofstream ofs(CW2A(savePath), std::ios::binary);
    if (!ofs) {
        AfxMessageBox(_T("无法打开保存文件"));
        return false;
    }

    // 写文件头（宽、高、位深）
    ofs.write((char*)&nWidth, sizeof(nWidth));
    ofs.write((char*)&nHeight, sizeof(nHeight));
    ofs.write((char*)&nBitCount, sizeof(nBitCount));

    //16位565处理
    if (nBitCount == 16) {
        BYTE is565 = m_bIs565Format ? 1 : 0;
        ofs.write((char*)&is565, sizeof(is565));
    }

    //行程编码数据
    std::vector<BYTE> imageDataNoPadding;
    imageDataNoPadding.reserve(nWidth * nHeight * (nBitCount / 8)); // 预分配空间

    int unpaddedRowSizeBytes = nWidth * (nBitCount / 8);
    int paddedRowSizeBytes = ((nWidth * nBitCount + 31) / 32) * 4;

    for (int y = 0; y < nHeight; ++y) {
        // pBits 指向DIB的像素数据，通常是底向顶存储
        // 第y行（从图像底部开始，y=0是图像最底行）
        BYTE* currentRowStartInBuffer = pBits + y * paddedRowSizeBytes;
        imageDataNoPadding.insert(imageDataNoPadding.end(), currentRowStartInBuffer, currentRowStartInBuffer + unpaddedRowSizeBytes);
    }

    if (!imageDataNoPadding.empty()) {
        BYTE currentValue = imageDataNoPadding[0];
        BYTE count = 1;
        for (size_t i = 1; i < imageDataNoPadding.size(); ++i) {
            if (imageDataNoPadding[i] == currentValue && count < 255) {
                count++;
            }
            else {
                ofs.write((char*)&count, 1);
                ofs.write((char*)&currentValue, 1);
                currentValue = imageDataNoPadding[i];
                count = 1;
            }
        }
        // 写入最后一个行程
        ofs.write((char*)&count, 1);
        ofs.write((char*)&currentValue, 1);
    }

    //写调色板数据
    if (nBitCount == 8 && pQUAD) {
        // 直接写入原始调色板，不进行顺序判断和修改
        ofs.write((char*)pQUAD, 256 * sizeof(RGBQUAD));
    }

    ofs.close();
    return true;
}

//行程解码
bool CImageProc::RLDecodeImage(const CString& openPath) {
    std::ifstream ifs(CW2A(openPath), std::ios::binary);
    if (!ifs) {
        AfxMessageBox(_T("无法打开解码文件"));
        return false;
    }

    // 2. 读取文件头
    int width = 0, height = 0, bitCount = 0;
    ifs.read((char*)&width, sizeof(width));
    ifs.read((char*)&height, sizeof(height));
    ifs.read((char*)&bitCount, sizeof(bitCount));

    //16位565处理
    bool is16Bit565Format = false;
    if (bitCount == 16) {
        BYTE is565_byte = 0;
        if (!ifs.read((char*)&is565_byte, sizeof(is565_byte))) {
            AfxMessageBox(_T("读取失败"));
            ifs.close();
            return false;
        }
        is16Bit565Format = (is565_byte == 1);
    }
    if (width <= 0 || height <= 0 || (bitCount != 8 && bitCount != 16 && bitCount != 24 && bitCount != 32)) {
        AfxMessageBox(_T("ERROR"));
        ifs.close();
        return false;
    }

    int bytePerPixel = bitCount / 8;
    int dataSizeUncompressed = width * height * bytePerPixel;
    int rowSizePadded = ((width * bitCount + 31) / 32) * 4;

    std::vector<BYTE> decodedData;
    if (dataSizeUncompressed > 0) {
        decodedData.reserve(dataSizeUncompressed);//预分配解压后数据容量
    }
    // 3. 读取编码数据并解码
    BYTE count = 0, value = 0;
    while (decodedData.size() < (size_t)dataSizeUncompressed && ifs.read((char*)&count, 1) && ifs.read((char*)&value, 1)) {
        for (int i = 0; i < count && decodedData.size() < (size_t)dataSizeUncompressed; ++i) {
            decodedData.push_back(value);
        }
    }

    if (decodedData.size() != (size_t)dataSizeUncompressed && dataSizeUncompressed > 0) {
        CString msg;
        msg.Format(_T("解码数据长度不符, 预期: %d, 实际: %d"), dataSizeUncompressed, decodedData.size());
        AfxMessageBox(msg);
        ifs.close();
        return false;
    }

    // 4. 构建BMP内存结构
    CleanUp();
    DWORD paletteSize = (bitCount == 8) ? (256 * sizeof(RGBQUAD)) : 0;
    DWORD colorMaskSize = (bitCount == 16) ? (3 * sizeof(DWORD)) : 0;
    DWORD imageActualDataSizeInDib = (DWORD)rowSizePadded * height;
    DWORD dibSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paletteSize + colorMaskSize + imageActualDataSizeInDib;


    m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, dibSize);
    if (!m_hDib) {
        AfxMessageBox(_T("内存分配失败"));
        ifs.close();
        return false;
    }
    pDib = (BYTE*)::GlobalLock(m_hDib);
    pBFH = (BITMAPFILEHEADER*)pDib;
    pBIH = (BITMAPINFOHEADER*)(pDib + sizeof(BITMAPFILEHEADER));
    nWidth = width;
    nHeight = height;
    nBitCount = bitCount;
    if (bitCount == 16) {
        m_bIs565Format = is16Bit565Format;
    }

    // 填写文件头
    pBFH->bfType = 0x4D42; //  BM
    pBFH->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paletteSize + colorMaskSize;
    pBFH->bfSize = dibSize;
    pBFH->bfReserved1 = 0;
    pBFH->bfReserved2 = 0;

    // 填写信息头
    pBIH->biSize = sizeof(BITMAPINFOHEADER);
    pBIH->biWidth = width;
    pBIH->biHeight = height;
    pBIH->biPlanes = 1;
    pBIH->biBitCount = bitCount;
    if (bitCount == 16) {
        pBIH->biCompression = BI_BITFIELDS; // 16位图像使用BI_BITFIELDS
    }
    else {
        pBIH->biCompression = BI_RGB; // 其他情况（如8, 24, 32位）使用BI_RGB
    }
    pBIH->biSizeImage = imageActualDataSizeInDib;
    pBIH->biXPelsPerMeter = 0;
    pBIH->biYPelsPerMeter = 0;
    pBIH->biClrUsed = (bitCount == 8) ? 256 : 0;
    pBIH->biClrImportant = (bitCount == 8) ? 256 : 0;


    // 填写调色板

    if (bitCount == 8) {
        pQUAD = (RGBQUAD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        if (!ifs.read((char*)pQUAD, 256 * sizeof(RGBQUAD))) {
            AfxMessageBox(_T("ERROR"));
            CleanUp();
            ifs.close();
            return false;
        }

    }
    else {
        pQUAD = nullptr;
    }

    // 填写颜色掩码 (仅针对16位图像)
    if (bitCount == 16) {
        DWORD* pColorMasks = (DWORD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
        if (is16Bit565Format) { // 565 format
            pColorMasks[0] = 0xF800; // Red mask
            pColorMasks[1] = 0x07E0; // Green mask
            pColorMasks[2] = 0x001F; // Blue mask
        }
        else { // 555 format
            pColorMasks[0] = 0x7C00; // Red mask
            pColorMasks[1] = 0x03E0; // Green mask
            pColorMasks[2] = 0x001F; // Blue mask
        }
    }

    // 填写像素数据
    pBits = pDib + pBFH->bfOffBits;
    int unpaddedRowBytes = width * bytePerPixel;

    for (int y = 0; y < height; ++y) {
        BYTE* destRow = pBits + y * rowSizePadded; // BMP中最后一行在文件最前面，即倒序写入
        BYTE* srcRow = decodedData.data() + y * unpaddedRowBytes;
        memcpy(destRow, srcRow, unpaddedRowBytes);
    }

    ifs.close();
    return true;
}