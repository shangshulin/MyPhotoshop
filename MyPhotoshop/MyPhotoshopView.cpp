
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
#include "CFilterSizeDialog.h"  
#include "CINTENSITYDlg.h"

#include <fftw3.h>
#include "CFreqPassFilterDlg.h"
#include "CSpectrumDlg.h"
#include "NoiseRatioDialog.h"
#include "ImpulseNoiseDialog.h"
#include "GaussianNoiseDialog.h"
#include "GaussianWhiteNoiseDialog.h"

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
    // 像素点信息
    ON_WM_LBUTTONDOWN() // 左键点击
    ON_COMMAND(ID_VIEW_PIXELINFO, &CMyPhotoshopView::OnViewPixelInfo) // 显示像素点信息
    ON_UPDATE_COMMAND_UI(ID_VIEW_PIXELINFO, &CMyPhotoshopView::OnUpdateViewPixelInfo) // 更新像素点信息菜单项状态
    //灰度处理
    ON_COMMAND(ID_INTENSITY_TRANS, &CMyPhotoshopView::OnIntensityTrans)// 灰度变换
    ON_COMMAND(ID_HISTOGRAM_EQUALIZATION, &CMyPhotoshopView::OnHistogramEqualization)// 直方图均衡化
    ON_COMMAND(ID_FUNCTION_HISTOGRAM_MATCHING, &CMyPhotoshopView::OnFunctionHistogramMatching) // 直方图规格化
    ON_COMMAND(ID_COLOR_STYLE_VINTAGE, &CMyPhotoshopView::OnColorStyleVintage)// 复古风格
    ON_COMMAND(ID_STYLE_BLACKWHITE, &CMyPhotoshopView::OnStyleBlackwhite)// 黑白风格
    //边缘检测
    ON_COMMAND(ID_EDGE_SOBEL, &CMyPhotoshopView::OnEdgeDetectionSobel)// 边缘检测-Sobel算子
    ON_COMMAND(ID_EDGE_PREWITT, &CMyPhotoshopView::OnEdgeDetectionPrewitt)// 边缘检测-Prewitt算子
    ON_COMMAND(ID_EDGE_ROBERT, &CMyPhotoshopView::OnEdgeDetectionRobert)// 边缘检测-Robert算子
    ON_COMMAND(ID_EDGE_LAPLACE, &CMyPhotoshopView::OnEdgeDetectionLaplace)// 边缘检测-Laplace算子
    ON_COMMAND(ID_EDGE_CANNY, &CMyPhotoshopView::OnEdgeDetectionCanny)// 边缘检测-Canny算子
    ON_COMMAND(ID_EDGE_LOG, &CMyPhotoshopView::OnEdgeDetectionLog)// 边缘检测-LoG算子
    //图像增强
    ON_COMMAND(ID_ENHANCEMENT, &CMyPhotoshopView::OnEnhancement)// 图像增强
	// 添加噪声
	ON_COMMAND(ID_FUNCTION_SALTANDPEPPER, &CMyPhotoshopView::OnFunctionSaltandpepper)// 添加椒盐噪声
	ON_COMMAND(ID_FUNCTION_IMPULSE, &CMyPhotoshopView::OnFunctionImpulse)// 添加脉冲噪声
    ON_COMMAND(ID_FUNCTION_GAUSSIAN, &CMyPhotoshopView::OnFunctionGaussian)// 添加高斯噪声
    ON_COMMAND(ID_FUNCTION_GAUSSIANWHITE, &CMyPhotoshopView::OnFunctionGaussianwhite)// 添加高斯白噪声
    //空域滤波
    ON_COMMAND(ID_FILTER_MEAN,OnFilterMean)// 均值滤波
	ON_COMMAND(ID_FILTER_MEDIAN, OnFilterMedian)// 中值滤波
	ON_COMMAND(ID_FILTER_MAX, OnFilterMax)// 最大值滤波
    // 频域滤波
    ON_COMMAND(ID_HIGHPASS_FILTER, &CMyPhotoshopView::OnFreqPassFilter)     //高通/低通滤波
    ON_COMMAND(ID_HOMOMORPHIC_FILTERING, &CMyPhotoshopView::OnHomomorphicFiltering)    //同态滤波
	// 撤销操作
    ON_COMMAND(ID_EDIT_UNDO, &CMyPhotoshopView::OnEditUndo)
    // 缩放操作
    ON_WM_MOUSEWHEEL()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_SIZE()
    //FFT与IFFT
    ON_COMMAND(ID_FREQ_FFT, &CMyPhotoshopView::OnFreqFFT)
    ON_COMMAND(ID_FREQ_IFFT, &CMyPhotoshopView::OnFreqIFFT)
    // 图像编码
    ON_COMMAND(ID_HUFFMAN_ENCODE, &CMyPhotoshopView::OnHuffmanEncode)
    ON_COMMAND(ID_HUFFMAN_DECODE, &CMyPhotoshopView::OnHuffmanDecode)
    ON_COMMAND(ID_LZW_ENCODE, &CMyPhotoshopView::OnLZWEncode)
    ON_COMMAND(ID_LZW_DECODE, &CMyPhotoshopView::OnLZWDecode)

