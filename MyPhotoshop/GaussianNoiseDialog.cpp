// GaussianNoiseDialog.cpp: 实现文件
//

#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "GaussianNoiseDialog.h"




IMPLEMENT_DYNAMIC(CGaussianNoiseDialog, CDialogEx)

CGaussianNoiseDialog::CGaussianNoiseDialog(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_GAUSSIAN_NOISE_DIALOG, pParent)
    , m_mean(0.0)
    , m_sigma(30.0)
{
}

CGaussianNoiseDialog::~CGaussianNoiseDialog()
{
}

void CGaussianNoiseDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MEAN_SLIDER, m_meanSlider);
    DDX_Control(pDX, IDC_SIGMA_SLIDER, m_sigmaSlider);
    DDX_Control(pDX, IDC_MEAN_EDIT, m_meanEdit);
    DDX_Control(pDX, IDC_SIGMA_EDIT, m_sigmaEdit);
}

BEGIN_MESSAGE_MAP(CGaussianNoiseDialog, CDialogEx)
    ON_WM_HSCROLL()
    ON_EN_CHANGE(IDC_MEAN_EDIT, &CGaussianNoiseDialog::OnEnChangeMeanEdit)
    ON_EN_CHANGE(IDC_SIGMA_EDIT, &CGaussianNoiseDialog::OnEnChangeSigmaEdit)
END_MESSAGE_MAP()

BOOL CGaussianNoiseDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 初始化滑块范围
    m_meanSlider.SetRange(-50, 50);
    m_meanSlider.SetPos(static_cast<int>(m_mean));

    m_sigmaSlider.SetRange(1, 100);
    m_sigmaSlider.SetPos(static_cast<int>(m_sigma));

    // 初始化编辑框
    CString strValue;
    strValue.Format(_T("%.1f"), m_mean);
    m_meanEdit.SetWindowText(strValue);

    strValue.Format(_T("%.1f"), m_sigma);
    m_sigmaEdit.SetWindowText(strValue);

    return TRUE;
}

void CGaussianNoiseDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar == (CScrollBar*)&m_meanSlider)
    {
        m_mean = m_meanSlider.GetPos();
        CString strValue;
        strValue.Format(_T("%.1f"), m_mean);
        m_meanEdit.SetWindowText(strValue);
    }
    else if (pScrollBar == (CScrollBar*)&m_sigmaSlider)
    {
        m_sigma = m_sigmaSlider.GetPos();
        CString strValue;
        strValue.Format(_T("%.1f"), m_sigma);
        m_sigmaEdit.SetWindowText(strValue);
    }

    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CGaussianNoiseDialog::OnEnChangeMeanEdit()
{
    CString strValue;
    m_meanEdit.GetWindowText(strValue);
    double value = _ttof(strValue);
    value = max(-50.0, min(50.0, value));
    m_mean = value;
    m_meanSlider.SetPos(static_cast<int>(value));
}

void CGaussianNoiseDialog::OnEnChangeSigmaEdit()
{
    CString strValue;
    m_sigmaEdit.GetWindowText(strValue);
    double value = _ttof(strValue);
    value = max(1.0, min(100.0, value));
    m_sigma = value;
    m_sigmaSlider.SetPos(static_cast<int>(value));
}