// FFTLogDialog.h
#pragma once
#include "afxdialogex.h"

class CFFTLogDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CFFTLogDialog)

public:
    CFFTLogDialog(CWnd* pParent = nullptr);
    virtual ~CFFTLogDialog();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_FFTLOG_DIALOG };
#endif

    double m_dLogBase;    // 对数底数
    double m_dScaleFactor; // 缩放因子

protected:
    virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    // 移除 OnEnChangeFilterSize 声明（如果存在）
};