END_MESSAGE_MAP()




// CMyPhotoshopView 构造/析构

CMyPhotoshopView::CMyPhotoshopView() noexcept
    : m_bShowPixelInfo(false)
    , m_dZoomRatio(1.0)
    , m_ptScrollPos(0, 0)
    , m_sizeTotal(0, 0)
    , m_nScrollStep(40)  // 默认滚动步长为40像素
{
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

void CMyPhotoshopView::SetZoomRatio(double ratio, CPoint ptCenter)
{
    ratio = max(0.1, min(10.0, ratio)); // 限制缩放范围

    if (ratio == m_dZoomRatio)
        return;

    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
        return;

    CRect rectClient;
    GetClientRect(&rectClient);

    // 如果没有指定中心点，使用当前视图中心
    if (ptCenter.x == -1 || ptCenter.y == -1)
    {
        ptCenter.x = rectClient.Width() / 2;
        ptCenter.y = rectClient.Height() / 2;
    }

    // 计算相对于图像的位置
    double dOldZoom = m_dZoomRatio;
    CPoint ptImage;
    ptImage.x = static_cast<int>((m_ptScrollPos.x + ptCenter.x) / dOldZoom);
    ptImage.y = static_cast<int>((m_ptScrollPos.y + ptCenter.y) / dOldZoom);

    // 更新缩放比例
    m_dZoomRatio = ratio;

    // 计算新的滚动位置
    m_ptScrollPos.x = static_cast<int>(ptImage.x * m_dZoomRatio - ptCenter.x);
    m_ptScrollPos.y = static_cast<int>(ptImage.y * m_dZoomRatio - ptCenter.y);

    // 更新滚动条并重绘
    UpdateScrollBars();
    Invalidate(TRUE);
}

void CMyPhotoshopView::OnDraw(CDC* pDC) {
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
        return;

    // 创建内存DC进行双缓冲
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);

    // 计算绘制区域
    int destWidth = int(pDoc->pImage->nWidth * m_dZoomRatio);
    int destHeight = int(pDoc->pImage->nHeight * m_dZoomRatio);

    // 计算总宽度和图像数量
    int totalWidth = destWidth;
    int imageCount = 1;

    if (pDoc->pImage->HasIFFTResult()) {
        imageCount = 2; // 原图和IFFT结果
    }

    // 计算总宽度（每张图之间留10像素间距）
    totalWidth = destWidth * imageCount + (imageCount - 1) * 10;

    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(pDC, totalWidth, destHeight);
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // 填充背景
    memDC.FillSolidRect(0, 0, totalWidth, destHeight, RGB(255, 255, 255));

    // 1. 绘制原始图像
    if (pDoc->pImage->HasOriginalImageData()) {
        pDoc->pImage->DisplayOriginalImage(&memDC, 0, 0, destWidth, destHeight);
    }
    else {
        pDoc->pImage->ShowBMP(&memDC, 0, 0, destWidth, destHeight);
    }

    // 2. 如果进行了IFFT，显示IFFT结果
    if (pDoc->pImage->HasIFFTResult()) {
        // 创建临时DC用于IFFT结果绘制
        CDC ifftDC;
        ifftDC.CreateCompatibleDC(&memDC);

        CBitmap ifftBmp;
        ifftBmp.CreateCompatibleBitmap(&memDC, destWidth, destHeight);
        CBitmap* pOldIfftBmp = ifftDC.SelectObject(&ifftBmp);

        // 填充IFFT结果背景
        ifftDC.FillSolidRect(0, 0, destWidth, destHeight, RGB(255, 255, 255));

        // 绘制IFFT结果
        pDoc->pImage->DisplayIFFTResult(&ifftDC, 0, 0, destWidth, destHeight);

        // 将IFFT结果拷贝到内存DC
        memDC.BitBlt(destWidth + 10, 0, destWidth, destHeight,
            &ifftDC, 0, 0, SRCCOPY);

        ifftDC.SelectObject(pOldIfftBmp);
    }

    // 从内存DC拷贝到屏幕DC，考虑滚动位置
    CRect rectClient;
    GetClientRect(&rectClient);
    pDC->BitBlt(0, 0, rectClient.Width(), rectClient.Height(),
        &memDC, m_ptScrollPos.x, m_ptScrollPos.y, SRCCOPY);

    memDC.SelectObject(pOldBitmap);
}

