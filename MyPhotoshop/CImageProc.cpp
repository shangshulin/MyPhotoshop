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
    m_bIs565Format = true; // Ĭ�ϼ���Ϊ565��ʽ
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

// ���ļ�
void CImageProc::OpenFile()
{
    // TODO: �ڴ˴�����ʵ�ִ���.
    CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"Bmp File(*.bmp)|*.bmp|JPG File(*.jpg)|*.jpg|All Files(*.*)|*.*||", NULL);
    if (fileDlg.DoModal() == IDOK)
    {
        CString stpathname = fileDlg.GetPathName();
        LoadBmp(stpathname);
    }
    else
        return;
}

// ���� BMP �ļ�
void CImageProc::LoadBmp(CString stFileName)
{
    // TODO: �ڴ˴�����ʵ�ִ���.
    CFile file;//�ļ�����
    CFileException e;//�ļ��쳣����
    if (!file.Open(stFileName, CFile::modeRead | CFile::shareExclusive, &e))
    {
#ifdef _DEBUG
        afxDump << "File could not be opened " << e.m_cause << "\n";
#endif
    }
    else
    {
        ULONGLONG nFileSize;    //ƥ��GetLength��������������
        nFileSize = file.GetLength();   //��ȡ�ļ���С
        m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, nFileSize);   //�����ڴ�
        pDib = (BYTE*)::GlobalLock(m_hDib);   //�����ڴ�
        file.Read(pDib, nFileSize);   //��ȡ�ļ�
        pBFH = (BITMAPFILEHEADER*)pDib;     //ָ���ļ�ͷ
        pBIH = (BITMAPINFOHEADER*)&pDib[sizeof(BITMAPFILEHEADER)];   //ָ����Ϣͷ
        pQUAD = (RGBQUAD*)&pDib[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)];   //ָ���ɫ��
        pBits = (BYTE*)&pDib[pBFH->bfOffBits];   //ָ��λͼ����
        nWidth = pBIH->biWidth;     // ��ȡͼ��Ŀ���
        nHeight = pBIH->biHeight;
		nBitCount = pBIH->biBitCount;   //��ȡλ���
   
        if (pBIH->biCompression == BI_RGB && nBitCount == 16) {
            //16λͼĬ��Ϊ555��ʽ
            m_bIs565Format = false;
        }
        else if (pBIH->biCompression == BI_BITFIELDS && nBitCount == 16) {
            // �����ɫ�����Ƿ�Ϊ565
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
    // TODO: �ڴ˴�����ʵ�ִ���.
    if (m_hDib != NULL)
    {
        ::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);      // ��������ģʽΪ COLORONCOLOR
        ::StretchDIBits(pDC->m_hDC, 0, 0, pBIH->biWidth, pBIH->biHeight, 0, 0, pBIH->biWidth, pBIH->biHeight, pBits, (BITMAPINFO*)pBIH, DIB_RGB_COLORS, SRCCOPY);       // ��λͼ���ݸ��Ƶ�Ŀ��DC��ָ��λ��
    }
}

