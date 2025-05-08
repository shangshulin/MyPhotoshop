#include "pch.h" // 预编译头文件
#include "CFilterSizeDialog.h" // 本类头文件

// 实现 CFilterSizeDialog 的动态创建功能
IMPLEMENT_DYNAMIC(CFilterSizeDialog, CDialogEx)

// 构造函数实现，初始化滤波器尺寸为 3
CFilterSizeDialog::CFilterSizeDialog(CWnd* pParent)
    : CDialogEx(IDD_FILTER_SIZE, pParent) // 调用基类构造函数，指定对话框资源 ID
    , m_nFilterSize(3) // 初始化成员变量
{
}

// 数据交换函数实现，实现控件与成员变量的数据同步
void CFilterSizeDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX); // 调用基类的数据交换
    DDX_Text(pDX, IDC_FILTER_SIZE, m_nFilterSize); // 绑定编辑框控件与 m_nFilterSize
    DDV_MinMaxInt(pDX, m_nFilterSize, 3, 15); // 设置 m_nFilterSize 的有效范围为 3~15
}

// 对话框初始化函数实现
BOOL CFilterSizeDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog(); // 调用基类初始化
    return TRUE; // 返回 TRUE，表示默认焦点处理
}

// OK 按钮响应函数实现
void CFilterSizeDialog::OnOK()
{
    CEdit* pEdit = (CEdit*)GetDlgItem(IDC_FILTER_SIZE); // 获取滤波器尺寸编辑框控件指针
    CString strSize; // 用于存储输入的字符串
    pEdit->GetWindowText(strSize); // 获取编辑框内容

    int size = _ttoi(strSize); // 将字符串转换为整数
    // 检查输入是否合法：3~15之间且为奇数
    if (size < 3 || size > 15 || size % 2 == 0) {
        MessageBox(_T("请输入3-15之间的奇数"), _T("输入错误"), MB_ICONERROR); // 弹出错误提示
        return; // 不关闭对话框
    }

    m_nFilterSize = size; // 设置成员变量
    CDialogEx::OnOK(); // 调用基类 OK，关闭对话框
}

// 消息映射宏实现（本类无自定义消息响应）
BEGIN_MESSAGE_MAP(CFilterSizeDialog, CDialogEx)
END_MESSAGE_MAP()