// 滚动条处理
void CMyPhotoshopView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO si;// 滚动条信息结构体
    GetScrollInfo(SB_HORZ, &si, SIF_ALL);

    int nPosOld = si.nPos;
    switch (nSBCode)
    {
    case SB_LEFT: si.nPos = si.nMin; break;
    case SB_RIGHT: si.nPos = si.nMax; break;
    case SB_LINELEFT: si.nPos -= m_nScrollStep; break;
    case SB_LINERIGHT: si.nPos += m_nScrollStep; break;
    case SB_PAGELEFT: si.nPos -= si.nPage; break;
    case SB_PAGERIGHT: si.nPos += si.nPage; break;
    case SB_THUMBTRACK: si.nPos = nPos; break;
    }

    si.nPos = max(si.nMin, min(si.nPos, si.nMax - (int)si.nPage + 1));
    if (si.nPos != nPosOld)
    {
        SetScrollPos(SB_HORZ, si.nPos);
        ScrollWindow(nPosOld - si.nPos, 0);
        m_ptScrollPos.x = si.nPos;
    }

    CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMyPhotoshopView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO si;
    GetScrollInfo(SB_VERT, &si, SIF_ALL);

    int nPosOld = si.nPos;
    switch (nSBCode)
    {
    case SB_TOP: si.nPos = si.nMin; break;
    case SB_BOTTOM: si.nPos = si.nMax; break;
    case SB_LINEUP: si.nPos -= m_nScrollStep; break;
    case SB_LINEDOWN: si.nPos += m_nScrollStep; break;
    case SB_PAGEUP: si.nPos -= si.nPage; break;
    case SB_PAGEDOWN: si.nPos += si.nPage; break;
    case SB_THUMBTRACK: si.nPos = nPos; break;
    }

    si.nPos = max(si.nMin, min(si.nPos, si.nMax - (int)si.nPage + 1));
    if (si.nPos != nPosOld)
    {
        SetScrollPos(SB_VERT, si.nPos);
        ScrollWindow(0, nPosOld - si.nPos);
        m_ptScrollPos.y = si.nPos;
    }

    CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CMyPhotoshopView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    UpdateScrollBars();
}

void CMyPhotoshopView::UpdateScrollBars()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage)
        return;

    CRect rectClient;
    GetClientRect(&rectClient);

    // 计算缩放后的图像尺寸
    m_sizeTotal.cx = static_cast<int>(pDoc->pImage->nWidth * m_dZoomRatio);
    m_sizeTotal.cy = static_cast<int>(pDoc->pImage->nHeight * m_dZoomRatio);

    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;

    // 水平滚动条
    si.nMax = m_sizeTotal.cx - 1;
    si.nPage = rectClient.Width();
    si.nPos = m_ptScrollPos.x;
    SetScrollInfo(SB_HORZ, &si, TRUE);

    // 垂直滚动条
    si.nMax = m_sizeTotal.cy - 1;
    si.nPage = rectClient.Height();
    si.nPos = m_ptScrollPos.y;
    SetScrollInfo(SB_VERT, &si, TRUE);

    // 确保滚动位置有效
    m_ptScrollPos.x = min(m_ptScrollPos.x, max(0, m_sizeTotal.cx - rectClient.Width()));
    m_ptScrollPos.y = min(m_ptScrollPos.y, max(0, m_sizeTotal.cy - rectClient.Height()));
}

