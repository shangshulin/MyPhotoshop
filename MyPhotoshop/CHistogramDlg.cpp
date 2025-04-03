﻿// CHistogramDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "CHistogramDlg.h"
#include "CImageProc.h"
#include "MyPhotoshopDoc.h"
#include <numeric> // for std::accumulate and std::round>
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CHistogramDlg 对话框
CHistogramDlg::CHistogramDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_HISTOGRAM_DLG, pParent)
    , m_histogramData(256, 0)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CHistogramDlg::~CHistogramDlg()
{
}

void CHistogramDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_HISTOGRAM, m_StaticHistogram); // 绑定Picture Control
}

BEGIN_MESSAGE_MAP(CHistogramDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

// CHistogramDlg 消息处理程序

BOOL CHistogramDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);         // 设置大图标
    SetIcon(m_hIcon, FALSE);        // 设置小图标

    // TODO: 在此添加额外的初始化代码

    return TRUE;  // 返回 TRUE 除非您设置焦点到控件
}

void CHistogramDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    if (IsIconic())
    {
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        // 获取 Picture Control 的矩形区域
        CRect plotRect;
        m_StaticHistogram.GetWindowRect(&plotRect);
        ScreenToClient(&plotRect);

        // 清除背景
        dc.FillSolidRect(plotRect, RGB(255, 255, 255)); // 白色背景

        // 绘制坐标轴
        CPen penBlack(PS_SOLID, 1, RGB(0, 0, 0));
        CPen* pOldPen = dc.SelectObject(&penBlack);

        // X 轴
        dc.MoveTo(plotRect.left, plotRect.bottom);
        dc.LineTo(plotRect.right, plotRect.bottom);

        // Y 轴
        dc.MoveTo(plotRect.left, plotRect.top);
        dc.LineTo(plotRect.left, plotRect.bottom);

        // 计算总像素数
        int totalPixels = std::accumulate(m_histogramData.begin(), m_histogramData.end(), 0);

        // 避免除以零的情况
        if (totalPixels == 0)
        {
            dc.SelectObject(pOldPen);
            CDialogEx::OnPaint();
            return;
        }

         //绘制 X 轴刻度和标签
        for (int i = 0; i <= 256; i += 32)
        {
            int xPos = plotRect.left + (i * (plotRect.Width())) / 256;
            dc.MoveTo(xPos, plotRect.bottom);
            dc.LineTo(xPos, plotRect.bottom + 5);
            CString strLabel;
            strLabel.Format(_T("%d"), i);
            dc.TextOutW(xPos - 10, plotRect.bottom + 10, strLabel);
        }
 
        // 获取最大值
        int maxElement = *std::max_element(m_histogramData.begin(), m_histogramData.end());

        // 计算归一化直方图的最大值
        double maxYValue = static_cast<double>(maxElement) / totalPixels;

        // 绘制 Y 轴刻度和标签
        int numTicks = 10; // 假设有10个刻度
        for (int i = 0; i <= numTicks; ++i)
        {
            // 计算每个刻度的位置（均匀分布在绘图区域内）
            int yPos = plotRect.bottom - (i * plotRect.Height() / numTicks);

            // 绘制刻度线
            dc.MoveTo(plotRect.left, yPos);
            dc.LineTo(plotRect.left - 5, yPos);

            // 显示刻度标签（百分比形式）
            double valuePercent = static_cast<double>(i) / numTicks * 100; // 百分比值
            CString strLabel;
            strLabel.Format(_T("%.0f%%"), valuePercent); // 格式化为字符串
            dc.TextOutW(plotRect.left - 40, yPos - 10, strLabel); // 在刻度旁显示标签
        }

        // 绘制归一化的直方图
        for (int i = 0; i < 256; ++i)
        {
            // 归一化数据值到 [0, 1] 范围
            double normalizedValue = static_cast<double>(m_histogramData[i]) / totalPixels;

            // 将归一化值映射到绘图区域的高度
            int barHeight = static_cast<int>(std::round(normalizedValue * plotRect.Height()));

            // 定义条形图的矩形区域
            CRect barRect(
                plotRect.left + i * (plotRect.Width()) / 256,          // 左边
                plotRect.bottom - barHeight,                          // 上边
                plotRect.left + (i + 1) * (plotRect.Width()) / 256,   // 右边
                plotRect.bottom                                         // 下边
            );

            // 绘制条形图（蓝色填充）
            dc.FillSolidRect(barRect, RGB(0, 0, 255));
        }

        dc.SelectObject(pOldPen);
        CDialogEx::OnPaint();
    }
}

// 用户拖动最小化窗口时系统调用此函数取得光标
// 显示。
HCURSOR CHistogramDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon); // 返回图标
}

void CHistogramDlg::SetHistogramData(const std::vector<int>& histogramData)
{
    m_histogramData = histogramData;// 将传入的直方图数据赋值给成员变量m_histogramData
}





