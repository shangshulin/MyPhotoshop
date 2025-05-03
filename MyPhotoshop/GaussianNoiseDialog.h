// GaussianNoiseDialog.h
#pragma once

class CGaussianNoiseDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CGaussianNoiseDialog)

public:
    CGaussianNoiseDialog(CWnd* pParent = nullptr);
    virtual ~CGaussianNoiseDialog();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GAUSSIAN_NOISE_DIALOG };
#endif

    double GetMean() const { return m_mean; }
    double GetSigma() const { return m_sigma; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    double m_mean;   // 均值
    double m_sigma;  // 标准差
    CSliderCtrl m_meanSlider;
    CSliderCtrl m_sigmaSlider;
    CEdit m_meanEdit;
    CEdit m_sigmaEdit;

    void UpdateEditFromSlider(CSliderCtrl& slider, CEdit& edit, bool isDouble = false, double scale = 1.0);
    void UpdateSliderFromEdit(CSliderCtrl& slider, CEdit& edit, bool isDouble = false, double scale = 1.0);

public:
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEnChangeMeanEdit();
    afx_msg void OnEnChangeSigmaEdit();
};