BOOL CMyPhotoshopView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // 转换为客户区坐标
    ScreenToClient(&pt);

    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        // Ctrl+滚轮：缩放功能
        CPoint ptCenter = pt;
        double zoomFactor = (zDelta > 0) ? 1.1 : 1.0 / 1.1;
        double newZoom = m_dZoomRatio * zoomFactor;
        newZoom = max(0.1, min(10.0, newZoom));
        SetZoomRatio(newZoom, ptCenter);
        return TRUE;
    }
    else if (GetKeyState(VK_SHIFT) & 0x8000)
    {
        // Shift+滚轮：水平滚动
        if (zDelta > 0)
            OnHScroll(SB_LINELEFT, 0, NULL);
        else
            OnHScroll(SB_LINERIGHT, 0, NULL);
        return TRUE;
    }
    else
    {
        // 普通滚轮：垂直滚动
        if (zDelta > 0)
            OnVScroll(SB_LINEUP, 0, NULL);
        else
            OnVScroll(SB_LINEDOWN, 0, NULL);
        return TRUE;
    }

    return CView::OnMouseWheel(nFlags, zDelta, pt);
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

// 左键点击事件处理程序
void CMyPhotoshopView::OnLButtonDown(UINT nFlags, CPoint point)
{
    // 如果启用了显示像素点信息
    if (m_bShowPixelInfo)
    {
        Invalidate(true);
        UpdateWindow();
        if (nFlags & MK_SHIFT)
        {
            CClientDC dc(this);
            CMyPhotoshopDoc* pDoc = GetDocument();
            ASSERT_VALID(pDoc);

            // 加上自定义滚动偏移
            int imgX = int((point.x + m_ptScrollPos.x) / m_dZoomRatio);
            int imgY = int((point.y + m_ptScrollPos.y) / m_dZoomRatio);

            // 判断是否在图像范围内
            if (imgX >= 0 && imgX < pDoc->pImage->nWidth && imgY >= 0 && imgY < pDoc->pImage->nHeight)
            {
                pDoc->pImage->DisplayColor(&dc, imgX, imgY, point.x, point.y);
            }
        }
    }
    CView::OnLButtonDown(nFlags, point);
}



void CMyPhotoshopView::OnHistogramEqualization()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid())
    {
        AfxMessageBox(_T("请先打开有效的图像"));
        return;
    }

    try {
        // 创建原始图像的深拷贝（用于撤回）
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        // 添加命令到命令栈
        AddCommand(
            [pDoc]() {
                // 执行均衡化
                pDoc->pImage->Balance_Transformations();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                // 撤回操作：恢复原始图像
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (CMemoryException* e) {
        e->Delete();
        AfxMessageBox(_T("内存不足，无法完成操作"));
    }
    catch (...) {
        AfxMessageBox(_T("直方图均衡化操作失败"));
    }
}

// 直方图规格化函数（带撤回功能）
void CMyPhotoshopView::OnFunctionHistogramMatching()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid())
    {
        AfxMessageBox(_T("请先打开有效的源图像文件"));
        return;
    }

    CFileDialog fileDlg(TRUE, _T("bmp"), NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST,
        _T("位图文件(*.bmp)|*.bmp|所有文件(*.*)|*.*||"), this);

    if (fileDlg.DoModal() != IDOK)
    {
        return;
    }

    try
    {
        // 创建目标图像和原始图像的拷贝
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        CImageProc* pTargetImage = new CImageProc();
        pTargetImage->LoadBmp(fileDlg.GetPathName());

        if (!pTargetImage->IsValid())
        {
            delete pOldImage;
            delete pTargetImage;
            AfxMessageBox(_T("无法加载目标图像文件"));
            return;
        }

        AddCommand(
            [pDoc, pTargetImage]() {
                pDoc->pImage->HistogramMatching(*pTargetImage);
                delete pTargetImage; // 执行完成后释放目标图像
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        AfxMessageBox(_T("内存不足，无法完成操作"));
    }
    catch (CFileException* e)
    {
        e->Delete();
        AfxMessageBox(_T("文件操作失败"));
    }
    catch (...)
    {
        AfxMessageBox(_T("直方图规格化操作失败"));
    }
}

//灰度变换
void CMyPhotoshopView::OnIntensityTrans()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid())
    {
        AfxMessageBox(_T("请先打开有效的图像"));
        return;
    }

    try {
        // 创建原始图像的深拷贝（用于撤回）
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        // 创建灰度变换对话框
        CINTENSITYDlg dlgIntensity;
        dlgIntensity.SetImageData(pDoc->pImage);

        // 显示对话框并处理结果
        if (dlgIntensity.DoModal() == IDOK)
        {
            // 用户确认操作，添加到命令栈
            AddCommand(
                [pDoc]() {
                    // 执行操作（对话框已修改图像）
                    pDoc->UpdateAllViews(nullptr);
                },
                [pDoc, pOldImage]() {
                    // 撤回操作：恢复原始图像
                    *pDoc->pImage = *pOldImage;
                    delete pOldImage;
                    pDoc->UpdateAllViews(nullptr);
                }
            );
        }
        else
        {
            // 用户取消操作，释放备份图像
            delete pOldImage;
        }
    }
    catch (CMemoryException* e) {
        e->Delete();
        AfxMessageBox(_T("内存不足，无法完成操作"));
    }
    catch (...) {
        AfxMessageBox(_T("灰度变换操作失败"));
    }
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

// 复古风格
void CMyPhotoshopView::OnColorStyleVintage()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) {
        AfxMessageBox(_T("请先打开有效的图像文件"));
        return;
    }

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplyVintageStyle();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}


