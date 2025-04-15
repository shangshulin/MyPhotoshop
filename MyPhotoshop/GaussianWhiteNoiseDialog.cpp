// GaussianWhiteNoiseDialog.cpp: 实现文件
//

#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "GaussianWhiteNoiseDialog.h"


// GaussianWhiteNoiseDialog.cpp


IMPLEMENT_DYNAMIC(CGaussianWhiteNoiseDialog, CDialogEx)

CGaussianWhiteNoiseDialog::CGaussianWhiteNoiseDialog(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_GAUSSIAN_WHITE_NOISE_DIALOG, pParent)
    , m_sigma(30.0)
{
}

CGaussianWhiteNoiseDialog::~CGaussianWhiteNoiseDialog()
{
}

void CGaussianWhiteNoiseDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SIGMA_SLIDER, m_sigmaSlider);
    DDX_Control(pDX, IDC_SIGMA_EDIT, m_sigmaEdit);
}

BEGIN_MESSAGE_MAP(CGaussianWhiteNoiseDialog, CDialogEx)
    ON_WM_HSCROLL()
    ON_EN_CHANGE(IDC_SIGMA_EDIT, &CGaussianWhiteNoiseDialog::OnEnChangeSigmaEdit)
END_MESSAGE_MAP()

BOOL CGaussianWhiteNoiseDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 初始化滑块范围
    m_sigmaSlider.SetRange(1, 100);
    m_sigmaSlider.SetPos(static_cast<int>(m_sigma));

    // 初始化编辑框
    CString strValue;
    strValue.Format(_T("%.1f"), m_sigma);
    m_sigmaEdit.SetWindowText(strValue);

    return TRUE;
}

void CGaussianWhiteNoiseDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar == (CScrollBar*)&m_sigmaSlider)
    {
        m_sigma = m_sigmaSlider.GetPos();
        CString strValue;
        strValue.Format(_T("%.1f"), m_sigma);
        m_sigmaEdit.SetWindowText(strValue);
    }

    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CGaussianWhiteNoiseDialog::OnEnChangeSigmaEdit()
{
    CString strValue;
    m_sigmaEdit.GetWindowText(strValue);
    double value = _ttof(strValue);
    value = max(1.0, min(100.0, value));
    m_sigma = value;
    m_sigmaSlider.SetPos(static_cast<int>(value));
}