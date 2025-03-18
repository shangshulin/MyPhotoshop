
// MyPhotoshopView.h: CMyPhotoshopView 类的接口
//

#pragma once


class CMyPhotoshopView : public CView
{
protected: // 仅从序列化创建
	CMyPhotoshopView() noexcept;
	DECLARE_DYNCREATE(CMyPhotoshopView)
	//以下三行用于测试GetPixel功能
	CPoint m_MousePos; // 存储鼠标位置
	bool m_bShiftPressed; // 标记 Shift 键是否按下
	afx_msg void OnMouseMove(UINT nFlags, CPoint point); //用于捕获鼠标位置和Shift键状态
// 特性
public:
	CMyPhotoshopDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	COLORREF MyGetPixel(CDC* pDC, int x, int y); // 手动实现的 GetPixel

// 实现
public:
	virtual ~CMyPhotoshopView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

#ifndef _DEBUG  // MyPhotoshopView.cpp 中的调试版本
inline CMyPhotoshopDoc* CMyPhotoshopView::GetDocument() const
   { return reinterpret_cast<CMyPhotoshopDoc*>(m_pDocument); }
#endif