// 黑白风格
void CMyPhotoshopView::OnStyleBlackwhite()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplyBlackAndWhiteStyle();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}

// 椒盐噪声
void CMyPhotoshopView::OnFunctionSaltandpepper()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) return;

    CNoiseRatioDialog dlg;
    if (dlg.DoModal() != IDOK) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        double saltRatio = dlg.GetSaltRatio();

        AddCommand(
            [pDoc, saltRatio]() {
                pDoc->pImage->AddSaltAndPepperNoise(0.05, saltRatio);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}


void CMyPhotoshopView::OnFunctionImpulse()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) return;

    CImpulseNoiseDialog dlg;
    if (dlg.DoModal() != IDOK) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        double noiseRatio = dlg.GetNoiseRatio();
        BYTE noiseValue1 = dlg.GetNoiseValue1();
        BYTE noiseValue2 = dlg.GetNoiseValue2();

        AddCommand(
            [pDoc, noiseRatio, noiseValue1, noiseValue2]() {
                pDoc->pImage->AddImpulseNoise(noiseRatio, noiseValue1, noiseValue2);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}


void CMyPhotoshopView::OnFunctionGaussian()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) return;

    CGaussianNoiseDialog dlg;
    if (dlg.DoModal() != IDOK) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        double mean = dlg.GetMean();
        double sigma = dlg.GetSigma();

        AddCommand(
            [pDoc, mean, sigma]() {
                pDoc->pImage->AddGaussianNoise(mean, sigma);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}


void CMyPhotoshopView::OnFunctionGaussianwhite()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) return;

    CGaussianWhiteNoiseDialog dlg;
    if (dlg.DoModal() != IDOK) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        double sigma = dlg.GetSigma();

        AddCommand(
            [pDoc, sigma]() {
                pDoc->pImage->AddGaussianWhiteNoise(sigma);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}

void CMyPhotoshopView::OnFilterMean()
{
    CFilterSizeDialog dlg;
    if (dlg.DoModal() != IDOK) return;

    CMyPhotoshopDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc || !pDoc->pImage) return;

    try {
        // 创建深拷贝
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        // 验证拷贝是否正确
        ASSERT(pOldImage->nWidth == pDoc->pImage->nWidth);
        ASSERT(pOldImage->nHeight == pDoc->pImage->nHeight);
        ASSERT(pOldImage->nBitCount == pDoc->pImage->nBitCount);

        int filterSize = dlg.GetFilterSize();

        AddCommand(
            [pDoc, filterSize]() {
                pDoc->pImage->SpatialFilter(filterSize, FilterType::Mean);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                // 确保恢复前后尺寸一致
                ASSERT(pOldImage->nWidth == pDoc->pImage->nWidth);
                ASSERT(pOldImage->nHeight == pDoc->pImage->nHeight);

                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (CMemoryException* e) {
        e->ReportError();
        e->Delete();
        AfxMessageBox(_T("内存不足，无法完成操作"));
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}

void CMyPhotoshopView::OnFilterMedian()
{
    CFilterSizeDialog dlg;
    if (dlg.DoModal() != IDOK) return;

    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        int filterSize = dlg.GetFilterSize();

        AddCommand(
            [pDoc, filterSize]() {
                pDoc->pImage->SpatialFilter(filterSize, FilterType::Mean);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}

void CMyPhotoshopView::OnFilterMax()
{
    CFilterSizeDialog dlg;
    if (dlg.DoModal() != IDOK) return;

    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        int filterSize = dlg.GetFilterSize();

        AddCommand(
            [pDoc, filterSize]() {
                pDoc->pImage->SpatialFilter(filterSize, FilterType::Max);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("操作失败"));
    }
}


void CMyPhotoshopView::OnEdgeDetectionSobel()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplySobelEdgeDetection();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("Sobel边缘检测操作失败"));
    }
}

void CMyPhotoshopView::OnEdgeDetectionPrewitt()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplyPrewittEdgeDetection();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("Prewitt边缘检测操作失败"));
    }
}

