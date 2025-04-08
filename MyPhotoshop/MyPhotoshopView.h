
// MyPhotoshopView.h: CMyPhotoshopView 类的接口
//

#pragma once
#include "afxwin.h"
#include "CImageProc.h"
#include "CHistogramDlg.h"

class CMyPhotoshopView : public CView
{
protected:
	bool m_bShowPixelInfo; // 控制是否显示像素点信息

protected: // 仅从序列化创建
	CMyPhotoshopView() noexcept;
	DECLARE_DYNCREATE(CMyPhotoshopView)

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

// 实现
public:
	virtual ~CMyPhotoshopView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()// 生成消息映射函数
public:
	afx_msg void OnViewPixelInfo(); // 响应菜单按钮点击的消息处理函数
	afx_msg void OnUpdateViewPixelInfo(CCmdUI* pCmdUI); // 更新菜单按钮状态的消息处理函数
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point); // 鼠标左键按下事件处理函数
	afx_msg void OnFunctionHistogramMatching(); // 直方图规格化菜单项的处理函数
	afx_msg void OnColorStyleVintage();
	afx_msg void OnUpdateColorStyleVintage(CCmdUI* pCmdUI);
};

#ifndef _DEBUG  // MyPhotoshopView.cpp 中的调试版本
inline CMyPhotoshopDoc* CMyPhotoshopView::GetDocument() const
   { return reinterpret_cast<CMyPhotoshopDoc*>(m_pDocument); }
#endif

