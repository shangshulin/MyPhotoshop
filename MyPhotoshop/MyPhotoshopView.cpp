
// MyPhotoshopView.cpp: CMyPhotoshopView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "MyPhotoshop.h"
#endif
#include "CImageProc.h"
#include "MyPhotoshopDoc.h"
#include "MyPhotoshopView.h"
#include <algorithm>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMyPhotoshopView

IMPLEMENT_DYNCREATE(CMyPhotoshopView, CView)// 动态创建

BEGIN_MESSAGE_MAP(CMyPhotoshopView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_WM_LBUTTONDOWN() // 左键点击
	ON_COMMAND(ID_VIEW_PIXELINFO, &CMyPhotoshopView::OnViewPixelInfo) // 菜单项点击
	ON_UPDATE_COMMAND_UI(ID_VIEW_PIXELINFO, &CMyPhotoshopView::OnUpdateViewPixelInfo) // 更新菜单项状态
	ON_COMMAND(ID_FUNCTION_HISTOGRAM_MATCHING, &CMyPhotoshopView::OnFunctionHistogramMatching)
    ON_COMMAND(ID_COLOR_STYLE_VINTAGE, &CMyPhotoshopView::OnColorStyleVintage)
    ON_UPDATE_COMMAND_UI(ID_COLOR_STYLE_VINTAGE, &CMyPhotoshopView::OnUpdateColorStyleVintage)
END_MESSAGE_MAP()




// CMyPhotoshopView 构造/析构

CMyPhotoshopView::CMyPhotoshopView() noexcept
	: m_bShowPixelInfo(false) // 默认不显示像素点信息
{
	// TODO: 在此处添加构造代码

}

CMyPhotoshopView::~CMyPhotoshopView()
{
}

BOOL CMyPhotoshopView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CMyPhotoshopView 绘图

void CMyPhotoshopView::OnDraw(CDC* pDC)
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
	if (pDoc->pImage->m_hDib != NULL)
	{
		pDoc->pImage->ShowBMP(pDC);
	}
}


// CMyPhotoshopView 打印

BOOL CMyPhotoshopView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 默认准备
	return DoPreparePrinting(pInfo);
}

void CMyPhotoshopView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加额外的打印前进行的初始化过程
}

void CMyPhotoshopView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加打印后进行的清理过程
}


// CMyPhotoshopView 诊断

#ifdef _DEBUG
void CMyPhotoshopView::AssertValid() const
{
	CView::AssertValid();
}

void CMyPhotoshopView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMyPhotoshopDoc* CMyPhotoshopView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMyPhotoshopDoc)));
	return (CMyPhotoshopDoc*)m_pDocument;
}
#endif //_DEBUG


// CMyPhotoshopView 消息处理程序

void CMyPhotoshopView::OnViewPixelInfo()
{
	// 切换显示像素信息的状态
	m_bShowPixelInfo = !m_bShowPixelInfo;

	if (m_bShowPixelInfo)
	{
		AfxMessageBox(_T("显示像素点信息已启用"));
	}
	else
	{
		AfxMessageBox(_T("显示像素点信息已禁用"));
	}
	Invalidate(true); // 强制重绘视图以清除像素点信息
	UpdateWindow();   // 立即更新窗口
}

void CMyPhotoshopView::OnUpdateViewPixelInfo(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowPixelInfo); // 设置菜单项选中状态
}


void CMyPhotoshopView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	// 如果启用了显示像素点信息
	if (m_bShowPixelInfo)
	{
		Invalidate(true);
		UpdateWindow();
		// 获取文档中的图像数据
		if (nFlags & MK_SHIFT)
		{
			CClientDC dc(this);
			CMyPhotoshopDoc* pDoc = GetDocument();// 获取文档中的图像数据
			ASSERT_VALID(pDoc);
			pDoc->pImage->GetColor(&dc, point.x, point.y);
		}
	}
	CView::OnLButtonDown(nFlags, point);
}

void CMyPhotoshopView::OnFunctionHistogramMatching()
{
    HistogramMatching();
}

