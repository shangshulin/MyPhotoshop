#pragma once
#include "afxdialogex.h"
#include "CImageProc.h"
#include "resource.h"

class CSpectrumDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CSpectrumDlg)

public:
    CSpectrumDlg(CWnd* pParent = nullptr, CImageProc* pImageProc = nullptr);
    virtual ~CSpectrumDlg();

    enum { IDD = IDD_SPECTRUM_DLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();

    DECLARE_MESSAGE_MAP()

private:
    CImageProc* m_pImageProc;
    CStatic m_spectrumView;
};