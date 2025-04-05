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
    // TODO: �ڴ˴����ʵ�ִ���.
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
    // TODO: �ڴ˴����ʵ�ִ���.
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
        nWidth = pBIH->biWidth;     // ��ȡͼ��Ŀ��
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
    // TODO: �ڴ˴����ʵ�ִ���.
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

    // ����ԭʼͼ��ֱ��ת����1bitͼ��֮�󣬺ڰ����ؽ���ֲ�������ȷ���Ƿ���ȷ��ʾ
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
