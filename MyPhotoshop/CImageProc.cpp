#include "pch.h"
#include "CImageProc.h"
#include <afxdlgs.h>
#include <vector>


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
    if (m_hDib) {
        if (pDib) {
            GlobalUnlock(m_hDib);
        }
        GlobalFree(m_hDib);
    }
    m_hDib = NULL;
    pDib = NULL;
    pBFH = NULL;
    pBIH = NULL;
    pQUAD = NULL;
    pBits = NULL;
    nWidth = nHeight = nBitCount = 0;
    m_bIs565Format = true;
}
void CImageProc::OpenFile()
{
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
    CleanUp();

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
        return;
    }

    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;

    float bytePerPixel = float(nBitCount) / 8;

    int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);

    BYTE* pixel = pBits + offset;

    BYTE red = 0, green = 0, blue = 0;

    switch (nBitCount)
    {
    case 1:
        CImageProc::GetColor1bit(pixel, red, green, blue, x, y, pDC);
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
        return;
    }

    COLORREF pixelColor = pDC->GetPixel(x, y);
    BYTE getPixelRed = GetRValue(pixelColor);
    BYTE getPixelGreen = GetGValue(pixelColor);
    BYTE getPixelBlue = GetBValue(pixelColor);

    pDC->SetBkMode(OPAQUE);

    pDC->SetTextColor(RGB(0, 0, 0));

    CString str;
    str.Format(L"RGB: (%d, %d, %d)", red, green, blue);

    CString getPixelStr;
    getPixelStr.Format(L"GetPixel RGB: (%d, %d, %d)", getPixelRed, getPixelGreen, getPixelBlue);

    CString location;
    location.Format(L"location:(%d, %d)", x, y);

    pDC->TextOutW(x, y, str);

    CSize textSize = pDC->GetTextExtent(str);

    pDC->TextOutW(x, y + textSize.cy, getPixelStr);

    pDC->TextOutW(x, y + textSize.cy * 2, location);
}

void CImageProc::GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC)
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
    if (m_bIs565Format) {

        red = (pixelValue & 0xF800) >> 11;    
        green = (pixelValue & 0x07E0) >> 5;   
        blue = pixelValue & 0x001F;           


        red = (red << 3) | (red >> 2);        
        green = (green << 2) | (green >> 4);  
        blue = (blue << 3) | (blue >> 2);    
    } else {
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


std::vector<int> CImageProc::CalculateHistogramMix()
{
    std::vector<int> histogram(256, 0);

    if (m_hDib == NULL)
    {
        return histogram;
    }


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


            int gray = static_cast<int>(0.299 * red + 0.587 * green + 0.114 * blue);
            histogram[gray]++;
        }
    }

    return histogram;
}


std::vector<std::vector<int>> CImageProc::CalculateHistogramRGB()
{
    std::vector<std::vector<int>> histograms(3, std::vector<int>(256, 0));

    if (m_hDib == NULL)
    {
        return histograms;
    }

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


            histograms[0][red]++;
            histograms[1][green]++;
            histograms[2][blue]++;
        }
    }

    return histograms;
}



void CImageProc::ApplyVintageStyle()
{
    if (!IsValid()) return;

    if (nBitCount <= 8) {       
        ApplyVintageToPalette();
    }
    else if (nBitCount == 16) {  
        ApplyVintageTo16Bit();
    }
    else {                     
        ApplyVintageToTrueColor();
    }
}

void CImageProc::ApplyVintageToPalette()
{
    if (!pQUAD) return;


    CreateVintagePalette();


    if (nBitCount == 1) {

        for (int i = 0; i < 2; i++) {
            BYTE gray = static_cast<BYTE>(0.299 * pQUAD[i].rgbRed +
                0.587 * pQUAD[i].rgbGreen +
                0.114 * pQUAD[i].rgbBlue);
            pQUAD[i] = {
                static_cast<BYTE>(gray * 0.7),  
                static_cast<BYTE>(gray * 0.6),
                static_cast<BYTE>(gray * 0.4),
                0
            };
        }
    }
}


