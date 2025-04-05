
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