#pragma once
#include "afxdialogex.h"
#include"CImageProc.h"

// CINTENSITYDlg 对话框

class CINTENSITYDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CINTENSITYDlg)

public:
	CINTENSITYDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CINTENSITYDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_INTENSITY_DLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	double m_alpha;  // 灰度变换参数alpha
	double m_beta;   // 灰度变换参数beta
	CImageProc* m_pImage; // 指向图像处理对象的指针
	void SetImageData(CImageProc* pImage); // 设置图像数据
	void ApplyIntensityTransform(); // 应用灰度线性变换
	CEdit m_editAlpha;
	CEdit m_editBeta;
	afx_msg void OnBnClickedButtonApply(); // 应用按钮点击事件
	afx_msg void OnBnClickedButtonCancel(); // 取消按钮点击事件
	virtual BOOL OnInitDialog();
};