// //CHistogramDlg.cpp : implementation file
//
//
//#include "pch.h"
//#include "framework.h"
//#include "MyPhotoshop.h"
//#include "afxdialogex.h"
//#include "CHistogramDlg.h"
//#include "CImageProc.h"
//#include "MyPhotoshopDoc.h"
//
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif
//
//// CHistogramDlg 对话框
//
//CHistogramDlg::CHistogramDlg(CWnd* pParent /*=nullptr*/)
//    : CDialogEx(IDD_HISTOGRAM_DLG, pParent)
//    , m_histogramData(256, 0)
//{
//    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
//}
//
//CHistogramDlg::~CHistogramDlg()
//{
//}
//
//void CHistogramDlg::DoDataExchange(CDataExchange* pDX)
//{
//    CDialogEx::DoDataExchange(pDX);
//    DDX_Control(pDX, IDC_STATIC_HISTOGRAM, m_StaticHistogram); // 绑定Picture Control
//}
//
//BEGIN_MESSAGE_MAP(CHistogramDlg, CDialogEx)
//    ON_WM_SYSCOMMAND()
//    ON_WM_PAINT()
//    ON_WM_QUERYDRAGICON()
//END_MESSAGE_MAP()
//
//// CHistogramDlg 消息处理程序
//
//BOOL CHistogramDlg::OnInitDialog()
//{
//    CDialogEx::OnInitDialog();
//
//    // 设置图标。当应用程序主窗口不是对话框时，框架将自动
//    //  执行此操作
//    SetIcon(m_hIcon, TRUE);         // 设置大图标
//    SetIcon(m_hIcon, FALSE);        // 设置小图标
//
//    // TODO: 在此添加额外的初始化代码
//
//    return TRUE;  // 返回 TRUE 除非您设置焦点到控件
//}
//
//void CHistogramDlg::OnPaint()
//{
//    CPaintDC dc(this); // device context for painting
//
//    if (IsIconic())
//    {
//        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
//
//        // Center icon in client rectangle
//        int cxIcon = GetSystemMetrics(SM_CXICON);
//        int cyIcon = GetSystemMetrics(SM_CYICON);
//        CRect rect;
//        GetClientRect(&rect);
//        int x = (rect.Width() - cxIcon + 1) / 2;
//        int y = (rect.Height() - cyIcon + 1) / 2;
//
//        // Draw the icon
//        dc.DrawIcon(x, y, m_hIcon);
//    }
//    else
//    {
//        // 获取 Picture Control 的矩形区域
//        CRect plotRect;
//        m_StaticHistogram.GetWindowRect(&plotRect);
//        ScreenToClient(&plotRect);
//
//        // 留白空间
//        int margin = 20;
//        plotRect.DeflateRect(margin, margin, margin, margin);
//
//        // 清除背景
//        dc.FillSolidRect(plotRect, RGB(255, 255, 255)); // 白色背景
//
//        // 绘制坐标轴
//        CPen penBlack(PS_SOLID, 1, RGB(0, 0, 0));
//        CPen* pOldPen = dc.SelectObject(&penBlack);
//
//        // X 轴
//        dc.MoveTo(plotRect.left, plotRect.bottom);
//        dc.LineTo(plotRect.right, plotRect.bottom);
//
//        // Y 轴
//        dc.MoveTo(plotRect.left, plotRect.top);
//        dc.LineTo(plotRect.left, plotRect.bottom);
//
//        // 绘制 X 轴刻度和标签
//        for (int i = 0; i <= 256; i += 32)
//        {
//            int xPos = plotRect.left + (i * (plotRect.Width())) / 256;
//            dc.MoveTo(xPos, plotRect.bottom);
//            dc.LineTo(xPos, plotRect.bottom + 5);
//            CString strLabel;
//            strLabel.Format(_T("%d"), i);
//            dc.TextOutW(xPos - 10, plotRect.bottom + 10, strLabel);
//        }
//
//        // 绘制 Y 轴刻度和标签
//        int maxCount = *std::max_element(m_histogramData.begin(), m_histogramData.end());
//        for (int i = 0; i <= maxCount; i += max(1, maxCount / 10))
//        {
//            int yPos = plotRect.bottom - (i * (plotRect.Height())) / maxCount;
//            dc.MoveTo(plotRect.left, yPos);
//            dc.LineTo(plotRect.left - 5, yPos);
//            CString strLabel;
//            strLabel.Format(_T("%d"), i);
//            dc.TextOutW(plotRect.left - 40, yPos - 10, strLabel);
//        }
//
//        // 绘制直方图
//        for (int i = 0; i < 256; ++i)
//        {
//            int barHeight = static_cast<int>((static_cast<double>(m_histogramData[i]) / maxCount) * (plotRect.Height()));
//            CRect barRect(plotRect.left + i * (plotRect.Width()) / 256, plotRect.bottom - barHeight,
//                plotRect.left + (i + 1) * (plotRect.Width()) / 256, plotRect.bottom);
//            dc.FillSolidRect(barRect, RGB(0, 0, 255)); // 蓝色条形图
//        }
//
//        dc.SelectObject(pOldPen);
//        CDialogEx::OnPaint();
//    }
//}
//
//// 用户拖动最小化窗口时系统调用此函数取得光标
//// 显示。
//HCURSOR CHistogramDlg::OnQueryDragIcon()
//{
//    return static_cast<HCURSOR>(m_hIcon); // 返回图标
//}
//
//void CHistogramDlg::SetHistogramData(const std::vector<int>& histogramData)
//{
//    m_histogramData = histogramData;
//}
//
//

