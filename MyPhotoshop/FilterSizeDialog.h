#pragma once
#include "pch.h"
#include "resource.h"

class CFilterSizeDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CFilterSizeDialog)
public:
    CFilterSizeDialog(CWnd* pParent = nullptr);
    int GetFilterSize() const { return m_nFilterSize; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    int m_nFilterSize = 3; // д╛хо3x3
    DECLARE_MESSAGE_MAP()
};