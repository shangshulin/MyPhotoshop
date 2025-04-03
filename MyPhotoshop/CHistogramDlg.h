﻿// CHistogramDlg.h : header file
//

#pragma once
#include <vector>

// CHistogramDlg dialog
class CHistogramDlg : public CDialogEx
{
    // Construction
public:
    CHistogramDlg(CWnd* pParent = nullptr);   // standard constructor
    ~CHistogramDlg();

    // Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_HISTOGRAM_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

private:
    std::vector<int> m_histogramData;
    CStatic m_StaticHistogram; // 添加 Picture Control 控件

public:
    void SetHistogramData(const std::vector<int>& histogramData);
};



