// MyPhotoshop.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "MyPhotoshop.h"
#include "MainFrm.h"

#include "MyPhotoshopDoc.h"
#include "MyPhotoshopView.h"
#include "CHistogramDlg.h"
#include "CINTENSITYDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMyPhotoshopApp

BEGIN_MESSAGE_MAP(CMyPhotoshopApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CMyPhotoshopApp::OnAppAbout) // “关于”菜单项的 ID
	// 基于文件的标准文档命令
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew) // 新建文件命令
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen) // 打开文件命令
	// 标准打印设置命令
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup) // 设置打印
	ON_COMMAND(ID_HISTOGRAM_MIX, &CMyPhotoshopApp::OnHistogramMix) // 直方图混合模式
	ON_COMMAND(ID_HISTOGRAM_RGB, &CMyPhotoshopApp::OnHistogramRGB) // 直方图RGB模式
	ON_COMMAND(ID_HISTOGRAM_EQUALIZATION, &CMyPhotoshopApp::OnHistogramEqualization)// 直方图均衡化
	ON_COMMAND(ID_INTENSITY_TRANS, &CMyPhotoshopApp::OnIntensityTrans) // 灰度线性变换

END_MESSAGE_MAP()


// CMyPhotoshopApp 构造

CMyPhotoshopApp::CMyPhotoshopApp() noexcept
{
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// 如果应用程序是利用公共语言运行时支持(/clr)构建的，则: 
	//     1) 必须有此附加设置，“重新启动管理器”支持才能正常工作。
	//     2) 在您的项目中，您必须按照生成顺序向 System.Windows.Forms 添加引用。
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: 将以下应用程序 ID 字符串替换为唯一的 ID 字符串；建议的字符串格式
	//为 CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("MyPhotoshop.AppID.NoVersion"));

	// TODO:  在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}

// 唯一的 CMyPhotoshopApp 对象

CMyPhotoshopApp theApp;


// CMyPhotoshopApp 初始化

BOOL CMyPhotoshopApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// 初始化 OLE 库
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// 使用 RichEdit 控件需要 AfxInitRichEdit2()
	// AfxInitRichEdit2();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
	LoadStdProfileSettings(4);  // 加载标准 INI 文件选项(包括 MRU)


	// 注册应用程序的文档模板。  文档模板
	// 将用作文档、框架窗口和视图之间的连接
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMyPhotoshopDoc),
		RUNTIME_CLASS(CMainFrame),       // 主 SDI 框架窗口
		RUNTIME_CLASS(CMyPhotoshopView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// 分析标准 shell 命令、DDE、打开文件操作的命令行
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);



	// 调度在命令行中指定的命令。  如果
	// 用 /RegServer、/Register、/Unregserver 或 /Unregister 启动应用程序，则返回 FALSE。
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// 唯一的一个窗口已初始化，因此显示它并对其进行更新
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CMyPhotoshopApp::ExitInstance()
{
	//TODO: 处理可能已添加的附加资源
	AfxOleTerm(FALSE);

	return CWinApp::ExitInstance();
}

// CMyPhotoshopApp 消息处理程序


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// 用于运行对话框的应用程序命令
void CMyPhotoshopApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CMyPhotoshopApp 消息处理程序


