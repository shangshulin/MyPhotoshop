// CLOWFILTERDlg.cpp: 实现文件
//

#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "CLOWFILTERDlg.h"

// CLOWFILTERDlg 对话框

IMPLEMENT_DYNAMIC(CLOWFILTERDlg, CDialogEx)

CLOWFILTERDlg::CLOWFILTERDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOW_FILTERDLG, pParent)
	, m_D0(30) // 初始化截止频率为30
	, m_step(2) // 可选：初始化阶数
	, m_pImageProc(nullptr)
{
}

CLOWFILTERDlg::~CLOWFILTERDlg()
{
}

void CLOWFILTERDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// 关联控件
	DDX_Text(pDX, IDC_EDIT_D0, m_D0);
	DDX_Text(pDX, IDC_BW_LOW_FILTER_EDIT, m_step);
}


BEGIN_MESSAGE_MAP(CLOWFILTERDlg, CDialogEx)
	ON_BN_CLICKED(IDC_LOW_FILTER, &CLOWFILTERDlg::OnBnClickedLowFilter)
	ON_BN_CLICKED(IDC_BW_LOW_FILTER, &CLOWFILTERDlg::OnBnClickedBwLowFilter)
	ON_BN_CLICKED(IDC_LOW_FILTER_BUTTON, &CLOWFILTERDlg::OnBnClickedLowFilterButtonApply)
END_MESSAGE_MAP()


// CLOWFILTERDlg 消息处理程序

void CLOWFILTERDlg::SetImageData(CImageProc* pImage)
{
	m_pImageProc = pImage; // 设置图像处理对象
}

int CLOWFILTERDlg::GetFilterType()
{
   // 0表示理想低通滤波器，1表示巴特沃斯低通滤波器
   return (m_step > 1) ? 1 : 0;
}

double CLOWFILTERDlg::GetD0()
{
	// 只返回成员变量
	return m_D0;
}

int CLOWFILTERDlg::GetStep()
{
	return static_cast<int>(m_step);
}

void CLOWFILTERDlg::OnBnClickedLowFilter()
{
	UpdateData(TRUE); // 统一获取界面输入
	if (m_D0 <= 0) {
		AfxMessageBox(_T("截止频率必须大于0！"), MB_ICONERROR);
		return;
	}
	if (m_pImageProc) {
		m_pImageProc->IdealLowPassFilter(m_D0);
		Invalidate(); // 刷新显示
	}
}

void CLOWFILTERDlg::OnBnClickedBwLowFilter()
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
	if (m_pImageProc) {
		m_pImageProc->ButterworthLowPassFilter(m_D0, static_cast<int>(m_step));
		Invalidate();
	}
}

void CLOWFILTERDlg::OnBnClickedLowFilterButtonApply()
{
	UpdateData(TRUE); // 统一获取界面输入

	if (!m_pImageProc) {
		AfxMessageBox(_T("图像处理对象未初始化！"), MB_ICONERROR);
		return;
	}
	if (m_D0 <= 0) {
		AfxMessageBox(_T("截止频率必须大于0！"), MB_ICONERROR);
		return;
	}
	if (m_step <= 0) {
		AfxMessageBox(_T("阶数必须大于0！"), MB_ICONERROR);
		return;
	}

	int filterType = GetFilterType();
	if (filterType == 1) {
		m_pImageProc->ButterworthLowPassFilter(m_D0, static_cast<int>(m_step));
	} else {
		m_pImageProc->IdealLowPassFilter(m_D0);
	}
	Invalidate();
}
