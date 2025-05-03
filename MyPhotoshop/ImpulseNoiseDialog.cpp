// ImpulseNoiseDialog.cpp
#include "pch.h"
#include "ImpulseNoiseDialog.h"

IMPLEMENT_DYNAMIC(CImpulseNoiseDialog, CDialogEx)

CImpulseNoiseDialog::CImpulseNoiseDialog(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_IMPULSE_NOISE_DIALOG, pParent)
    , m_noiseRatio(0.05)
    , m_noiseValue1(0)
    , m_noiseValue2(255)
{
}

CImpulseNoiseDialog::~CImpulseNoiseDialog()
{
}

void CImpulseNoiseDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_NOISE_RATIO_SLIDER, m_noiseRatioSlider);
    DDX_Control(pDX, IDC_NOISE_VALUE1_SLIDER, m_noiseValue1Slider);
    DDX_Control(pDX, IDC_NOISE_VALUE2_SLIDER, m_noiseValue2Slider);
    DDX_Control(pDX, IDC_NOISE_RATIO_EDIT, m_noiseRatioEdit);
    DDX_Control(pDX, IDC_NOISE_VALUE1_EDIT, m_noiseValue1Edit);
    DDX_Control(pDX, IDC_NOISE_VALUE2_EDIT, m_noiseValue2Edit);
}

BEGIN_MESSAGE_MAP(CImpulseNoiseDialog, CDialogEx)
    ON_WM_HSCROLL()
    ON_EN_CHANGE(IDC_NOISE_RATIO_EDIT, &CImpulseNoiseDialog::OnEnChangeNoiseRatioEdit)
    ON_EN_CHANGE(IDC_NOISE_VALUE1_EDIT, &CImpulseNoiseDialog::OnEnChangeNoiseValue1Edit)
    ON_EN_CHANGE(IDC_NOISE_VALUE2_EDIT, &CImpulseNoiseDialog::OnEnChangeNoiseValue2Edit)
END_MESSAGE_MAP()

BOOL CImpulseNoiseDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 初始化滑块
    m_noiseRatioSlider.SetRange(0, 100);
    m_noiseRatioSlider.SetPos(static_cast<int>(m_noiseRatio * 100));

    m_noiseValue1Slider.SetRange(0, 255);
    m_noiseValue1Slider.SetPos(m_noiseValue1);

    m_noiseValue2Slider.SetRange(0, 255);
    m_noiseValue2Slider.SetPos(m_noiseValue2);

    // 初始化编辑框
    CString strValue;
    strValue.Format(_T("%.0f%%"), m_noiseRatio * 100);
    m_noiseRatioEdit.SetWindowText(strValue);

    strValue.Format(_T("%d"), m_noiseValue1);
    m_noiseValue1Edit.SetWindowText(strValue);

    strValue.Format(_T("%d"), m_noiseValue2);
    m_noiseValue2Edit.SetWindowText(strValue);

    return TRUE;
}

void CImpulseNoiseDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar == (CScrollBar*)&m_noiseRatioSlider)
    {
        int pos = m_noiseRatioSlider.GetPos();
        m_noiseRatio = pos / 100.0;
        CString strValue;
        strValue.Format(_T("%.0f%%"), m_noiseRatio * 100);
        m_noiseRatioEdit.SetWindowText(strValue);
    }
    else if (pScrollBar == (CScrollBar*)&m_noiseValue1Slider)
    {
        m_noiseValue1 = static_cast<BYTE>(m_noiseValue1Slider.GetPos());
        CString strValue;
        strValue.Format(_T("%d"), m_noiseValue1);
        m_noiseValue1Edit.SetWindowText(strValue);
    }
    else if (pScrollBar == (CScrollBar*)&m_noiseValue2Slider)
    {
        m_noiseValue2 = static_cast<BYTE>(m_noiseValue2Slider.GetPos());
        CString strValue;
        strValue.Format(_T("%d"), m_noiseValue2);
        m_noiseValue2Edit.SetWindowText(strValue);
    }

    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CImpulseNoiseDialog::OnEnChangeNoiseRatioEdit()
{
    CString strValue;
    m_noiseRatioEdit.GetWindowText(strValue);
    strValue.Remove('%');
    double value = _ttof(strValue) / 100.0;
    value = max(0.0, min(1.0, value));
    m_noiseRatio = value;
    m_noiseRatioSlider.SetPos(static_cast<int>(value * 100));
}

void CImpulseNoiseDialog::OnEnChangeNoiseValue1Edit()
{
    CString strValue;
    m_noiseValue1Edit.GetWindowText(strValue);
    int value = _ttoi(strValue);
    value = max(0, min(255, value));
    m_noiseValue1 = static_cast<BYTE>(value);
    m_noiseValue1Slider.SetPos(value);
}

void CImpulseNoiseDialog::OnEnChangeNoiseValue2Edit()
{
    CString strValue;
    m_noiseValue2Edit.GetWindowText(strValue);
    int value = _ttoi(strValue);
    value = max(0, min(255, value));
    m_noiseValue2 = static_cast<BYTE>(value);
    m_noiseValue2Slider.SetPos(value);
}