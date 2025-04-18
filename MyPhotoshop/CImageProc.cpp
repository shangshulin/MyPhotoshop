#include "pch.h"
#include "CImageProc.h"
#include <afxdlgs.h>
#include <vector>
#include <algorithm>


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
	isPaletteDarkToLight = false;
}

//打开文件
void CImageProc::OpenFile()
{
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"Bmp File(*.bmp)|*.bmp|JPG File(*.jpg)|*.jpg|All Files(*.*)|*.*||", NULL);
	if (fileDlg.DoModal() == IDOK)
	{
		CString stpathname = fileDlg.GetPathName();//获取文件路径
		LoadBmp(stpathname);//加载图片
	}
	else
		return;
}

// 加载图片
void CImageProc::LoadBmp(CString stFileName)
{
	CleanUp();//清空内存

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
	if (nFileSize < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))//文件大小太小
	{
		file.Close();//关闭文件
		return;
	}

	m_hDib = ::GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, nFileSize);//分配内存
	if (!m_hDib)//分配内存失败
	{
		file.Close();
		return;
	}

	pDib = (BYTE*)::GlobalLock(m_hDib);//分配内存
	if (!pDib)//分配内存失败
	{
		GlobalFree(m_hDib);
		m_hDib = NULL;
		file.Close();
		return;
	}

	UINT nBytesRead = file.Read(pDib, (UINT)nFileSize);//读取文件
	file.Close();

	if (nBytesRead != nFileSize)//读取文件失败
	{
		CleanUp();
		return;
	}

	pBFH = (BITMAPFILEHEADER*)pDib;//获取文件头
	if (pBFH->bfType != 0x4D42)//检查文件头
	{
		CleanUp();
		return;
	}

	pBIH = (BITMAPINFOHEADER*)&pDib[sizeof(BITMAPFILEHEADER)];//获取信息头
	if (pBIH->biSize < sizeof(BITMAPINFOHEADER))//检查信息头
	{
		CleanUp();
		return;
	}

	pQUAD = (RGBQUAD*)&pDib[sizeof(BITMAPFILEHEADER) + pBIH->biSize];//获取调色板

	if (pBFH->bfOffBits >= nFileSize)//检查文件头
	{
		CleanUp();
		return;
	}
	pBits = &pDib[pBFH->bfOffBits];//获取位图数据
	nWidth = pBIH->biWidth;//获取宽高和位深
	nHeight = abs(pBIH->biHeight);
	nBitCount = pBIH->biBitCount;

	DWORD dwImageSize = ((nWidth * nBitCount + 31) / 32) * 4 * nHeight;//dwImageSize为位图数据的大小
	if (pBFH->bfOffBits + dwImageSize > nFileSize)//检查位图数据大小
	{
		CleanUp();
		return;
	}

	if (pBIH->biCompression == BI_RGB && nBitCount == 16)//检查位图数据格式
	{
		m_bIs565Format = false;
	}
	else if (pBIH->biCompression == BI_BITFIELDS && nBitCount == 16)//处理565格式的16位位图数据
	{
		if (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD) <= nFileSize)//pBIH->biSize为信息头的大小
		{
			DWORD* masks = reinterpret_cast<DWORD*>(&pDib[sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)]);
			DWORD redMask = masks[0];
			DWORD greenMask = masks[1];
			DWORD blueMask = masks[2];
			//检查位图数据格式
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
void CImageProc::ShowBMP(CDC* pDC)
{
	if (m_hDib != NULL)
	{
		::SetStretchBltMode(pDC->m_hDC, COLORONCOLOR);//设置拉伸模式为COLORONCOLOR
		::StretchDIBits(pDC->m_hDC, 0, 0, pBIH->biWidth, pBIH->biHeight, 0, 0, pBIH->biWidth, pBIH->biHeight, pBits, (BITMAPINFO*)pBIH, DIB_RGB_COLORS, SRCCOPY);//显示图片
	}
}

