
#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "CHighPassFilterDlg.h"

// CHighPassFilterDlg 对话框

IMPLEMENT_DYNAMIC(CHighPassFilterDlg, CDialogEx)

CHighPassFilterDlg::CHighPassFilterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HIGH_FILTERDLG, pParent),
	m_D0(50),  // 截止频率初始化为50
	m_step(1),  // 巴特沃斯滤波阶数初始化为1
	m_pImageProc(nullptr) // 图像处理对象指针初始化为nullptr
{

}

CHighPassFilterDlg::~CHighPassFilterDlg()
{
}

void CHighPassFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// 关联控件
	DDX_Text(pDX, IDC_EDIT_D0_HIGH, m_D0);
	DDX_Text(pDX, IDC_BW_HIGH_FILTER_EDIT, m_step);
}


BEGIN_MESSAGE_MAP(CHighPassFilterDlg, CDialogEx)
	ON_BN_CLICKED(IDC_HIGH_FILTER, &CHighPassFilterDlg::OnBnClickedHighFilter)
	ON_BN_CLICKED(IDC_BW_HIGH_FILTER, &CHighPassFilterDlg::OnBnClickedBwHighFilter)
	ON_BN_CLICKED(IDC_HIGH_FILTER_BUTTON, &CHighPassFilterDlg::OnBnClickedHighFilterButtonApply)
END_MESSAGE_MAP()


// CHighPassFilterDlg 消息处理程序

void CHighPassFilterDlg::SetImageData(CImageProc* pImage)
{
	m_pImageProc = pImage; // 设置图像处理对象
}

int CHighPassFilterDlg::GetFilterType()
{
	// 0表示理想高通滤波器，1表示巴特沃斯高通滤波器
	return (m_step > 1) ? 1 : 0;
}

double CHighPassFilterDlg::GetD0()
{
	// 只返回成员变量
	return m_D0;
}

int CHighPassFilterDlg::GetStep()
{
	return static_cast<int>(m_step);
}

void CHighPassFilterDlg::OnBnClickedHighFilter()
{
	UpdateData(TRUE); // 统一获取界面输入
	if (m_D0 <= 0) {
		AfxMessageBox(_T("截止频率必须大于0！"), MB_ICONERROR);
		return;
	}
	UpdateData(FALSE); // 刷新控件显示
}

void CHighPassFilterDlg::OnBnClickedBwHighFilter()
{
	UpdateData(TRUE); // 统一获取界面输入
	if (m_D0 <= 0) {
		AfxMessageBox(_T("截止频率必须大于0！"), MB_ICONERROR);
		return;
	}
	if (m_step <= 0) {
		AfxMessageBox(_T("阶数必须大于0！"), MB_ICONERROR);
		return;
	}
	UpdateData(FALSE); // 刷新控件显示
}

void CHighPassFilterDlg::OnBnClickedHighFilterButtonApply()
{
	UpdateData(TRUE);
	// 此处不做滤波，只关闭对话框，参数通过成员变量传递
	Invalidate(); // 刷新显示
	EndDialog(IDOK);
}