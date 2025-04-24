
// MyPhotoshopView.h: CMyPhotoshopView 类的接口
//

#pragma once
#include "afxwin.h"
#include "CImageProc.h"
#include "CHistogramDlg.h"
#include "NoiseRatioDialog.h"
#include "ImpulseNoiseDialog.h"
#include "GaussianNoiseDialog.h"
#include "GaussianWhiteNoiseDialog.h"
#include "Command.h"
#include <stack>

class CMyPhotoshopView : public CView
{
protected:
	bool m_bShowPixelInfo; // 控制是否显示像素点信息
	double m_dZoomRatio; // 缩放比例，1.0为原始大小

public:
	void SetZoomRatio(double ratio);
	double GetZoomRatio() const;
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

protected: // 仅从序列化创建
	CMyPhotoshopView() noexcept;
	DECLARE_DYNCREATE(CMyPhotoshopView)
	std::stack<CCommand*> m_commandStack; // 命令栈，用于存储命令对象

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
	//灰度处理
	afx_msg void OnHistogramEqualization();//直方图均衡化菜单项的处理函数
	afx_msg void OnFunctionHistogramMatching(); // 直方图规格化菜单项的处理函数
	afx_msg void OnColorStyleVintage();
	afx_msg void OnStyleBlackwhite();
	//添加噪声
	afx_msg void OnFunctionSaltandpepper();
	afx_msg void OnFunctionImpulse();
	afx_msg void OnFunctionGaussian();
	afx_msg void OnFunctionGaussianwhite();
	//空域滤波
	afx_msg void OnFilterMean();
	afx_msg void OnFilterMedian();
	afx_msg void OnFilterMax();
	//边缘检测
	afx_msg void OnEdgeDetectionSobel();//Sobel边缘检测菜单项的处理函数
	afx_msg void OnEdgeDetectionPrewitt();//Prewitt边缘检测菜单项的处理函数
	afx_msg void OnEdgeDetectionRobert();//Robert边缘检测菜单项的处理函数
	afx_msg void OnEdgeDetectionCanny();//Canny边缘检测菜单项的处理函数
	afx_msg void OnEdgeDetectionLaplace();//Laplace边缘检测菜单项的处理函数
	afx_msg void OnEdgeDetectionLog();//LoG边缘检测菜单项的处理函数
	//图像增强
	afx_msg void OnEnhancement();// 图像增强菜单项的处理函数
public:
	template <typename TExecute, typename TUndo>
	void AddCommand(TExecute&& executeFunc, TUndo&& undoFunc)
	{
		CCommand* pCommand = new CGenericCommand(
			std::forward<TExecute>(executeFunc),
			std::forward<TUndo>(undoFunc));
		pCommand->Execute(); // 立即执行操作
		m_commandStack.push(pCommand);
	}


	afx_msg void OnEditUndo();
};

#ifndef _DEBUG  // MyPhotoshopView.cpp 中的调试版本
inline CMyPhotoshopDoc* CMyPhotoshopView::GetDocument() const
   { return reinterpret_cast<CMyPhotoshopDoc*>(m_pDocument); }
#endif

