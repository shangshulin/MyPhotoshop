// CINTENSITYDlg.cpp: 实现文件
//

#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "CINTENSITYDlg.h"


// CINTENSITYDlg 对话框

IMPLEMENT_DYNAMIC(CINTENSITYDlg, CDialogEx)

CINTENSITYDlg::CINTENSITYDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_INTENSITY_DLG, pParent)
	, m_alpha(1.0)  // 默认值为1.0
	, m_beta(0.0)   // 默认值为0.0
	, m_pImage(nullptr)
{

}

CINTENSITYDlg::~CINTENSITYDlg()
{
}

void CINTENSITYDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// 关联控件
	DDX_Control(pDX, IDC_EDIT_ALPHA, m_editAlpha);
	DDX_Control(pDX, IDC_EDIT_BETA, m_editBeta);
	// 关联数据
	DDX_Text(pDX, IDC_EDIT_ALPHA, m_alpha);
	DDX_Text(pDX, IDC_EDIT_BETA, m_beta);
	DDX_Control(pDX, IDC_COMBO1, m_Intensity_trans_mode);
}


BEGIN_MESSAGE_MAP(CINTENSITYDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &CINTENSITYDlg::OnBnClickedButtonApply)// 应用按钮
	ON_BN_CLICKED(IDC_BUTTON_CANCEL, &CINTENSITYDlg::OnBnClickedButtonCancel)// 取消按钮

END_MESSAGE_MAP()


// CINTENSITYDlg 消息处理程序

BOOL CINTENSITYDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置初始值
	m_editAlpha.SetWindowText(_T("1.0"));
	m_editBeta.SetWindowText(_T("0.0"));

	// 添加下拉框选项（顺序：线性、对数、指数）
	m_Intensity_trans_mode.AddString(_T("线性"));  // 索引 0
	m_Intensity_trans_mode.AddString(_T("对数"));  // 索引 1
	m_Intensity_trans_mode.AddString(_T("指数"));  // 索引 2

	// 默认选择第一个选项（线性）
	m_Intensity_trans_mode.SetCurSel(0);

	return TRUE;
}

void CINTENSITYDlg::SetImageData(CImageProc* pImage)
{
	m_pImage = pImage;
}

void CINTENSITYDlg::OnBnClickedButtonApply()
{
	// 更新数据
	UpdateData(TRUE);

	// 应用灰度线性变换
	ApplyIntensityTransform();
}

void CINTENSITYDlg::OnBnClickedButtonCancel()
{
	// 关闭对话框
	 CDialogEx::OnOK();
}

