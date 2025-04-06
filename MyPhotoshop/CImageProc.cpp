#include "pch.h"
#include "CImageProc.h"
#include <afxdlgs.h>
#include <vector>

CImageProc::CImageProc()
{
    //m_hDib = NULL;
    //pDib = new BYTE;
    //pBFH = new BITMAPFILEHEADER;
    //pBIH = new BITMAPINFOHEADER;
    //pQUAD = new RGBQUAD;
    //pBits = new BYTE;
    //nWidth = 0;
    //nHeight = 0;
    //nBitCount = 0;
    //m_bIs565Format = true; // 默认假设为565格式
    InitializeMembers();
}
CImageProc::~CImageProc()
{
    //delete pDib;
    //delete pBFH;
    //delete pBIH;
    //delete pQUAD;
    //delete pBits;
    //if (m_hDib != NULL)
    //    GlobalUnlock(m_hDib);
    CleanUp();
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
    // 先清理现有数据
    CleanUp();

    CFile file;
    CFileException e;

    // 1. 尝试打开文件
    if (!file.Open(stFileName, CFile::modeRead | CFile::shareDenyWrite, &e))
    {
#ifdef _DEBUG
        afxDump << "File could not be opened " << e.m_cause << "\n";
#endif
        return;
    }

    // 2. 获取文件大小并验证最小尺寸
    ULONGLONG nFileSize = file.GetLength();
    if (nFileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
    {
        file.Close();
        return;
    }

    // 3. 分配全局内存
    m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, nFileSize);
    if (!m_hDib)
    {
        file.Close();
        return;
    }

    // 4. 锁定内存
    pDib = (BYTE*)::GlobalLock(m_hDib);
    if (!pDib)
    {
        GlobalFree(m_hDib);
        m_hDib = NULL;
        file.Close();
        return;
    }

    // 5. 读取文件内容
    UINT nBytesRead = file.Read(pDib, (UINT)nFileSize);
    file.Close();

    if (nBytesRead != nFileSize)
    {
        CleanUp();
        return;
    }

    // 6. 设置文件头指针并验证
    pBFH = (BITMAPFILEHEADER*)pDib;
    if (pBFH->bfType != 0x4D42) // 'BM'标志
    {
        CleanUp();
        return;
    }

    // 7. 设置信息头指针并验证
    pBIH = (BITMAPINFOHEADER*)&pDib[sizeof(BITMAPFILEHEADER)];
    if (pBIH->biSize < sizeof(BITMAPINFOHEADER))
    {
        CleanUp();
        return;
    }

    // 8. 设置调色板指针
    pQUAD = (RGBQUAD*)&pDib[sizeof(BITMAPFILEHEADER) + pBIH->biSize];

    // 9. 验证位图数据偏移量
    if (pBFH->bfOffBits >= nFileSize)
    {
        CleanUp();
        return;
    }
    pBits = &pDib[pBFH->bfOffBits];

    // 10. 设置基本图像参数
    nWidth = pBIH->biWidth;
    nHeight = abs(pBIH->biHeight); // 处理可能的倒向位图
    nBitCount = pBIH->biBitCount;

    // 11. 验证位图数据大小
    DWORD dwImageSize = ((nWidth * nBitCount + 31) / 32) * 4 * nHeight;
    if (pBFH->bfOffBits + dwImageSize > nFileSize)
    {
        CleanUp();
        return;
    }

    // 12. 处理16位位图格式
    if (pBIH->biCompression == BI_RGB && nBitCount == 16)
    {
        m_bIs565Format = false;
    }
    else if (pBIH->biCompression == BI_BITFIELDS && nBitCount == 16)
    {
        // 确保有足够的空间存放颜色掩码
        if (sizeof(BITMAPFILEHEADER) + pBIH->biSize + 3 * sizeof(DWORD) <= nFileSize)
        {
            DWORD* masks = reinterpret_cast<DWORD*>(&pDib[sizeof(BITMAPFILEHEADER) + pBIH->biSize]);
            DWORD redMask = masks[0];
            DWORD greenMask = masks[1];
            DWORD blueMask = masks[2];
            m_bIs565Format = (redMask == 0xF800 && greenMask == 0x07E0 && blueMask == 0x001F);
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

void CImageProc::InitializeMembers()
{
    m_hDib = NULL;
    pDib = NULL;
    pBFH = NULL;
    pBIH = NULL;
    pQUAD = NULL;
    pBits = NULL;
    nWidth = nHeight = nBitCount = 0;
    m_bIs565Format = true;
}

void CImageProc::CleanUp()
{
    if (m_hDib) {
        if (pDib) {
            GlobalUnlock(m_hDib);
        }
        GlobalFree(m_hDib);
    }
    InitializeMembers();
}

// 复古风格效果
void CImageProc::ApplyVintageStyle()
{
    if (!IsValid()) return;

    if (nBitCount <= 8) {        // 1/4/8位调色板图像
        ApplyVintageToPalette();
    }
    else if (nBitCount == 16) {  // 16位高彩色
        ApplyVintageTo16Bit();
    }
    else {                       // 24/32位真彩色
        ApplyVintageToTrueColor();
    }
}
//复古风格调色板图像处理(1/4/8位)
void CImageProc::ApplyVintageToPalette()
{
    if (!pQUAD) return;

    // 创建复古风格调色板
    CreateVintagePalette();

    // 对于1位图像需要特殊处理
    if (nBitCount == 1) {
        // 转换为灰度复古效果
        for (int i = 0; i < 2; i++) {
            BYTE gray = static_cast<BYTE>(0.299 * pQUAD[i].rgbRed +
                0.587 * pQUAD[i].rgbGreen +
                0.114 * pQUAD[i].rgbBlue);
            pQUAD[i] = {
                static_cast<BYTE>(gray * 0.7),  // 偏黄的灰度
                static_cast<BYTE>(gray * 0.6),
                static_cast<BYTE>(gray * 0.4),
                0
            };
        }
    }
}

//16位图像处理
void CImageProc::ApplyVintageTo16Bit()
{
    if (!IsValid() || nBitCount != 16) return;

    int rowSize = ((nWidth * 16 + 31) / 32) * 4;

    for (int y = 0; y < nHeight; y++) {
        BYTE* pPixel = pBits + (nHeight - 1 - y) * rowSize;

        for (int x = 0; x < nWidth; x++) {
            WORD* pixel = reinterpret_cast<WORD*>(&pPixel[x * 2]);
            WORD oldPixel = *pixel;

            // 转换为8位RGB分量（扩展精度）
            BYTE r, g, b;
            if (m_bIs565Format) {
                // 565格式转换：5-6-5 → 8-8-8
                r = (oldPixel & 0xF800) >> 11 ;  // 5位红色
                g = (oldPixel & 0x07E0) >> 5;  // 6位绿色
                b = oldPixel         & 0x001F;  // 5位蓝色
                
                // 扩展到8位（保持比例）
                r = (r << 3) | (r >> 2);  // 5→8位：复制高位到低位
                g = (g << 2) | (g >> 4);  // 6→8位：复制高位到低位
                b = (b << 3) | (b >> 2);
            } else {
                // 555格式转换：5-5-5 → 8-8-8
                r = (oldPixel & 0x7C00) >> 10;
                g = (oldPixel & 0x03E0) >> 5;
                b = oldPixel & 0x001F;
                
                // 扩展到8位
                r = (r << 3) | (r >> 2);
                g = (g << 3) | (g >> 2);
                b = (b << 3) | (b >> 2);
            }

            // 应用复古效果（使用8位精度计算）
            int newR = min(255, static_cast<int>(r * 1.15 + 15));
            int newG = min(255, static_cast<int>(g * 0.85 + 5));
            int newB = min(255, static_cast<int>(b * 0.7));

            // 添加噪点（±3范围，比24位图像小）
            int noise = (rand() % 7) - 3; // -3到3的随机数
            newR = max(0, min(255, newR + noise));
            newG = max(0, min(255, newG + noise));
            newB = max(0, min(255, newB + noise));

            // 转换回16位格式
            if (m_bIs565Format) {
                // 8→5位红色：取高5位 | 8→6位绿色：取高6位 | 8→5位蓝色
                *pixel = ((newR >> 3) << 11) | 
                         ((newG >> 2) << 5)  | 
                         (newB >> 3);
            } else {
                // 555格式：全部取高5位
                *pixel = ((newR >> 3) << 10) | 
                         ((newG >> 3) << 5)  | 
                         (newB >> 3);
            }
        }
    }
}

//复古风格真彩色处理(24/32位)
void CImageProc::ApplyVintageToTrueColor()
{
    if (!IsValid() || (nBitCount != 24 && nBitCount != 32)) return;

    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
    int bytesPerPixel = nBitCount / 8; // 3 for 24-bit, 4 for 32-bit

    for (int y = 0; y < nHeight; y++) {
        BYTE* pPixel = pBits + (nHeight - 1 - y) * rowSize;

        for (int x = 0; x < nWidth; x++) {
            // 获取像素位置（考虑32位可能有Alpha通道）
            BYTE* pixel = &pPixel[x * bytesPerPixel];

            // 读取颜色分量（BGR顺序）
            BYTE& blue = pixel[0];
            BYTE& green = pixel[1];
            BYTE& red = pixel[2];

            // 复古风格核心算法 -----------------------------------

            // 1. 基础色调调整（增强红/黄，减弱蓝）
            int newRed = min(255, static_cast<int>(red * 1.15));
            int newGreen = min(255, static_cast<int>(green * 0.85));
            int newBlue = min(255, static_cast<int>(blue * 0.7));

            // 2. 添加褐色色调偏移
            newRed = min(255, newRed + 25);
            newGreen = min(255, newGreen + 15);

            // 3. 添加老照片噪点（模拟胶片颗粒）
            int noise = (rand() % 11) - 5; // -5到5的随机数
            newRed = max(0, min(255, newRed + noise));
            newGreen = max(0, min(255, newGreen + noise));
            newBlue = max(0, min(255, newBlue + noise));

            // 4. 添加轻微褪色效果
            newRed = static_cast<int>(newRed * 0.95 + 10);
            newGreen = static_cast<int>(newGreen * 0.95 + 5);
            newBlue = static_cast<int>(newBlue * 0.95);

            // 写入处理后的颜色 -----------------------------------
            red = static_cast<BYTE>(newRed);
            green = static_cast<BYTE>(newGreen);
            blue = static_cast<BYTE>(newBlue);

            // 保持Alpha通道不变（如果是32位）
            // pixel[3] 保持不变
        }
    }
}

//调色板创建函数
void CImageProc::CreateVintagePalette()
{
    if (nBitCount > 8) return;

    int paletteSize = 1 << nBitCount;
    RGBQUAD* newPalette = new RGBQUAD[paletteSize];

    // 创建复古风格调色板
    for (int i = 0; i < paletteSize; i++) {
        BYTE r = pQUAD[i].rgbRed;
        BYTE g = pQUAD[i].rgbGreen;
        BYTE b = pQUAD[i].rgbBlue;

        // 复古效果算法
        newPalette[i].rgbRed = min(255, static_cast<int>(r * 1.1 + 15));
        newPalette[i].rgbGreen = min(255, static_cast<int>(g * 0.9 + 10));
        newPalette[i].rgbBlue = min(255, static_cast<int>(b * 0.8));
        newPalette[i].rgbReserved = 0;
    }

    // 应用新调色板
    memcpy(pQUAD, newPalette, paletteSize * sizeof(RGBQUAD));
    delete[] newPalette;
}

