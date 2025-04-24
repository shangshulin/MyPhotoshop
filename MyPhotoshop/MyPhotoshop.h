
// MyPhotoshop.h: MyPhotoshop 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"       // 主符号


// CMyPhotoshopApp:
// 有关此类的实现，请参阅 MyPhotoshop.cpp
//

class CMyPhotoshopApp : public CWinApp
{
public:
	CMyPhotoshopApp() noexcept;


// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// 实现
	afx_msg void OnAppAbout();
	afx_msg void OnHistogramMix();//直方图混合显示模式菜单项的处理函数
	afx_msg void OnHistogramRGB();//直方图RGB显示模式菜单项的处理函数
	DECLARE_MESSAGE_MAP()
};

extern CMyPhotoshopApp theApp;
