#pragma once
#include "afxwin.h"

// CNoiseRatioDialog 对话框
class CNoiseRatioDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CNoiseRatioDialog)

public:
    CNoiseRatioDialog(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CNoiseRatioDialog();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_NOISE_RATIO_DIALOG };
#endif

    double GetSaltRatio() const { return m_saltRatio; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    CSliderCtrl m_saltRatioSlider;
    CEdit m_saltRatioEdit;
    double m_saltRatio;  // 盐噪声比例(0.0-1.0)

public:
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEnChangeSaltRatioEdit();
};