void CImageProc::ApplyVintageTo16Bit()
{
    if (!IsValid() || nBitCount != 16) return;

    int rowSize = ((nWidth * 16 + 31) / 32) * 4;

    for (int y = 0; y < nHeight; y++) {
        BYTE* pPixel = pBits + (nHeight - 1 - y) * rowSize;

        for (int x = 0; x < nWidth; x++) {
            WORD* pixel = reinterpret_cast<WORD*>(&pPixel[x * 2]);
            WORD oldPixel = *pixel;


            BYTE r, g, b;
            if (m_bIs565Format) {

                r = (oldPixel & 0xF800) >> 11 ;
                g = (oldPixel & 0x07E0) >> 5;  
                b = oldPixel         & 0x001F;  
                

                r = (r << 3) | (r >> 2);  
                g = (g << 2) | (g >> 4);  
                b = (b << 3) | (b >> 2);
            } else {

                r = (oldPixel & 0x7C00) >> 10;
                g = (oldPixel & 0x03E0) >> 5;
                b = oldPixel & 0x001F;
                

                r = (r << 3) | (r >> 2);
                g = (g << 3) | (g >> 2);
                b = (b << 3) | (b >> 2);
            }


            int newR = min(255, static_cast<int>(r * 1.15 + 15));
            int newG = min(255, static_cast<int>(g * 0.85 + 5));
            int newB = min(255, static_cast<int>(b * 0.7));


            int noise = (rand() % 7) - 3;
            newR = max(0, min(255, newR + noise));
            newG = max(0, min(255, newG + noise));
            newB = max(0, min(255, newB + noise));


            if (m_bIs565Format) {
               
                *pixel = ((newR >> 3) << 11) | 
                         ((newG >> 2) << 5)  | 
                         (newB >> 3);
            } else {

                *pixel = ((newR >> 3) << 10) | 
                         ((newG >> 3) << 5)  | 
                         (newB >> 3);
            }
        }
    }
}


void CImageProc::ApplyVintageToTrueColor()
{
    if (!IsValid() || (nBitCount != 24 && nBitCount != 32)) return;

    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
    int bytesPerPixel = nBitCount / 8; 

    for (int y = 0; y < nHeight; y++) {
        BYTE* pPixel = pBits + (nHeight - 1 - y) * rowSize;

        for (int x = 0; x < nWidth; x++) {

            BYTE* pixel = &pPixel[x * bytesPerPixel];


            BYTE& blue = pixel[0];
            BYTE& green = pixel[1];
            BYTE& red = pixel[2];

            int newRed = min(255, static_cast<int>(red * 1.15));
            int newGreen = min(255, static_cast<int>(green * 0.85));
            int newBlue = min(255, static_cast<int>(blue * 0.7));

            newRed = min(255, newRed + 25);
            newGreen = min(255, newGreen + 15);

            int noise = (rand() % 11) - 5; 
            newRed = max(0, min(255, newRed + noise));
            newGreen = max(0, min(255, newGreen + noise));
            newBlue = max(0, min(255, newBlue + noise));

            newRed = static_cast<int>(newRed * 0.95 + 10);
            newGreen = static_cast<int>(newGreen * 0.95 + 5);
            newBlue = static_cast<int>(newBlue * 0.95);


            red = static_cast<BYTE>(newRed);
            green = static_cast<BYTE>(newGreen);
            blue = static_cast<BYTE>(newBlue);

        }
    }
}


void CImageProc::CreateVintagePalette()
{
    if (nBitCount > 8) return;

    int paletteSize = 1 << nBitCount;
    RGBQUAD* newPalette = new RGBQUAD[paletteSize];


    for (int i = 0; i < paletteSize; i++) {
        BYTE r = pQUAD[i].rgbRed;
        BYTE g = pQUAD[i].rgbGreen;
        BYTE b = pQUAD[i].rgbBlue;

        newPalette[i].rgbRed = min(255, static_cast<int>(r * 1.1 + 15));
        newPalette[i].rgbGreen = min(255, static_cast<int>(g * 0.9 + 10));
        newPalette[i].rgbBlue = min(255, static_cast<int>(b * 0.8));
        newPalette[i].rgbReserved = 0;
    }


    memcpy(pQUAD, newPalette, paletteSize * sizeof(RGBQUAD));
    delete[] newPalette;
}



