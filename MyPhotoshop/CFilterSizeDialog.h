#pragma once // 防止头文件被多次包含
#include "pch.h" // 预编译头文件
#include "resource.h" // 包含资源定义
#include "CImageProc.h" // 图像处理类头文件
#include "MyPhotoshop.h" // 主程序头文件
#include "afxdialogex.h" // MFC 对话框扩展支持

// CFilterSizeDialog 类声明，继承自 CDialogEx
class CFilterSizeDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CFilterSizeDialog) // 声明动态创建支持（MFC 宏）

public:
    // 构造函数，允许指定父窗口指针，默认为 nullptr
    CFilterSizeDialog(CWnd* pParent = nullptr);

    // 获取滤波器尺寸的公有方法
    int GetFilterSize() const { return m_nFilterSize; }

protected:
    // 重载数据交换函数，用于控件与变量之间的数据同步
    virtual void DoDataExchange(CDataExchange* pDX);

    // 重载对话框初始化函数
    virtual BOOL OnInitDialog();

    // 重载 OK 按钮响应函数
    virtual void OnOK();

    int m_nFilterSize = 3; // 滤波器尺寸成员变量，默认值为 3（3x3）

    DECLARE_MESSAGE_MAP() // 声明消息映射宏
};