// Robert边缘检测
void CMyPhotoshopView::OnEdgeDetectionRobert()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplyRobertEdgeDetection();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("Robert边缘检测操作失败"));
    }
}

// Canny边缘检测
void CMyPhotoshopView::OnEdgeDetectionCanny()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplyCannyEdgeDetection();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("Canny边缘检测操作失败"));
    }
}


//LoG边缘检测
void CMyPhotoshopView::OnEdgeDetectionLog()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplyLoGEdgeDetection();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("LoG边缘检测操作失败"));
    }
}

// Laplace边缘检测
void CMyPhotoshopView::OnEdgeDetectionLaplace()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->pImage->ApplyLaplaceEdgeDetection();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("Laplace边缘检测操作失败"));
    }
}

// 图像增强
void CMyPhotoshopView::OnEnhancement()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) return;

    try {
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                pDoc->ApplyImageEnhancement();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("图像增强操作失败"));
    }
}

void CMyPhotoshopView::OnEditUndo()
{
    if (!m_commandStack.empty())
    {
        CCommand* pCommand = m_commandStack.top();
        m_commandStack.pop();
        pCommand->Undo(); // 撤回操作
        delete pCommand;  // 删除命令对象
    }
}


void CMyPhotoshopView::OnFreqFFT() {
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage) {
        AfxMessageBox(_T("请先打开有效的图像文件"));
        return;
    }

    try {
        // 保存原始图像状态
        CImageProc* pOldImage = new CImageProc();
        if (!pOldImage) AfxThrowMemoryException();
        *pOldImage = *pDoc->pImage;

        // 添加到命令栈
        AddCommand(
            [pDoc]() {
                // 执行FFT
                if (pDoc->pImage->FFT2D(true)) {
                    // 显示频谱对话框
                    CSpectrumDlg dlg(AfxGetMainWnd(), pDoc->pImage);
                    dlg.DoModal();

                    pDoc->UpdateAllViews(nullptr);
                }
                else if (!pDoc->pImage->FFT2D(true)) {
                    AfxMessageBox(_T("FFT变换失败"));
                }
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->pImage->ResetFFTState();
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (CMemoryException* e) {
        e->Delete();
        AfxMessageBox(_T("内存不足，无法保存图像状态"));
    }
    catch (...) {
        AfxMessageBox(_T("FFT变换初始化失败"));
    }
}

void CMyPhotoshopView::OnFreqIFFT() {
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsFFTPerformed()) {
        AfxMessageBox(_T("请先执行FFT变换"));
        return;
    }

    try {
        // 保存当前状态用于撤销
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        AddCommand(
            [pDoc]() {
                if (!pDoc->pImage->IFFT2D(false)) {
                    AfxMessageBox(_T("IFFT变换失败"));
                }
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                pDoc->pImage->ResetFFTState();
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (...) {
        AfxMessageBox(_T("IFFT变换失败"));
    }
}

void CMyPhotoshopView::OnFreqPassFilter()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) {
        AfxMessageBox(_T("请先打开有效图像"));
        return;
    }

    CFreqPassFilterDlg dlg;
    dlg.SetImageData(pDoc->pImage);
    if (dlg.DoModal() == IDOK) {
        double D0 = dlg.m_D0;
        int filterType = dlg.m_high_filter_type;
        int step = dlg.m_step;
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        AddCommand(
            [pDoc, D0, filterType, step]() {
                pDoc->pImage->FreqPassFilter(D0, step, filterType);
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
}


// 同态滤波
void CMyPhotoshopView::OnHomomorphicFiltering() {
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) {
        AfxMessageBox(_T("请先打开有效的图像"));
        return;
    }

    try {
        // 创建原始图像的深拷贝（用于撤回）
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;

        // 添加命令到命令栈
        AddCommand(
            [pDoc]() {
                // 执行同态滤波
                pDoc->pImage->HomomorphicFiltering();
                pDoc->UpdateAllViews(nullptr);
            },
            [pDoc, pOldImage]() {
                // 撤回操作：恢复原始图像
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
    catch (CMemoryException* e) {
        e->Delete();
        AfxMessageBox(_T("内存不足，无法完成操作"));
    }
    catch (...) {
        AfxMessageBox(_T("同态滤波操作失败"));
    }
}

void CMyPhotoshopView::OnHuffmanEncode()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) {
        AfxMessageBox(_T("请先打开有效的图像"));
        return;
    }
    // 弹出“另存为”对话框
    CFileDialog fileDlg(FALSE, _T("huff"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("Huffman编码文件(*.huff)|*.huff|所有文件(*.*)|*.*||"), this);
    if (fileDlg.DoModal() == IDOK)
    {
        CString savePath = fileDlg.GetPathName();
        // 假设m_imageProc是你的CImageProc对象
        if (!pDoc->pImage->HuffmanEncodeImage(savePath))
        {
            AfxMessageBox(_T("保存失败！"));
        }
        else
        {
            AfxMessageBox(_T("保存成功！"));
        }
    }
}

void CMyPhotoshopView::OnHuffmanDecode()
{
    CFileDialog fileDlg(TRUE, _T("huff"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
        _T("Huffman编码文件(*.huff)|*.huff|所有文件(*.*)|*.*||"), this);
    if (fileDlg.DoModal() == IDOK)
    {
        CString filePath = fileDlg.GetPathName();
        CMyPhotoshopDoc* pDoc = GetDocument();
        if (!pDoc || !pDoc->pImage)
        {
            AfxMessageBox(_T("文档或图像对象无效"));
            return;
        }
        if (!pDoc->pImage->HuffmanDecodeImage(filePath))
        {
            AfxMessageBox(_T("解码失败或文件格式错误！"));
        }
        else
        {
            AfxMessageBox(_T("解码成功！"));
            pDoc->UpdateAllViews(nullptr); // 刷新显示
        }
    }
}

// LZW编码
void CMyPhotoshopView::OnLZWEncode()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc)
        return;

    if (!pDoc->pImage || !pDoc->pImage->IsValid()) {
        AfxMessageBox(_T("请先打开一个图像！"));
        return;
    }

    // 创建保存文件对话框
    CFileDialog dlg(FALSE, _T("lzw"), _T("output.lzw"),
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("LZW编码文件 (*.lzw)|*.lzw|所有文件 (*.*)|*.*||"), this);

    if (dlg.DoModal() == IDOK) {
        CString savePath = dlg.GetPathName();
        
        // 保存当前状态用于撤销
        CImageProc* pOldImage = new CImageProc();
        *pOldImage = *pDoc->pImage;
        
        // 执行LZW编码
        AddCommand(
            [pDoc, savePath]() {
                bool result = pDoc->pImage->LZWEncodeImage(savePath);
                if (result) {
                    AfxMessageBox(_T("LZW编码成功！"));
                } else {
                    AfxMessageBox(_T("LZW编码失败！"));
                }
                return result;
            },
            [pDoc, pOldImage]() {
                *pDoc->pImage = *pOldImage;
                delete pOldImage;
                pDoc->UpdateAllViews(nullptr);
            }
        );
    }
}

// LZW解码
void CMyPhotoshopView::OnLZWDecode()
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc)
        return;

    // 不再检查是否有有效图像
    
    // 创建打开文件对话框
    CFileDialog dlg(TRUE, _T("lzw"), NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        _T("LZW encoded files (*.lzw)|*.lzw|All files (*.*)|*.*||"), this);

    if (dlg.DoModal() == IDOK) {
        CString openPath = dlg.GetPathName();
        
        // 执行LZW解码
        bool result = pDoc->pImage->LZWDecodeImage(openPath);
        if (result) {
            // 刷新视图
            Invalidate();
            
            // 重置滚动条和缩放比例
            m_dZoomRatio = 1.0;
            m_ptScrollPos = CPoint(0, 0);
            UpdateScrollBars();
        }
    }
}