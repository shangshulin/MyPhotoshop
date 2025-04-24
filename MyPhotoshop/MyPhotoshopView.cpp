
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
#include "FilterSizeDialog.h"  

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
	ON_COMMAND(ID_FILTER_MEDIAN, OnFilterMedian)//   中值滤波
	ON_COMMAND(ID_FILTER_MAX, OnFilterMax)// 最大值滤波
	// 撤销操作
    ON_COMMAND(ID_EDIT_UNDO, &CMyPhotoshopView::OnEditUndo)
    // 缩放操作
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()




// CMyPhotoshopView 构造/析构

CMyPhotoshopView::CMyPhotoshopView() noexcept
    : m_bShowPixelInfo(false), m_dZoomRatio(1.0) // 默认缩放比例为1.0
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

void CMyPhotoshopView::SetZoomRatio(double ratio)
{
    m_dZoomRatio = ratio;
    Invalidate(); // 触发重绘
}

double CMyPhotoshopView::GetZoomRatio() const
{
    return m_dZoomRatio;
}

void CMyPhotoshopView::OnDraw(CDC* pDC)
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
	if (pDoc->pImage->m_hDib != NULL)
	{
        int destWidth = int(pDoc->pImage->nWidth * m_dZoomRatio);
        int destHeight = int(pDoc->pImage->nHeight * m_dZoomRatio);
        // 计算居中显示的左上角坐标
        CRect clientRect;
        GetClientRect(&clientRect);
        int x = (clientRect.Width() - destWidth) / 2;
        int y = (clientRect.Height() - destHeight) / 2;
        pDoc->pImage->ShowBMP(pDC, x, y, destWidth, destHeight);
	}
}

BOOL CMyPhotoshopView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // 检查Ctrl键是否按下
    if (GetKeyState(VK_CONTROL) & 0x8000)
    {
        if (zDelta > 0)
            m_dZoomRatio *= 1.1; // 放大
        else
            m_dZoomRatio /= 1.1; // 缩小

        // 限制缩放比例范围
        if (m_dZoomRatio < 0.1) m_dZoomRatio = 0.1;
        if (m_dZoomRatio > 10.0) m_dZoomRatio = 10.0;

        Invalidate(); // 触发重绘
        return TRUE;  // 已处理
    }
    // 未按Ctrl时，调用基类默认行为
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
            CMyPhotoshopDoc* pDoc = GetDocument();
            ASSERT_VALID(pDoc);

            int destWidth = int(pDoc->pImage->nWidth * m_dZoomRatio);
            int destHeight = int(pDoc->pImage->nHeight * m_dZoomRatio);
            CRect clientRect;
            GetClientRect(&clientRect);
            int x = (clientRect.Width() - destWidth) / 2;
            int y = (clientRect.Height() - destHeight) / 2;

            if (point.x >= x && point.x < x + destWidth &&
                point.y >= y && point.y < y + destHeight)
            {
                int imgX = int((point.x - x) / m_dZoomRatio);
                int imgY = int((point.y - y) / m_dZoomRatio);
                // 传递窗口坐标point.x, point.y用于文本显示
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
                if (pDoc->pImage->HistogramMatching(*pTargetImage))
                {
                    pDoc->SetModifiedFlag(TRUE);
                    pDoc->UpdateAllViews(nullptr);
                    AfxMessageBox(_T("直方图规格化完成"), MB_OK | MB_ICONINFORMATION);
                }
                delete pTargetImage; // 执行完成后释放目标图像
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
                pDoc->pImage->MeanFilter(filterSize);
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
                pDoc->pImage->MedianFilter(filterSize);
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
                pDoc->pImage->MaxFilter(filterSize);
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
