#pragma once
#include "afxdialogex.h"
#include "resource.h"

// ImpulseNoiseDialog.h


class CImpulseNoiseDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CImpulseNoiseDialog)

public:
    CImpulseNoiseDialog(CWnd* pParent = nullptr);
    virtual ~CImpulseNoiseDialog();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_IMPULSE_NOISE_DIALOG };
#endif

    double GetNoiseRatio() const { return m_noiseRatio; }
    BYTE GetNoiseValue1() const { return m_noiseValue1; }
    BYTE GetNoiseValue2() const { return m_noiseValue2; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    double m_noiseRatio;  // 噪声比例(0.0-1.0)
    BYTE m_noiseValue1;   // 噪声值1
    BYTE m_noiseValue2;   // 噪声值2
    CSliderCtrl m_noiseRatioSlider;
    CSliderCtrl m_noiseValue1Slider;
    CSliderCtrl m_noiseValue2Slider;
    CEdit m_noiseRatioEdit;
    CEdit m_noiseValue1Edit;
    CEdit m_noiseValue2Edit;

    void UpdateEditFromSlider(CSliderCtrl& slider, CEdit& edit, bool isPercentage = false);
    void UpdateSliderFromEdit(CSliderCtrl& slider, CEdit& edit, bool isPercentage = false);

public:
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEnChangeNoiseRatioEdit();
    afx_msg void OnEnChangeNoiseValue1Edit();
    afx_msg void OnEnChangeNoiseValue2Edit();
};