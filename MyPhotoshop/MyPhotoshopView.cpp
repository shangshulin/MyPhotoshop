
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
	// 像素点信息
	ON_WM_LBUTTONDOWN() // 左键点击
	ON_COMMAND(ID_VIEW_PIXELINFO, &CMyPhotoshopView::OnViewPixelInfo) // 显示像素点信息
	ON_UPDATE_COMMAND_UI(ID_VIEW_PIXELINFO, &CMyPhotoshopView::OnUpdateViewPixelInfo) // 更新像素点信息菜单项状态
	//灰度处理
	ON_COMMAND(ID_FUNCTION_HISTOGRAM_MATCHING, &CMyPhotoshopView::OnFunctionHistogramMatching) // 直方图规格化
	ON_COMMAND(ID_COLOR_STYLE_VINTAGE, &CMyPhotoshopView::OnColorStyleVintage)// 复古风格
	ON_COMMAND(ID_STYLE_BLACKWHITE, &CMyPhotoshopView::OnStyleBlackwhite)// 黑白风格
	//边缘检测
	ON_COMMAND(ID_EDGE_SOBEL, &CMyPhotoshopView::OnEdgeDetectionSobel)// 边缘检测-Sobel算子
	ON_COMMAND(ID_EDGE_PREWITT, &CMyPhotoshopView::OnEdgeDetectionPrewitt)// 边缘检测-Prewitt算子
	ON_COMMAND(ID_EDGE_ROBERT, &CMyPhotoshopView::OnEdgeDetectionRobert)// 边缘检测-Robert算子
	ON_COMMAND(ID_EDGE_LAPLACE, &CMyPhotoshopView::OnEdgeDetectionLaplace)// 边缘检测-Laplace算子

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
			CMyPhotoshopDoc* pDoc = GetDocument();// 获取文档中的图像数据
			ASSERT_VALID(pDoc);
			pDoc->pImage->GetColor(&dc, point.x, point.y);
		}
	}
	CView::OnLButtonDown(nFlags, point);
}

// 直方图规格化函数
void CMyPhotoshopView::OnFunctionHistogramMatching()
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid())
	{
		AfxMessageBox(_T("请先打开有效的源图像文件"));
		return;
	}

	CImageProc* pSourceImage = pDoc->pImage;

	CFileDialog fileDlg(TRUE, _T("bmp"), NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST,
		_T("位图文件(*.bmp)|*.bmp|所有文件(*.*)|*.*||"), this);

	if (fileDlg.DoModal() != IDOK)
	{
		return;
	}

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

	if (pSourceImage->HistogramMatching(targetImageProc))
	{
		Invalidate(TRUE);
		pDoc->SetModifiedFlag(TRUE);
		AfxMessageBox(_T("直方图规格化完成"), MB_OK | MB_ICONINFORMATION);
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
	// TODO: 在此添加命令处理程序代码
	CMyPhotoshopDoc* pDoc = GetDocument();
	if (!pDoc || !pDoc->pImage || !pDoc->pImage->IsValid()) {// 检查文档和图像有效性
		AfxMessageBox(_T("请先打开有效的图像文件"));
		return;
	}

	// 应用复古风格
	pDoc->pImage->ApplyVintageStyle();

	// 视图重绘
	Invalidate(); // 使视图无效，触发重绘
	UpdateWindow(); // 立即更新窗口
}


// 黑白风格
void CMyPhotoshopView::OnStyleBlackwhite()
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	if (pDoc->pImage)
	{
		pDoc->pImage->ApplyBlackAndWhiteStyle(); // 应用黑白风格

		// 视图重绘
		Invalidate(); // 使视图无效，触发重绘
		UpdateWindow(); // 立即更新窗口
	}
}

void CMyPhotoshopView::OnEdgeDetectionSobel()
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	if (pDoc->pImage)
	{
		pDoc->pImage->ApplySobelEdgeDetection(); // 应用Sobel边缘检测
		// 视图重绘
		Invalidate(); // 使视图无效，触发重绘
		UpdateWindow(); // 立即更新窗口
	}
}

void CMyPhotoshopView::OnEdgeDetectionPrewitt()
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	if (pDoc->pImage)
	{
		pDoc->pImage->ApplyPrewittEdgeDetection(); // 应用Prewitt边缘检测
		// 视图重绘
		Invalidate(); // 使视图无效，触发重绘
		UpdateWindow(); // 立即更新窗口
	}
}
// Robert边缘检测
void CMyPhotoshopView::OnEdgeDetectionRobert()
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	if (pDoc->pImage)
	{
		pDoc->pImage->ApplyRobertEdgeDetection(); // 应用Robert边缘检测
		// 视图重绘
		Invalidate(); // 使视图无效，触发重绘
		UpdateWindow(); // 立即更新窗口
	}
}

// Laplace边缘检测
void CMyPhotoshopView::OnEdgeDetectionLaplace()
{
	CMyPhotoshopDoc* pDoc = GetDocument();
	if (pDoc->pImage)
	{
		pDoc->pImage->ApplyLaplaceEdgeDetection(); // 应用Laplace边缘检测
		// 视图重绘
		Invalidate(); // 使视图无效，触发重绘
		UpdateWindow(); // 立即更新窗口
	}
}