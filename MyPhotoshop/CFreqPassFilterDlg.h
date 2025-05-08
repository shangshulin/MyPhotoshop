#pragma execution_character_set("utf-8")
#include "afxdialogex.h"
#include "CImageProc.h"

// CFreqPassFilterDlg 对话框

class CFreqPassFilterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFreqPassFilterDlg)

public:
	CFreqPassFilterDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFreqPassFilterDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HIGH_FILTERDLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CImageProc* m_pImageProc; // 指向图像处理对象的指针
	double m_D0;  // 截止频率
	double m_step; // 巴特沃斯滤波阶数
	void SetImageData(CImageProc* pImage); // 设置图像数据
	int m_high_filter_type;
	afx_msg void OnBnClickedHighFilterButtonApply(); // 应用按钮点击事件
	virtual BOOL OnInitDialog();
};