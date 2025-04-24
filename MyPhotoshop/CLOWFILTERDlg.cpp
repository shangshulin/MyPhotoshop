// CLOWFILTERDlg.cpp: 实现文件
//

#include "pch.h"
#include "MyPhotoshop.h"
#include "afxdialogex.h"
#include "CLOWFILTERDlg.h"

// CLOWFILTERDlg 对话框

IMPLEMENT_DYNAMIC(CLOWFILTERDlg, CDialogEx)

CLOWFILTERDlg::CLOWFILTERDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOW_FILTERDLG, pParent),
	m_D0(0.0),  // 截止频率初始化为0
	m_step(1),  // 巴特沃斯滤波阶数初始化为1
	m_pImageProc(nullptr) // 图像处理对象指针初始化为nullptr
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
   // 根据用户输入确定滤波器类型
   UpdateData(TRUE); // 获取界面输入
   
   // 返回滤波器类型，不在此处执行滤波操作
   // 0表示理想低通滤波器，1表示巴特沃斯低通滤波器
   return (m_step > 1) ? 1 : 0;
}

double CLOWFILTERDlg::GetD0()
{
	// 获取用户输入的截止频率
	UpdateData(TRUE); // 从界面控件获取数据
	// 检查截止频率是否有效
	if (m_D0 <= 0) {
		AfxMessageBox(_T("截止频率必须大于0！"), MB_ICONERROR);
		return -1; // 返回无效值
	}
	return m_D0; // 返回有效的截止频率
}

int CLOWFILTERDlg::GetStep()
{
	UpdateData(TRUE); // 从界面控件获取数据

	// 检查阶数是否有效
	if (m_step <= 0) {
		AfxMessageBox(_T("阶数必须大于0！"), MB_ICONERROR);
		return -1; // 返回无效值
	}
	return m_step; // 返回有效阶数
}

void CLOWFILTERDlg::OnBnClickedLowFilter()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE); // 获取界面输入
	if (m_pImageProc) {
		m_pImageProc->IdealLowPassFilter(m_D0);
		Invalidate(); // 刷新显示
	}
}

void CLOWFILTERDlg::OnBnClickedBwLowFilter()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_pImageProc) {
		m_pImageProc->ButterworthLowPassFilter(m_D0, m_step);
		Invalidate();
	}
}


void CLOWFILTERDlg::OnBnClickedLowFilterButtonApply()
{
	// 获取用户输入的截止频率和阶数
	UpdateData(TRUE);

	// 检查图像处理对象是否存在
	if (!m_pImageProc) {
		AfxMessageBox(_T("图像处理对象未初始化！"), MB_ICONERROR);
		return;
	}

	// 检查截止频率和阶数的有效性
	double d0 = GetD0();
	int step = GetStep();
	if (d0 <= 0 || step <= 0) {
		// 错误信息已在 GetD0 和 GetStep 中显示
		return;
	}

	// 根据用户选择的滤波器类型应用滤波
	int filterType = GetFilterType();
	if (filterType == 1) {
		// 应用巴特沃斯低通滤波器
		m_pImageProc->ButterworthLowPassFilter(d0, step);
	} else {
		// 应用理想低通滤波器
		m_pImageProc->IdealLowPassFilter(d0);
	}

	// 刷新显示
	Invalidate();
	AfxMessageBox(_T("滤波器已成功应用！"), MB_ICONINFORMATION);
}
