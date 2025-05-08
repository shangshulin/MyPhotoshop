#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "FilterSizeDialog.h"
//#include "CImageProc.h"


IMPLEMENT_DYNAMIC(CFilterSizeDialog, CDialogEx)

CFilterSizeDialog::CFilterSizeDialog(CWnd* pParent)
    : CDialogEx(IDD_FILTER_SIZE, pParent)
    , m_nFilterSize(3)
{
}

void CFilterSizeDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_FILTER_SIZE, m_nFilterSize);
    DDV_MinMaxInt(pDX, m_nFilterSize, 3, 15);
}

BOOL CFilterSizeDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    return TRUE;
}

void CFilterSizeDialog::OnOK()
{
    CEdit* pEdit = (CEdit*)GetDlgItem(IDC_FILTER_SIZE);
    CString strSize;
    pEdit->GetWindowText(strSize);

    int size = _ttoi(strSize);
    if (size < 3 || size > 15 || size % 2 == 0) {
        MessageBox(_T("请输入3-15之间的奇数"), _T("输入错误"), MB_ICONERROR);
        return;
    }

    m_nFilterSize = size;
    CDialogEx::OnOK();
}

BEGIN_MESSAGE_MAP(CFilterSizeDialog, CDialogEx)
END_MESSAGE_MAP()