#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "NoiseRatioDialog.h"

IMPLEMENT_DYNAMIC(CNoiseRatioDialog, CDialogEx)

CNoiseRatioDialog::CNoiseRatioDialog(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_NOISE_RATIO_DIALOG, pParent)
    , m_saltRatio(0.5) // 默认50%盐噪声
{
}

CNoiseRatioDialog::~CNoiseRatioDialog()
{
}

void CNoiseRatioDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SALT_RATIO_SLIDER, m_saltRatioSlider);
    DDX_Control(pDX, IDC_SALT_RATIO_EDIT, m_saltRatioEdit);
}

BEGIN_MESSAGE_MAP(CNoiseRatioDialog, CDialogEx)
    ON_WM_HSCROLL()
    ON_EN_CHANGE(IDC_SALT_RATIO_EDIT, &CNoiseRatioDialog::OnEnChangeSaltRatioEdit)
END_MESSAGE_MAP()

BOOL CNoiseRatioDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 初始化滑块范围(0-100)
    m_saltRatioSlider.SetRange(0, 100);
    m_saltRatioSlider.SetPos(static_cast<int>(m_saltRatio * 100));

    // 初始化编辑框
    CString strRatio;
    strRatio.Format(_T("%.0f%%"), m_saltRatio * 100);
    m_saltRatioEdit.SetWindowText(strRatio);

    return TRUE;
}

void CNoiseRatioDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar == (CScrollBar*)&m_saltRatioSlider)
    {
        int pos = m_saltRatioSlider.GetPos();
        m_saltRatio = pos / 100.0;

        CString strRatio;
        strRatio.Format(_T("%.0f%%"), m_saltRatio * 100);
        m_saltRatioEdit.SetWindowText(strRatio);
    }

    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CNoiseRatioDialog::OnEnChangeSaltRatioEdit()
{
    CString strRatio;
    m_saltRatioEdit.GetWindowText(strRatio);

    strRatio.Remove('%');

    double ratio = _ttof(strRatio) / 100.0;
    ratio = max(0.0, min(1.0, ratio));

    m_saltRatio = ratio;
    m_saltRatioSlider.SetPos(static_cast<int>(ratio * 100));
}