void CMyPhotoshopApp::OnHistogramMix()
{
	CHistogramDlg dlgHistogram;
	dlgHistogram.m_histogramType = 0;
	// 获取活动文档
	POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();// 获取第一个文档模板位置
	if (pos != NULL)
	{
		CDocTemplate* pTemplate = AfxGetApp()->GetNextDocTemplate(pos);// 获取第一个文档模板
		POSITION docPos = pTemplate->GetFirstDocPosition();// 获取第一个文档位置
		if (docPos != NULL)// 判断是否存在文档
		{
			CDocument* pDoc = pTemplate->GetNextDoc(docPos);// 获取第一个文档
			if (pDoc)
			{
				CMyPhotoshopDoc* pMyDoc = dynamic_cast<CMyPhotoshopDoc*>(pDoc);// 将文档转换为CMyPhotoshopDoc类型
				if (pMyDoc && pMyDoc->pImage != nullptr)
				{
					std::vector<int> m_HistogramMix;					
					m_HistogramMix = pMyDoc->pImage->CalculateHistogramMix();// 计算灰度直方图

					dlgHistogram.SetHistogramDataMix(m_HistogramMix); // 设置直方图数据
					dlgHistogram.DoModal();// 显示对话框
				}
				else
				{
					AfxMessageBox(_T("No image loaded."));
				}
			}
			else
			{
				AfxMessageBox(_T("No document opened."));
			}
		}
	}
}
void CMyPhotoshopApp::OnHistogramRGB()
{
	CHistogramDlg dlgHistogram;
	dlgHistogram.m_histogramType = 1; // 假设1表示RGB模式

	// 获取活动文档
	POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition(); // 获取第一个文档模板位置
	if (pos != NULL)
	{
		CDocTemplate* pTemplate = AfxGetApp()->GetNextDocTemplate(pos); // 获取第一个文档模板
		POSITION docPos = pTemplate->GetFirstDocPosition(); // 获取第一个文档位置
		if (docPos != NULL) // 判断是否存在文档
		{
			CDocument* pDoc = pTemplate->GetNextDoc(docPos); // 获取第一个文档
			if (pDoc)
			{
				CMyPhotoshopDoc* pMyDoc = dynamic_cast<CMyPhotoshopDoc*>(pDoc); // 将文档转换为CMyPhotoshopDoc类型
				if (pMyDoc && pMyDoc->pImage != nullptr)
				{
					std::vector<std::vector<int>> m_HistogramRGB;
					m_HistogramRGB = pMyDoc->pImage->CalculateHistogramRGB(); // 计算RGB直方图
					dlgHistogram.SetHistogramDataRGB(m_HistogramRGB); // 设置RGB直方图数据
					dlgHistogram.DoModal(); // 显示对话框
				}
				else
				{
					AfxMessageBox(_T("No image loaded."));
				}
			}
			else
			{
				AfxMessageBox(_T("No document opened."));
			}
		}
	}
}

// 在文件末尾添加以下函数实现
void CMyPhotoshopApp::OnIntensityTrans()
{
	// 创建并显示灰度线性变换对话框
	CINTENSITYDlg dlgIntensity;

	// 获取活动文档
	POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();
	if (pos != NULL)
	{
		CDocTemplate* pTemplate = AfxGetApp()->GetNextDocTemplate(pos);
		POSITION docPos = pTemplate->GetFirstDocPosition();
		if (docPos != NULL)
		{
			CDocument* pDoc = pTemplate->GetNextDoc(docPos);
			if (pDoc)
			{
				CMyPhotoshopDoc* pMyDoc = dynamic_cast<CMyPhotoshopDoc*>(pDoc);
				if (pMyDoc && pMyDoc->pImage != nullptr)
				{
					// 将图像数据传递给对话框
					dlgIntensity.SetImageData(pMyDoc->pImage);

					// 显示对话框
					dlgIntensity.DoModal();

					// 对话框关闭后可以处理返回结果
					// 例如更新图像并刷新视图
				}
				else
				{
					AfxMessageBox(_T("未加载图像。"));
				}
			}
			else
			{
				AfxMessageBox(_T("未打开文档。"));
			}
		}
	}
}
void CMyPhotoshopApp::OnHistogramEqualization()//直方图均衡化菜单命令的响应函数
{
	// 获取活动文档
	POSITION pos = AfxGetApp()->GetFirstDocTemplatePosition();
	if (pos != NULL)
	{
		CDocTemplate* pTemplate = AfxGetApp()->GetNextDocTemplate(pos);
		POSITION docPos = pTemplate->GetFirstDocPosition();
		if (docPos != NULL)//// 检查是否存在打开的文档
		{
			CDocument* pDoc = pTemplate->GetNextDoc(docPos);
			if (pDoc)
			{
				CMyPhotoshopDoc* pMyDoc = dynamic_cast<CMyPhotoshopDoc*>(pDoc);
				if (pMyDoc && pMyDoc->pImage != nullptr)
				{
					// 获取视图
					POSITION viewPos = pMyDoc->GetFirstViewPosition(); 
					if (viewPos != NULL)
					{
						CView* pView = pMyDoc->GetNextView(viewPos);
						if (pView)
						{
							CClientDC dc(pView);

							std::vector<std::vector<int>> histograms_balance;
							histograms_balance = pMyDoc->pImage->Balance_Transformations(dc); // 执行直方图均衡化

							// 重新显示均衡化后的直方图
							CHistogramDlg dlgHistogram;//// 创建直方图显示对话框
							dlgHistogram.m_histogramType = 1;//设置直方图类型为RGB模式
							dlgHistogram.SetHistogramDataRGB(histograms_balance);//将均衡化数据传递给对话框
							dlgHistogram.DoModal();
						}
					}
				}
				else
				{
					AfxMessageBox(_T("No image loaded."));
				}
			}
			else
			{
				AfxMessageBox(_T("No document opened."));
			}
		}
	}
}