// ��ȡ������ɫ
void CImageProc::GetColor(CClientDC* pDC, int x, int y)
{
    //��������Լ�ͼ���Ƿ���Ч
    if (m_hDib == NULL || x < 0 || x >= nWidth || y < 0 || y >= nHeight)
    {
        return; // ��Ч�����δ����ͼ��
    }

    // ÿ���ֽ��� = (ÿ�е�bit�� + 31) / 32 * 4     ��ÿ���ֽ���������4�ı�������bit����32�ı���������ȡ����
    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;


    //����λ��ȼ����ÿ�����ص���ʼλ��

    float  bytePerPixel = float(nBitCount)/ 8;
    // ÿ������ռ�õ��ֽ�����nNumColors Ϊÿ�����ص�λ�������������ݵ���8bitλͼ��

    int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
    // ƫ���� = (ͼ��߶� - 1 - ������) * ÿ���ֽ��� + ������ * ÿ������ռ�õ��ֽ���   ��y�ķ�Χ��[0,nHeight-1]�� 
    // ��ǿ������ת�������ڵ���8bitͼ��pixelָ��ǰ���������ֽڵ���ʼλ�á�

    BYTE* pixel = pBits + offset;// ��ȡ������λͼ�����е�λ�ã���ʼ��+ƫ������

    //  RGB ֵ
    BYTE red = 0, green = 0, blue = 0;

    switch (nBitCount)
    {
    case 1: // 1λλͼ
    {
        CImageProc::GetColor1bit(pixel,red,green,blue,x,y,pDC);

        break;
    }
    case 4: // 4λλͼ
    {
        CImageProc::GetColor4bit(pixel, red, green, blue, x);
        break;
    }
    case 8: // 8λλͼ
    {
        CImageProc::GetColor8bit(pixel, red, green, blue, x);
        break;
    }
    case 16: // 16λλͼ
    {
        CImageProc::GetColor16bit(pixel, red, green, blue);
        break;
    }
    case 24: // 24λλͼ
    {
        CImageProc::GetColor24bit(pixel, red, green, blue);
        break;
    }
    case 32: // 32λλͼ
    {
        CImageProc::GetColor32bit(pixel, red, green, blue);
        break;
    }
    default:
        return; // ��֧�ֵ���ɫ���
    }

    // ʹ�� GetPixel ��ȡ������ɫ
    COLORREF pixelColor = pDC->GetPixel(x, y);
    BYTE getPixelRed = GetRValue(pixelColor);
    BYTE getPixelGreen = GetGValue(pixelColor);
    BYTE getPixelBlue = GetBValue(pixelColor);

    // �����ı�������͸��
    pDC->SetBkMode(OPAQUE);

    // �����ı���ɫΪ��ɫ
    pDC->SetTextColor(RGB(0, 0, 0));

    // ��ʽ�� RGB ֵ��������Ϣ
    CString str;
    str.Format(L"RGB: (%d, %d, %d)", red, green, blue);

    CString getPixelStr;
    getPixelStr.Format(L"GetPixel RGB: (%d, %d, %d)", getPixelRed, getPixelGreen, getPixelBlue);

    CString location;
    location.Format(L"location��(%d, %d)", x, y);

    // �������λ����ʾ RGB ֵ
    pDC->TextOutW(x, y, str);

    // ��ȡ�ı��߶�
    CSize textSize = pDC->GetTextExtent(str);

    // ����һ����ʾ GetPixel ��ȡ�� RGB ֵ
    pDC->TextOutW(x, y + textSize.cy, getPixelStr);

    // ����һ����ʾ����
    pDC->TextOutW(x, y + textSize.cy * 2, location);

}
void CImageProc::GetColor1bit(BYTE* pixel, BYTE& red, BYTE& green, BYTE& blue, int x, int y, CDC* pDC)
{
    BYTE index = (*pixel >> (7 - x % 8)) & 0x01;
    red = pQUAD[index].rgbRed;
    green = pQUAD[index].rgbGreen;
    blue = pQUAD[index].rgbBlue;

    // ����ԭʼͼ��ֱ��ת����1bitͼ��֮�󣬺ڰ����ؽ����ֲ�������ȷ���Ƿ���ȷ��ʾ
    // ���´������ڲ鿴��ǰ����ֵ���Ӷ���֤�Ƿ���ʾ��ȷ

    //CString str2;
    //str2.Format(L"pixel��(%u);index��(%d)", *pixel, index);
    //// ��ȡ�ı��߶�
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
        // ��ȡ565��ʽ��RGB����
        red = (pixelValue & 0xF800) >> 11;    
        green = (pixelValue & 0x07E0) >> 5;   
        blue = pixelValue & 0x001F;           

        // ��������չ��8λ
        red = (red << 3) | (red >> 2);        
        green = (green << 2) | (green >> 4);  
        blue = (blue << 3) | (blue >> 2);    
    } else {
        // ��ȡ555��ʽ��RGB����
        red = (pixelValue & 0x7C00) >> 10;   
        green = (pixelValue & 0x03E0) >> 5;   
        blue = pixelValue & 0x001F;           

        // ��������չ��8λ
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

// ������ģʽ�Ҷ�ֱ��ͼ
std::vector<int> CImageProc::CalculateGrayHistogramMix()
{
    std::vector<int> histogram(256, 0);

    if (m_hDib == NULL)
    {
        return histogram;
    }

    // ÿ���ֽ��� = (ÿ�е�bit�� + 31) / 32 * 4
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

            // ����Ҷ�ֵ
            int gray = static_cast<int>(0.299 * red + 0.587 * green + 0.114 * blue);
            histogram[gray]++;
        }
    }

    return histogram;
}