std::vector<std::vector<int>> CImageProc::Balance_Transformations(CClientDC& dc)
{
    std::vector<std::vector<int>> balancedRgbHistograms(3, std::vector<int>(256, 0)); 
    float p[256] = { 0 };
    float S[256] = { 0 };
    int F[256] = { 0 };
    std::vector<int> intensity_paint = CalculateHistogramMix();
    int w = nWidth;
    int h = nHeight;

    float pixel_count = static_cast<float>(w * h);
    for (int i = 0; i < 256; i++) {
        p[i] = intensity_paint[i] / pixel_count;
    }

    S[0] = 255.0f * p[0];
    for (int n = 1; n < 256; n++) {
        S[n] = 255.0f * p[n] + S[n - 1];
    }

    for (int j = 0; j < 256; j++) {
        F[j] = static_cast<int>(S[j] + 0.5f);
        F[j] = max(0, min(255, F[j]));
    }

    int rowSize = ((w * nBitCount + 31) / 32) * 4;
    float bytePerPixel = nBitCount / 8.0f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int offset = (h - 1 - y) * rowSize + static_cast<int>(x * bytePerPixel);
            BYTE* pixel = pBits + offset;

            BYTE red, green, blue;

            switch (nBitCount) {
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

            int Y = static_cast<int>(0.3f * red + 0.59f * green + 0.11f * blue + 0.5f);
            Y = max(0, min(255, Y));

            if (Y == 0) {
                dc.SetPixelV(x, y, RGB(0, 0, 0));
                balancedRgbHistograms[0][0]++;  
                balancedRgbHistograms[1][0]++;
                balancedRgbHistograms[2][0]++;
                continue;
            }

            float scale = F[Y] / static_cast<float>(Y);
            int new_r = static_cast<int>(red * scale + 0.5f);
            int new_g = static_cast<int>(green * scale + 0.5f);
            int new_b = static_cast<int>(blue * scale + 0.5f);

            new_r = max(0, min(255, new_r));
            new_g = max(0, min(255, new_g));
            new_b = max(0, min(255, new_b));


            balancedRgbHistograms[0][new_r]++;
            balancedRgbHistograms[1][new_g]++;
            balancedRgbHistograms[2][new_b]++;

            dc.SetPixelV(x, y, RGB(new_r, new_g, new_b));
        }
    }

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

    
    std::vector<RGBQUAD> grayPalette(256);
    for (int i = 0; i < 256; ++i)
    {
        grayPalette[i].rgbRed = static_cast<BYTE>(i);
        grayPalette[i].rgbGreen = static_cast<BYTE>(i);
        grayPalette[i].rgbBlue = static_cast<BYTE>(i);
        grayPalette[i].rgbReserved = 0;
    }


    if (nBitCount == 8)
    {
        std::vector<RGBQUAD> grayPalette8bit(256);
        for (int i = 0; i < 256; ++i)
        {
            grayPalette8bit[i].rgbRed = static_cast<BYTE>(255-i);
            grayPalette8bit[i].rgbGreen = static_cast<BYTE>(255-i);
            grayPalette8bit[i].rgbBlue = static_cast<BYTE>(255-i);
            grayPalette8bit[i].rgbReserved = 0;
        }
        memcpy(pQUAD, grayPalette8bit.data(), sizeof(RGBQUAD) * 256);

    }


    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;

    
    for (int y = 0; y < nHeight; ++y)
    {
        for (int x = 0; x < nWidth; ++x)
        {
            int offset = (nHeight - 1 - y) * rowSize + (x * (nBitCount / 8));
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
                AfxMessageBox(_T("Unsupported bit count."));
                return;
            }


            BYTE grayValue = static_cast<BYTE>((red * 0.299) + (green * 0.587) + (blue * 0.114));


            switch (nBitCount)
            {
            case 1:
            {
                BYTE bitIndex = x % 8;
                if (grayValue < 128)
                {
                    *pixel &= ~(1 << (7 - bitIndex));
                    
                }
                else
                {
                    *pixel |= (1 << (7 - bitIndex));
                }
                break;
            }
            case 4:
            {
                if (x % 2 == 0)
                {
                    *pixel = (*pixel & 0x0F) | ((grayValue >> 4) << 4);
                }
                else
                {
                    *pixel = (*pixel & 0xF0) | (grayValue & 0x0F);
                }
                break;
            }
            case 8:
                break;
            case 16:
            {
                if (m_bIs565Format)
                {
                    WORD newPixel = ((grayValue >> 3) << 11) | ((grayValue >> 2) << 5) | (grayValue >> 3);
                    *((WORD*)pixel) = newPixel;
                }
                else
                {
                    WORD newPixel = ((grayValue >> 3) << 10) | ((grayValue >> 3) << 5) | (grayValue >> 3);
                    *((WORD*)pixel) = newPixel;
                }
                break;
            }
            case 24:
                pixel[0] = grayValue;
                pixel[1] = grayValue;
                pixel[2] = grayValue;
                break;
            case 32:
                pixel[0] = grayValue;
                pixel[1] = grayValue;
                pixel[2] = grayValue;
                break;
            }
        }
    }
}