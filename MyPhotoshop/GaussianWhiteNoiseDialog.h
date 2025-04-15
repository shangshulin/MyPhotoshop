#pragma once
#include "afxdialogex.h"


// GaussianWhiteNoiseDialog.h


class CGaussianWhiteNoiseDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CGaussianWhiteNoiseDialog)

public:
    CGaussianWhiteNoiseDialog(CWnd* pParent = nullptr);
    virtual ~CGaussianWhiteNoiseDialog();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GAUSSIAN_WHITE_NOISE_DIALOG };
#endif

    double GetSigma() const { return m_sigma; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    double m_sigma;  // 标准差
    CSliderCtrl m_sigmaSlider;
    CEdit m_sigmaEdit;

    void UpdateEditFromSlider();
    void UpdateSliderFromEdit();

public:
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEnChangeSigmaEdit();
};