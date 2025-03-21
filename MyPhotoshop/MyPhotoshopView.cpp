
// MyPhotoshopView.cpp: CMyPhotoshopView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "MyPhotoshop.h"
#endif

#include "MyPhotoshopDoc.h"
#include "MyPhotoshopView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMyPhotoshopView

IMPLEMENT_DYNCREATE(CMyPhotoshopView, CView)

BEGIN_MESSAGE_MAP(CMyPhotoshopView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
    ON_WM_MOUSEMOVE() // 添加鼠标移动消息映射，用于测试GetPixel
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// CMyPhotoshopView 构造/析构

CMyPhotoshopView::CMyPhotoshopView() noexcept
{
	// TODO: 在此处添加构造代码
    //以下两行用于测试GetPixel
    m_MousePos = CPoint(0, 0); // 初始化鼠标位置
    m_bShiftPressed = false; // 初始化 Shift 键状态
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

//void CMyPhotoshopView::OnDraw(CDC* pDC)
//{
//	CMyPhotoshopDoc* pDoc = GetDocument();
//	ASSERT_VALID(pDoc);
//	if (!pDoc)
//		return;
//
//	// TODO: 在此处为本机数据添加绘制代码
//	if (pDoc->pImage->m_hDib != NULL)
//	{
//		pDoc->pImage->ShowBMP(pDC);
//	}
//}
// 
//这个函数的功能：按住SHIFT，鼠标无需点击即可显示RGB，用来和GetColor的结果作比较表明MyGetPixel函数的正确性
void CMyPhotoshopView::OnDraw(CDC* pDC)
{
    CMyPhotoshopDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc)
        return;

    // 如果位图存在，显示位图
    if (pDoc->pImage->m_hDib != NULL)
    {
        pDoc->pImage->ShowBMP(pDC);

        // 如果 Shift 键按下，显示鼠标位置像素的 RGB 和坐标
        if (m_bShiftPressed)
        {
            // 获取鼠标位置处的像素颜色
            COLORREF color = MyGetPixel(pDC, m_MousePos.x, m_MousePos.y);

            // 检查颜色是否有效
            if (color != CLR_INVALID)
            {
                // 分解颜色值
                BYTE red = GetRValue(color);
                BYTE green = GetGValue(color);
                BYTE blue = GetBValue(color);

                // 在屏幕上显示颜色值和坐标
                CString str;
                str.Format(_T("Pixel at (%d, %d): R=%d, G=%d, B=%d"),
                    m_MousePos.x, m_MousePos.y, red, green, blue);
                pDC->TextOut(10, 10, str); // 在 (10, 10) 处显示信息
            }
            else
            {
                // 如果颜色无效，显示错误信息
                pDC->TextOut(10, 10, _T("Invalid pixel coordinates or device context."));
            }
        }
    }
}

//这个函数用来测试GetPixel功能
void CMyPhotoshopView::OnMouseMove(UINT nFlags, CPoint point)
{
    // 捕获鼠标位置
    m_MousePos = point;

    // 检测 Shift 键是否按下
    m_bShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    // 触发重绘
    Invalidate();

    // 调用基类的默认处理
    CView::OnMouseMove(nFlags, point);
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


void CMyPhotoshopView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	Invalidate(true);
	UpdateWindow();
	if (nFlags & MK_SHIFT)
	{
		CClientDC dc(this);
		CMyPhotoshopDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		pDoc->pImage->GetColor(&dc, point.x, point.y);
		//dc.TextOutW(point.x, point.y,L"Success！");
	}
	CView::OnLButtonDown(nFlags, point);
}

COLORREF CMyPhotoshopView::MyGetPixel(CDC* pDC, int x, int y)
{
    // 检查设备上下文是否有效
    if (pDC == nullptr)
    {
        return CLR_INVALID;
    }

    // 创建一个兼容的内存设备上下文
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);

    // 获取设备上下文的尺寸
    CRect rect;
    pDC->GetClipBox(&rect);

    // 检查坐标是否在有效范围内
    if (x < 0 || x >= rect.Width() || y < 0 || y >= rect.Height())
    {
        return CLR_INVALID;
    }

    // 创建一个兼容的位图
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());

    // 将位图选入内存设备上下文
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // 将设备上下文的内容复制到内存设备上下文中
    memDC.BitBlt(0, 0, rect.Width(), rect.Height(), pDC, 0, 0, SRCCOPY);

    // 获取位图数据
    BITMAP bmpInfo;
    bitmap.GetBitmap(&bmpInfo);

    // 计算位图每行的字节数
    int bytesPerLine = ((bmpInfo.bmWidth * bmpInfo.bmBitsPixel + 31) / 32) * 4;

    // 分配内存以存储位图数据
    BYTE* pBits = new BYTE[bytesPerLine * bmpInfo.bmHeight];
    bitmap.GetBitmapBits(bytesPerLine * bmpInfo.bmHeight, pBits);

    // 计算像素的偏移量
    int offset = y * bytesPerLine + x * (bmpInfo.bmBitsPixel / 8);

    // 提取颜色值
    COLORREF color = RGB(pBits[offset + 2], pBits[offset + 1], pBits[offset]);

    // 清理资源
    delete[] pBits;
    memDC.SelectObject(pOldBitmap);
    bitmap.DeleteObject();
    memDC.DeleteDC();

    return color;
}
