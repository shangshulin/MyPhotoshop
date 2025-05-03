// CHighPassFilterDialog.cpp: 实现文件
//

#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "CHighPassFilterDialog.h"


// CHighPassFilterDialog 对话框

IMPLEMENT_DYNAMIC(CHighPassFilterDialog, CDialogEx)

CHighPassFilterDialog::CHighPassFilterDialog(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_HIGH_PASS_FILTER_DIALOG, pParent)
    , m_nFilterType(0)
    , m_dCutoffFrequency(10.0)
    , m_nOrder(1)
{
}

CHighPassFilterDialog::~CHighPassFilterDialog()
{
}

BOOL CHighPassFilterDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 添加下拉选项
    m_cmbFilterType.AddString(_T("理想高通滤波器"));
    m_cmbFilterType.AddString(_T("巴特沃斯高通滤波器"));

    // 设置默认选择项（可选）
    m_cmbFilterType.SetCurSel(0);

    return TRUE;
}

void CHighPassFilterDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FILTER_TYPE_COMBO, m_cmbFilterType);
    DDX_Control(pDX, IDC_CUTOFF_FREQUENCY_EDIT, m_edtCutoffFrequency);
    DDX_Control(pDX, IDC_ORDER_EDIT, m_edtOrder);
    DDX_CBIndex(pDX, IDC_FILTER_TYPE_COMBO, m_nFilterType);
    DDX_Text(pDX, IDC_CUTOFF_FREQUENCY_EDIT, m_dCutoffFrequency);
    DDX_Text(pDX, IDC_ORDER_EDIT, m_nOrder);
}

BEGIN_MESSAGE_MAP(CHighPassFilterDialog, CDialogEx)
END_MESSAGE_MAP()


// CHighPassFilterDialog 消息处理程序
