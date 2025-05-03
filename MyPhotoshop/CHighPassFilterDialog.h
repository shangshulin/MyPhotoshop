#pragma once
#include "afxdialogex.h"
#include "afxwin.h"

// CHighPassFilterDialog 对话框

class CHighPassFilterDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CHighPassFilterDialog)

public:
    CHighPassFilterDialog(CWnd* pParent = nullptr);
    virtual ~CHighPassFilterDialog();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_HIGH_PASS_FILTER_DIALOG };
#endif

    int GetFilterType() const { return m_nFilterType; }
    double GetCutoffFrequency() const { return m_dCutoffFrequency; }
    int GetOrder() const { return m_nOrder; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()

private:
    int m_nFilterType; // 0: 理想高通, 1: 巴特沃斯高通
    double m_dCutoffFrequency;
    int m_nOrder;
    CComboBox m_cmbFilterType;
    CEdit m_edtCutoffFrequency;
    CEdit m_edtOrder;
};
