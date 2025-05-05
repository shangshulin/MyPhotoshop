#pragma once
#include "afxdialogex.h"
#include "CImageProc.h"

// CLOWFILTERDlg 对话框
class CLOWFILTERDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CLOWFILTERDlg)

public:
    CLOWFILTERDlg(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CLOWFILTERDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_LOW_FILTERDLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    CImageProc* m_pImageProc; // 指向图像处理对象的指针
    double m_D0;  // 截止频率
    double m_step; // 巴特沃斯滤波阶数
    void SetImageData(CImageProc* pImage); // 设置图像数据
    int GetFilterType(); // 获取滤波器类型
    double GetD0(); // 获取截止频率
    int GetStep(); // 获取巴特沃斯滤波阶数
    afx_msg void OnBnClickedLowFilter();
    afx_msg void OnBnClickedBwLowFilter();
    afx_msg void OnBnClickedLowFilterButtonApply();
};