//允许用户自行选择图像，但有断言错误
//void CMyPhotoshopView::HistogramMatching()
//{
//    CMyPhotoshopDoc* pDoc = GetDocument();
//    if (!pDoc || !pDoc->pImage || !pDoc->pImage->m_hDib)
//    {
//        AfxMessageBox(_T("请先打开有效的源图像文件"));
//        return;
//    }
//
//    CImageProc* pSourceImage = pDoc->pImage; // 源图像
//    if (pSourceImage->nBitCount != 24)
//    {
//        AfxMessageBox(_T("源图像必须是24位色"));
//        return;
//    }
//
//    // 让用户选择目标图像
//    CFileDialog fileDlg(TRUE, _T("bmp"), NULL,
//        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
//        _T("24位位图文件(*.bmp)|*.bmp|所有文件(*.*)|*.*||"), this);
//
//    if (fileDlg.DoModal() != IDOK)
//    {
//        return; // 用户取消了选择
//    }
//
//    CString targetPath = fileDlg.GetPathName();
//
//    // 加载目标图像
//    CImageProc targetImageProc;
//    try {
//        targetImageProc.LoadBmp(targetPath);
//        if (!targetImageProc.m_hDib || targetImageProc.nBitCount != 24)
//        {
//            AfxMessageBox(_T("选择的目标图像不是24位色，请重新选择"));
//            return;
//        }
//    }
//    catch (...) {
//        AfxMessageBox(_T("加载目标图像失败，请检查文件是否有效"));
//        return;
//    }
//
//    int width = pSourceImage->nWidth;
//    int height = pSourceImage->nHeight;
//    int rowSize = ((width * 24 + 31) / 32) * 4;
//    int targetRowSize = ((targetImageProc.nWidth * 24 + 31) / 32) * 4;
//
//    // 对每个颜色通道(R,G,B)分别进行直方图规格化
//    for (int channel = 0; channel < 3; channel++) // 0:B, 1:G, 2:R
//    {
//        // 1. 计算源图像当前通道的直方图
//        std::vector<int> sourceHist(256, 0);
//        // 计算源图像直方图
//        for (int y = 0; y < height; y++)
//        {
//            BYTE* pSource = pSourceImage->pBits + (height - 1 - y) * rowSize;
//            for (int x = 0; x < width; x++)
//            {
//                sourceHist[pSource[x * 3 + channel]]++;
//            }
//        }
//
//        // 2. 计算目标图像当前通道的直方图
//        std::vector<int> targetHist(256, 0);
//        int targetWidth = targetImageProc.nWidth;
//        int targetHeight = targetImageProc.nHeight;
//        for (int y = 0; y < targetHeight; y++)
//        {
//            BYTE* pTarget = targetImageProc.pBits + (targetHeight - 1 - y) * targetRowSize;
//            for (int x = 0; x < targetWidth; x++)
//            {
//                targetHist[pTarget[x * 3 + channel]]++;
//            }
//        }
//
//        // 3. 计算源图像当前通道的CDF
//        std::vector<double> sourceCDF(256, 0);
//        sourceCDF[0] = sourceHist[0];
//        for (int i = 1; i < 256; i++) {
//            sourceCDF[i] = sourceCDF[i - 1] + sourceHist[i];
//        }
//        // 归一化
//        for (int i = 0; i < 256; i++) {
//            sourceCDF[i] /= (width * height);
//        }
//
//        // 4. 计算目标图像当前通道的CDF
//        std::vector<double> targetCDF(256, 0);
//        targetCDF[0] = targetHist[0];
//        for (int i = 1; i < 256; i++) {
//            targetCDF[i] = targetCDF[i - 1] + targetHist[i];
//        }
//        // 归一化
//        for (int i = 0; i < 256; i++) {
//            targetCDF[i] /= (targetWidth * targetHeight);
//        }
//
//        // 5. 创建当前通道的映射表
//        std::vector<BYTE> mapping(256, 0);
//        for (int i = 0; i < 256; i++)
//        {
//            double cdfValue = sourceCDF[i];
//            int j = 255;
//            while (j > 0 && targetCDF[j] > cdfValue + 1e-6) {
//                j--;
//            }
//            mapping[i] = static_cast<BYTE>(j);
//        }
//
//        // 6. 应用映射到源图像当前通道
//        for (int y = 0; y < height; y++)
//        {
//            BYTE* pSource = pSourceImage->pBits + (height - 1 - y) * rowSize;
//            for (int x = 0; x < width; x++)
//            {
//                pSource[x * 3 + channel] = mapping[pSource[x * 3 + channel]];
//            }
//        }
//    }
//
//    // 7. 更新视图
//    Invalidate(TRUE);
//    pDoc->SetModifiedFlag(TRUE);
//    AfxMessageBox(_T("直方图规格化完成，已根据选择的图像调整"));
//}