// 获取像素颜色
void CImageProc::GetColor(CClientDC* pDC, int x, int y)
{
	if (m_hDib == NULL || x < 0 || x >= nWidth || y < 0 || y >= nHeight)
	{
		return;
	}
	// 计算每行字节数
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	// 计算每个像素的字节数
	float bytePerPixel = float(nBitCount) / 8;
	// 计算像素在位图中的偏移量
	int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
	// pixel指向当前像素
	BYTE* pixel = pBits + offset;
	// 获取像素颜色
	BYTE red = 0, green = 0, blue = 0;
	// 根据位深获取像素颜色
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

	pDC->SetBkMode(OPAQUE);//设置背景色为不透明

	pDC->SetTextColor(RGB(0, 0, 0));//设置文本颜色为黑色

	CString str;
	str.Format(L"RGB: (%d, %d, %d)", red, green, blue);

	CString getPixelStr;
	getPixelStr.Format(L"GetPixel RGB: (%d, %d, %d)", getPixelRed, getPixelGreen, getPixelBlue);

	CString location;
	location.Format(L"location:(%d, %d)", x, y);

	pDC->TextOutW(x, y, str);//显示像素颜色

	CSize textSize = pDC->GetTextExtent(str);

	pDC->TextOutW(x, y + textSize.cy, getPixelStr);//显示GetPixel颜色

	pDC->TextOutW(x, y + textSize.cy * 2, location);//显示像素位置
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
	if (m_bIs565Format) // 处理565格式
	{

		red = (pixelValue & 0xF800) >> 11;
		green = (pixelValue & 0x07E0) >> 5;
		blue = pixelValue & 0x001F;


		red = (red << 3) | (red >> 2);
		green = (green << 2) | (green >> 4);
		blue = (blue << 3) | (blue >> 2);
	}
	else // 处理555格式
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

	// 计算每行字节数
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	// 计算每个像素的字节数
	float bytePerPixel = float(nBitCount) / 8;
	// 遍历每个像素
	for (int y = 0; y < nHeight; ++y)
	{
		for (int x = 0; x < nWidth; ++x)
		{
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);//计算像素在位图中的偏移量
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
	// 计算每行字节数
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	// 计算每个像素的字节数
	float bytePerPixel = float(nBitCount) / 8;
	// 遍历每个像素
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
	else if (nBitCount == 16) {
		if (!IsValid() || nBitCount != 16) return;

		int rowSize = ((nWidth * 16 + 31) / 32) * 4;

		for (int y = 0; y < nHeight; y++) {
			BYTE* pPixel = pBits + (nHeight - 1 - y) * rowSize;

			for (int x = 0; x < nWidth; x++) {
				WORD* pixel = reinterpret_cast<WORD*>(&pPixel[x * 2]);
				WORD oldPixel = *pixel;

				BYTE r, g, b;
				GetColor16bit(reinterpret_cast<BYTE*>(&oldPixel), r, g, b);

				int newR = min(255, static_cast<int>(r * 1.15 + 15));
				int newG = min(255, static_cast<int>(g * 0.85 + 5));
				int newB = min(255, static_cast<int>(b * 0.7));


				int noise = (rand() % 7) - 3;
				newR = max(0, min(255, newR + noise));
				newG = max(0, min(255, newG + noise));
				newB = max(0, min(255, newB + noise));


				if (m_bIs565Format) {

					*pixel = ((newR >> 3) << 11) |
						((newG >> 2) << 5) |
						(newB >> 3);
				}
				else {

					*pixel = ((newR >> 3) << 10) |
						((newG >> 3) << 5) |
						(newB >> 3);
				}
			}
		}
	}
	else {
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
}


// 函数定义：直方图均衡化处理，返回均衡化后的RGB三通道直方图数据
// 输入参数：dc为设备上下文对象，用于像素操作
std::vector<std::vector<int>> CImageProc::Balance_Transformations(CClientDC& dc)
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

	// 计算每行像素数据的字节对齐长度（兼容32位对齐规则）
	int rowSize = ((w * nBitCount + 31) / 32) * 4;
	// 计算每个像素占用的字节数（根据位深度nBitCount）
	float bytePerPixel = nBitCount / 8.0f;

	// 遍历图像每个像素,进行均衡化处理
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			// 计算当前像素的内存偏移量（考虑图像倒置存储）
			int offset = (h - 1 - y) * rowSize + static_cast<int>(x * bytePerPixel);//图像倒置存储+当前行内像素的水平偏移
			// 获取像素指针（pBits为类内图像数据指针）
			BYTE* pixel = pBits + offset;//pixel指向当前像素的起始位置

			// 定义RGB分量变量
			BYTE red, green, blue;

			// 根据位深度调用不同的颜色解析方法
			switch (nBitCount) {
			case 1:  // 1位色：调色板索引模式
				GetColor1bit(pixel, red, green, blue, x, y, nullptr);
				break;
			case 4:  // 4位色：16色模式
				GetColor4bit(pixel, red, green, blue, x);
				break;
			case 8:  // 8位色：256色模式
				GetColor8bit(pixel, red, green, blue, x);
				break;
			case 16: // 16位高彩色（可能含透明度）
				GetColor16bit(pixel, red, green, blue);
				break;
			case 24: // 24位真彩色（RGB各8位）
				GetColor24bit(pixel, red, green, blue);
				break;
			case 32: // 32位色（含Alpha通道）
				GetColor32bit(pixel, red, green, blue);
				break;
			default: // 不支持其他位深度
				continue;
			}

			// 根据人眼对颜色的敏感度（绿 > 红 > 蓝），使用加权系数计算亮度（ITU-R BT.601标准加权：Y=0.299R+0.587G+0.114B）
			int Y = static_cast<int>(0.3f * red + 0.59f * green + 0.11f * blue + 0.5f);
			Y = max(0, min(255, Y)); // 约束到0-255范围

			// 特殊处理纯黑像素,直接跳过缩放计算，​​保持像素为黑色
			if (Y == 0) {
				dc.SetPixelV(x, y, RGB(0, 0, 0));  // 设置纯黑色
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

			// 设置设备上下文的像素颜色
			dc.SetPixelV(x, y, RGB(new_r, new_g, new_b));
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
	if (nBitCount == 1)
	{
		// 判断原调色板的明暗顺序
		if (pQUAD[0].rgbRed < pQUAD[1].rgbRed)
		{
			isPaletteDarkToLight = true;
		}
		else
		{
			isPaletteDarkToLight = false;
		}

		std::vector<RGBQUAD> grayPalette1bit(2); // 创建1位灰度调色板
		if (isPaletteDarkToLight)
		{
			for (int i = 0; i < 2; i++)
			{
				grayPalette1bit[i].rgbRed = static_cast<BYTE>(i * 255);
				grayPalette1bit[i].rgbGreen = static_cast<BYTE>(i * 255);
				grayPalette1bit[i].rgbBlue = static_cast<BYTE>(i * 255);
				grayPalette1bit[i].rgbReserved = 0;
			}
		}
		else
		{
			for (int i = 0; i < 2; i++)
			{
				grayPalette1bit[i].rgbRed = static_cast<BYTE>((2 - i) * 255);
				grayPalette1bit[i].rgbGreen = static_cast<BYTE>((2 - i) * 255);
				grayPalette1bit[i].rgbBlue = static_cast<BYTE>((2 - i) * 255);
				grayPalette1bit[i].rgbReserved = 0;
			}
		}
		memcpy(pQUAD, grayPalette1bit.data(), sizeof(RGBQUAD) * 2);
	}
	else if (nBitCount == 8)
	{
		if (pQUAD[0].rgbRed < pQUAD[255].rgbRed)
		{
			isPaletteDarkToLight = true;
		}
		else
		{
			isPaletteDarkToLight = false;
		}
		std::vector<RGBQUAD> grayPalette8bit(256); // 创建8位灰度调色板

		if (isPaletteDarkToLight)
		{
			for (int i = 0; i < 256; i++)
			{
				grayPalette8bit[i].rgbRed = static_cast<BYTE>(i);
				grayPalette8bit[i].rgbGreen = static_cast<BYTE>(i);
				grayPalette8bit[i].rgbBlue = static_cast<BYTE>(i);
				grayPalette8bit[i].rgbReserved = 0;
			}
		}
		else
		{
			for (int i = 0; i < 256; i++)
			{
				grayPalette8bit[i].rgbRed = static_cast<BYTE>(255 - i);
				grayPalette8bit[i].rgbGreen = static_cast<BYTE>(255 - i);
				grayPalette8bit[i].rgbBlue = static_cast<BYTE>(255 - i);
				grayPalette8bit[i].rgbReserved = 0;
			}
		}
		memcpy(pQUAD, grayPalette8bit.data(), sizeof(RGBQUAD) * 256);
	}


	// 获取图像的行字节数
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	// 计算每个像素的字节数
	float bytePerPixel = float(nBitCount) / 8;

	// 遍历图像的每个像素点
	for (int y = 0; y < nHeight; ++y)
	{
		for (int x = 0; x < nWidth; ++x)
		{
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);//计算像素在位图中的偏移量
			BYTE* pixel = pBits + offset;// 获取像素的偏移量

			BYTE red = 0, green = 0, blue = 0;

			switch (nBitCount)
			{
			case 1:
				GetColor1bit(pixel, red, green, blue, x, y, nullptr);
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

			// 计算灰度值
			BYTE grayValue = static_cast<BYTE>((red * 0.299) + (green * 0.587) + (blue * 0.114));

			// 根据灰度值设置像素
			switch (nBitCount)
			{
			case 1:
				break;
			case 8:
				break;
			case 16:
			{
				WORD newPixel;
				if (m_bIs565Format)
				{
					// RGB565: 5-6-5
					BYTE r = (grayValue >> 3) & 0x1F;  // 5-bit red
					BYTE g = (grayValue >> 2) & 0x3F;  // 6-bit green
					BYTE b = (grayValue >> 3) & 0x1F;  // 5-bit blue
					newPixel = (r << 11) | (g << 5) | b;
				}
				else
				{
					// RGB555: 5-5-5
					BYTE r = (grayValue >> 3) & 0x1F;;  // 5-bit red
					BYTE g = (grayValue >> 3) & 0x1F;;  // 5-bit green
					BYTE b = (grayValue >> 3) & 0x1F;;  // 5-bit blue
					newPixel = (r << 10) | (g << 5) | b;
				}
				*((WORD*)pixel) = newPixel;
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

// 实现直方图规格化函数

bool CImageProc::HistogramMatching(CImageProc& targetImageProc)
{
	if (!IsValid() || !targetImageProc.IsValid())
	{
		AfxMessageBox(_T("图像数据无效"));
		return false;
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

		// 计算源图像直方图
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
					GetColor1bit(&pSource[x / 8], red, green, blue, x % 8, y, nullptr);
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

				BYTE val;
				switch (channel)
				{
				case 0:
					val = red;
					break;
				case 1:
					val = green;
					break;
				case 2:
					val = blue;
					break;
				}
				sourceHist[val]++;
			}
		}

		// 计算目标图像直方图
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
					GetColor1bit(&pTarget[x / 8], red, green, blue, x % 8, y, nullptr);
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

				BYTE val;
				switch (channel % numTargetChannels)
				{
				case 0:
					val = red;
					break;
				case 1:
					val = green;
					break;
				case 2:
					val = blue;
					break;
				}
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

		// 归一化
		for (int i = 0; i < 256; i++)
		{
			sourceCDF[i] /= (width * height);
			targetCDF[i] /= (targetWidth * targetHeight);
		}

		// 创建映射表
		std::vector<BYTE> mapping(256, 0);
		for (int i = 0; i < 256; i++)
		{
			double cdf = sourceCDF[i];
			int left = 0, right = 255;
			int result = 0;
			while (left <= right)
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

		// 应用映射
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
					GetColor1bit(&pSource[x / 8], red, green, blue, x % 8, y, nullptr);
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

				BYTE newRed = red;
				BYTE newGreen = green;
				BYTE newBlue = blue;
				switch (channel)
				{
				case 0:
					newRed = mapping[red];
					break;
				case 1:
					newGreen = mapping[green];
					break;
				case 2:
					newBlue = mapping[blue];
					break;
				}

				switch (nBitCount)
				{
				case 1:
					break;
				case 4:
					break;
				case 8:
					pSource[x] = newRed;
					break;
				case 16:
				{
					WORD newPixel;
					if (m_bIs565Format)
					{
						BYTE r = (newRed >> 3) & 0x1F;
						BYTE g = (newGreen >> 2) & 0x3F;
						BYTE b = (newBlue >> 3) & 0x1F;
						newPixel = (r << 11) | (g << 5) | b;
					}
					else
					{
						BYTE r = (newRed >> 3) & 0x1F;
						BYTE g = (newGreen >> 3) & 0x1F;
						BYTE b = (newBlue >> 3) & 0x1F;
						newPixel = (r << 10) | (g << 5) | b;
					}
					*((WORD*)&pSource[x * 2]) = newPixel;
					break;
				}
				case 24:
					pSource[x * 3] = newBlue;
					pSource[x * 3 + 1] = newGreen;
					pSource[x * 3 + 2] = newRed;
					break;
				case 32:
					pSource[x * 4] = newBlue;
					pSource[x * 4 + 1] = newGreen;
					pSource[x * 4 + 2] = newRed;
					break;
				}
			}
		}
	}

	return true;
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
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	float bytePerPixel = float(nBitCount) / 8;

	// 转换为灰度图像
	for (int y = 0; y < nHeight; ++y) {
		for (int x = 0; x < nWidth; ++x) {
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;

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
			magnitude = min(255, max(0, magnitude));

			// 更新像素
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;

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
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	float bytePerPixel = float(nBitCount) / 8;

	// 转换为灰度图像
	for (int y = 0; y < nHeight; ++y) {
		for (int x = 0; x < nWidth; ++x) {
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;

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
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;

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
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	float bytePerPixel = float(nBitCount) / 8;

	// 转换为灰度图像
	for (int y = 0; y < nHeight; ++y) {
		for (int x = 0; x < nWidth; ++x) {
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;

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
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;

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
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	float bytePerPixel = float(nBitCount) / 8;
	// 转换为灰度图像
	for (int y = 0; y < nHeight; ++y) {
		for (int x = 0; x < nWidth; ++x) {
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;
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
	const int sobelY[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };
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
	std::vector<BYTE> edgeImage(nWidth* nHeight, 0);
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
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;
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
	int rowSize = ((nWidth * nBitCount + 31) / 32) * 4;
	float bytePerPixel = float(nBitCount) / 8;
	for (int y = 0; y < nHeight; ++y) {
		for (int x = 0; x < nWidth; ++x) {
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;
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

	// 2. 高斯平滑（5x5核，sigma=1.0）
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
			int offset = (nHeight - 1 - y) * rowSize + int(float(x) * bytePerPixel);
			BYTE* pixel = pBits + offset;
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