//����RGBģʽֱ��ͼ
std::vector<std::vector<int>> CImageProc::CalculateRGBHistograms()
{
    std::vector<std::vector<int>> histograms(3, std::vector<int>(256, 0));

    if (m_hDib == NULL)
    {
        return histograms;
    }

    // ÿ���ֽ��� = (ÿ�е�bit�� + 31) / 32 * 4
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

            // ���¸�����ɫͨ����ֱ��ͼ
            histograms[0][red]++;
            histograms[1][green]++;
            histograms[2][blue]++;
        }
    }

    return histograms;
}

std::vector<std::vector<int>> CImageProc::Balance_Transformations(CClientDC& dc)
{
    std::vector<std::vector<int>> balancedRgbHistograms(3, std::vector<int>(256, 0)); // ������ԭ�����ı�����
    float p[256] = { 0 };
    float S[256] = { 0 };
    int F[256] = { 0 };
    std::vector<int> intensity_paint = CalculateGrayHistogramMix();
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
                balancedRgbHistograms[0][0]++;  // ��¼���⻯���ֵ
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

            // �ռ����⻯���ֱ��ͼ����
            balancedRgbHistograms[0][new_r]++;
            balancedRgbHistograms[1][new_g]++;
            balancedRgbHistograms[2][new_b]++;

            dc.SetPixelV(x, y, RGB(new_r, new_g, new_b));
        }
    }

    return balancedRgbHistograms; // ���ؾ��⻯���ֱ��ͼ
}

// ���Ӻڰ׷����
void CImageProc::ApplyBlackAndWhiteStyle()
{
    if (m_hDib == NULL)
    {
        AfxMessageBox(_T("No valid image is loaded."));
        return;
    }

    // �����Ҷȵ�ɫ��
    std::vector<RGBQUAD> grayPalette(256);
    for (int i = 0; i < 256; ++i)
    {
        grayPalette[i].rgbRed = static_cast<BYTE>(i);
        grayPalette[i].rgbGreen = static_cast<BYTE>(i);
        grayPalette[i].rgbBlue = static_cast<BYTE>(i);
        grayPalette[i].rgbReserved = 0;
    }

    //// �滻ԭ�е�ɫ�壨�������� 8 λ��λͼ��
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

    // ����ÿ�е��ֽ���
    int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;

    // ����λͼ��������Ӧ�Ҷȵ�ɫ��
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

            // ����Ҷ�ֵ
            BYTE grayValue = static_cast<BYTE>((red * 0.299) + (green * 0.587) + (blue * 0.114));

            // ��������ֵ
            switch (nBitCount)
            {
            case 1:
            {
                BYTE bitIndex = x % 8;
                if (grayValue < 128)
                {
                    *pixel &= ~(1 << (7 - bitIndex));// ���ö�ӦλΪ0
                    
                }
                else
                {
                    *pixel |= (1 << (7 - bitIndex));// ���ö�ӦλΪ1
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