void CMyPhotoshopView::HistogramMatching()
{
    // 1. 获取文档指针并检查有效性
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid())
    {
        AfxMessageBox(_T("请先打开有效的源图像文件"));
        return;
    }

    // 2. 获取源图像指针并检查位深度
    CImageProc* pSourceImage = pDoc->pImage;
    if (pSourceImage->nBitCount != 24)
    {
        AfxMessageBox(_T("源图像必须是24位色"));
        return;
    }

    // 3. 创建并显示文件选择对话框
    CFileDialog fileDlg(TRUE, _T("bmp"), NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST,
        _T("24位位图文件(*.bmp)|*.bmp|所有文件(*.*)|*.*||"), this);

    if (fileDlg.DoModal() != IDOK)
    {
        return;
    }

    // 4. 创建目标图像处理器并加载图像
    CImageProc targetImageProc;
    CString targetPath = fileDlg.GetPathName();

    try
    {
        targetImageProc.LoadBmp(targetPath);

        if (!targetImageProc.IsValid())
        {
            AfxMessageBox(_T("无法加载目标图像文件"));
            return;
        }

        if (targetImageProc.nBitCount != 24)
        {
            AfxMessageBox(_T("选择的目标图像不是24位色，请重新选择"));
            return;
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        AfxMessageBox(_T("内存不足，无法加载目标图像"));
        return;
    }
    catch (CFileException* e)
    {
        e->Delete();
        AfxMessageBox(_T("文件加载失败，请检查文件是否有效"));
        return;
    }
    catch (...)
    {
        AfxMessageBox(_T("加载目标图像时发生未知错误"));
        return;
    }

    // 5. 获取图像尺寸信息
    int width = pSourceImage->nWidth;
    int height = pSourceImage->nHeight;
    int rowSize = ((width * 24 + 31) / 32) * 4;
    int targetWidth = targetImageProc.nWidth;
    int targetHeight = targetImageProc.nHeight;
    int targetRowSize = ((targetWidth * 24 + 31) / 32) * 4;

    // 6. 显示等待光标
    CWaitCursor waitCursor;

    // 7. 对每个颜色通道进行直方图规格化
    for (int channel = 0; channel < 3; channel++) // B, G, R
    {
        // 7.1 计算直方图
        std::vector<int> sourceHist(256, 0);
        std::vector<int> targetHist(256, 0);

        // 计算源图像直方图
        for (int y = 0; y < height; y++)
        {
            BYTE* pSource = pSourceImage->pBits + (height - 1 - y) * rowSize;
            if (!pSource) continue;

            for (int x = 0; x < width; x++)
            {
                if (x * 3 + channel < rowSize)
                {
                    BYTE val = pSource[x * 3 + channel];
                    sourceHist[val]++;
                }
            }
        }

        // 计算目标图像直方图
        for (int y = 0; y < targetHeight; y++)
        {
            BYTE* pTarget = targetImageProc.pBits + (targetHeight - 1 - y) * targetRowSize;
            if (!pTarget) continue;

            for (int x = 0; x < targetWidth; x++)
            {
                if (x * 3 + channel < targetRowSize)
                {
                    BYTE val = pTarget[x * 3 + channel];
                    targetHist[val]++;
                }
            }
        }

        // 7.2 计算CDF
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

        // 7.3 创建映射表
        std::vector<BYTE> mapping(256, 0);
        for (int i = 0; i < 256; i++)
        {
            double cdf = sourceCDF[i];
            int j = 255;
            while (j > 0 && targetCDF[j] > cdf + 1e-6)
                j--;
            mapping[i] = static_cast<BYTE>(j);
        }

        // 7.4 应用映射
        for (int y = 0; y < height; y++)
        {
            BYTE* pSource = pSourceImage->pBits + (height - 1 - y) * rowSize;
            if (!pSource) continue;

            for (int x = 0; x < width; x++)
            {
                if (x * 3 + channel < rowSize)
                {
                    BYTE origVal = pSource[x * 3 + channel];
                    pSource[x * 3 + channel] = mapping[origVal];
                }
            }
        }
    }

    // 8. 更新视图
    Invalidate(TRUE);
    pDoc->SetModifiedFlag(TRUE);
    AfxMessageBox(_T("直方图规格化完成"), MB_OK | MB_ICONINFORMATION);
}

// 复古风格效果
//复古风格算法实现了以下效果：
//增强红色调(×1.1)
//轻微降低绿色调(×0.9)
//明显降低蓝色调(×0.8)
//添加褐色调(红色 + 20，绿色 + 10)
//添加轻微随机噪点模拟老照片质感
//注意事项
//此功能只支持24位和32位色图像
//应用效果前会检查图像是否有效
//菜单项在没有有效图像时会自动禁用
//修改后的图像会被标记为已修改(SetModifiedFlag)
void CMyPhotoshopView::OnColorStyleVintage()
{
    // TODO: 在此添加命令处理程序代码
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) {
        AfxMessageBox(_T("请先打开有效的图像文件"));
        return;
    }

    // 应用复古风格
    pDoc->pImage->ApplyVintageStyle();

    // 更新视图
    Invalidate(TRUE);
    pDoc->SetModifiedFlag(TRUE);
}


void CMyPhotoshopView::OnUpdateColorStyleVintage(CCmdUI* pCmdUI)
{
    // TODO: 在此添加命令更新用户界面处理程序代码
    CMyPhotoshopDoc* pDoc = GetDocument();
    //pCmdUI->Enable(pDoc && pDoc->pImage && pDoc->pImage->IsValid() &&
    //    (pDoc->pImage->nBitCount == 24 || pDoc->pImage->nBitCount == 32));
    pCmdUI->Enable(pDoc && pDoc->pImage && pDoc->pImage->IsValid());
}