void CINTENSITYDlg::ApplyIntensityTransform()
{
	if (m_pImage == nullptr)
	{
		AfxMessageBox(_T("未加载图像！"));
		return;
	}

	// 获取用户选择的变换模式
	int selectedMode = m_Intensity_trans_mode.GetCurSel();
	if (selectedMode < 0 || selectedMode > 2)
	{
		AfxMessageBox(_T("无效的变换模式！"));
		return;
	}

	// 获取图像数据
	HANDLE hDib = m_pImage->m_hDib;
	if (hDib == NULL)
	{
		AfxMessageBox(_T("图像数据无效！"));
		return;
	}

	// 锁定内存
	BYTE* pDib = (BYTE*)GlobalLock(hDib);
	if (pDib == NULL)
	{
		AfxMessageBox(_T("无法访问图像数据！"));
		return;
	}

	BITMAPFILEHEADER* pBFH = (BITMAPFILEHEADER*)pDib;//获取文件头

	// 获取位图信息头
	BITMAPINFOHEADER* pBIH = (BITMAPINFOHEADER*)(pDib + sizeof(BITMAPFILEHEADER));
	int nWidth = pBIH->biWidth;
	int nHeight = pBIH->biHeight;
	int nBitCount = pBIH->biBitCount;

	// 获取颜色表和像素数据
	RGBQUAD* pRGBQuad = NULL;
	BYTE* pBits = NULL;

	// 根据位深度计算颜色表的位置和大小
	if (nBitCount <= 8)
	{
		pRGBQuad = (RGBQUAD*)(pDib + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		pBits = &pDib[pBFH->bfOffBits];//获取位图数据
	}
	else
	{
		pBits = &pDib[pBFH->bfOffBits];//获取位图数据
	}

	// 应用灰度线性变换
	// 公式: newPixel = alpha * oldPixel + beta
	switch (nBitCount)
	{
	case 1:
	{
		// 修改颜色表
		for (int i = 0; i < 2; i++)
		{
			int oldPixel = 0.299 * pRGBQuad[i].rgbRed + 0.587 * pRGBQuad[i].rgbGreen + 0.114 * pRGBQuad[i].rgbBlue;
			int newValue = 0;
			switch (selectedMode)
			{
			case 0: // 线性变换
				newValue = (int)(m_alpha * oldPixel + m_beta);
				break;
			case 1: // 对数变换
				newValue = (int)(m_alpha * log(1.0 + oldPixel) + m_beta);
				break;
			case 2: // 指数变换
				newValue = (int)(m_alpha * exp(oldPixel / 255.0) + m_beta);
				break;
			}

			// int newValue = (int)(m_alpha * oldPixel + m_beta);
			// 确保值在0-255范围内
			newValue = max(0, min(255, newValue));

			pRGBQuad[i].rgbRed = newValue;
			pRGBQuad[i].rgbGreen = newValue;
			pRGBQuad[i].rgbBlue = newValue;
		}
	break;
	}
	case 8:  // 8位灰度图像
	{
		// 修改颜色表
		for (int i = 0; i < 256; i++)
		{
			int oldPixel = 0.299 * pRGBQuad[i].rgbRed + 0.587 * pRGBQuad[i].rgbGreen + 0.114 * pRGBQuad[i].rgbBlue;
			
			int newValue = 0;
			switch (selectedMode)
			{
			case 0: // 线性变换
				newValue = (int)(m_alpha * oldPixel + m_beta);
				break;
			case 1: // 对数变换
				newValue = (int)(m_alpha * log(1.0 + oldPixel) + m_beta);
				break;
			case 2: // 指数变换
				newValue = (int)(m_alpha * exp(oldPixel / 255.0) + m_beta);
				break;
			}
			//int newValue = (int)(m_alpha * oldPixel + m_beta);
			// 确保值在0-255范围内
			newValue = max(0, min(255, newValue));

			pRGBQuad[i].rgbRed = newValue;
			pRGBQuad[i].rgbGreen = newValue;
			pRGBQuad[i].rgbBlue = newValue;
		}
	break;
	}
	case 16: // 16位图像
	{
		bool is565 = m_pImage->m_bIs565Format; // 判断是否为565格式
		int bytesPerLine = ((nWidth * 2 + 3) / 4) * 4; // 每行字节数（4字节对齐）

		for (int y = 0; y < nHeight; y++)
		{
			for (int x = 0; x < nWidth; x++)
			{
				int offset = y * bytesPerLine + x * 2;
				WORD pixel = *(WORD*)(pBits + offset);

				BYTE red, green, blue;

				if (is565)
				{
					// 提取RGB值 (565格式)
					red = (pixel >> 11) & 0x1F;
					green = (pixel >> 5) & 0x3F;
					blue = pixel & 0x1F;

					// 转换到0-255范围
					red = (red * 255) / 31;
					green = (green * 255) / 63;
					blue = (blue * 255) / 31;
				}
				else
				{
					// 提取RGB值 (555格式)
					red = (pixel >> 10) & 0x1F;
					green = (pixel >> 5) & 0x1F;
					blue = pixel & 0x1F;

					// 转换到0-255范围
					red = (red * 255) / 31;
					green = (green * 255) / 31;
					blue = (blue * 255) / 31;
				}

				// 计算灰度值
				BYTE gray = (BYTE)(0.299 * red + 0.587 * green + 0.114 * blue);

				// 应用线性变换
				int newGray = 0;
				switch (selectedMode)
				{
				case 0: // 线性变换
					newGray = (int)(m_alpha * gray + m_beta);
					break;
				case 1: // 对数变换
					newGray = (int)(m_alpha * log(1.0 + gray) + m_beta);
					break;
				case 2: // 指数变换
					newGray = (int)(m_alpha * exp(gray / 255.0) + m_beta);
					break;
				}
				//int newGray = (int)(m_alpha * gray + m_beta);
				newGray = max(0, min(255, newGray)); // 确保值在0-255范围内

				// 将灰度值转换回16位格式
				if (is565)
				{
					red = (newGray * 31) / 255;
					green = (newGray * 63) / 255;
					blue = (newGray * 31) / 255;

					pixel = (red << 11) | (green << 5) | blue;
				}
				else
				{
					red = (newGray * 31) / 255;
					green = (newGray * 31) / 255;
					blue = (newGray * 31) / 255;

					pixel = (red << 10) | (green << 5) | blue;
				}

				// 更新像素值
				*(WORD*)(pBits + offset) = pixel;
			}
		}
		break;
	}
	case 24:  // 24位彩色图像
	{
		// 计算每行字节数（需要4字节对齐）
		int nBytesPerLine = (nWidth * 3 + 3) / 4 * 4;

		for (int y = 0; y < nHeight; y++)
		{
			for (int x = 0; x < nWidth; x++)
			{
				int offset = y * nBytesPerLine + x * 3;

				// 获取RGB值
				BYTE blue = pBits[offset];
				BYTE green = pBits[offset + 1];
				BYTE red = pBits[offset + 2];

				// 计算灰度值
				BYTE gray = (BYTE)(0.299 * red + 0.587 * green + 0.114 * blue);

				// 应用线性变换
				int newGray = 0;
				switch (selectedMode)
				{
				case 0: // 线性变换
					newGray = (int)(m_alpha * gray + m_beta);
					break;
				case 1: // 对数变换
					newGray = (int)(m_alpha * log(1.0 + gray) + m_beta);
					break;
				case 2: // 指数变换
					newGray = (int)(m_alpha * exp(gray / 255.0) + m_beta);
					break;
				}
				//int newGray = (int)(m_alpha * gray + m_beta);
				// 确保值在0-255范围内
				newGray = max(0, min(255, newGray));

				// 更新像素值
				pBits[offset] = newGray;     // B
				pBits[offset + 1] = newGray; // G
				pBits[offset + 2] = newGray; // R
			}
		}
		break;
	}
	

	case 32:  // 32位彩色图像
	{
		for (int y = 0; y < nHeight; y++)
		{
			for (int x = 0; x < nWidth; x++)
			{
				int offset = (y * nWidth + x) * 4;

				// 获取RGB值
				BYTE blue = pBits[offset];
				BYTE green = pBits[offset + 1];
				BYTE red = pBits[offset + 2];

				// 计算灰度值
				BYTE gray = (BYTE)(0.299 * red + 0.587 * green + 0.114 * blue);

				// 应用线性变换
				int newGray = 0;
				switch (selectedMode)
				{
				case 0: // 线性变换
					newGray = (int)(m_alpha * gray + m_beta);
					break;
				case 1: // 对数变换
					newGray = (int)(m_alpha * log(1.0 + gray) + m_beta);
					break;
				case 2: // 指数变换
					newGray = (int)(m_alpha * exp(gray / 255.0) + m_beta);
					break;
				}
				//int newGray = (int)(m_alpha * gray + m_beta);
				// 确保值在0-255范围内
				newGray = max(0, min(255, newGray));

				// 更新像素值
				pBits[offset] = newGray;     // B
				pBits[offset + 1] = newGray; // G
				pBits[offset + 2] = newGray; // R
			}
		}
		break;
	}


	default:
		AfxMessageBox(_T("不支持的图像格式！"));
		break;
	}

	// 解锁内存
	GlobalUnlock(hDib);

	// 刷新视图
	CWnd* pMainWnd = AfxGetMainWnd();
	if (pMainWnd)
	{
		// 获取活动视图
		CView* pView = ((CFrameWnd*)pMainWnd)->GetActiveView();
		if (pView)
		{
			// 强制重绘
			pView->Invalidate();
			pView->UpdateWindow();
		}
	}
}
