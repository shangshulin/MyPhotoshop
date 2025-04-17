
// MyPhotoshopDoc.cpp: CMyPhotoshopDoc 类的实现
//

#include "pch.h"
#include "framework.h"
#include "MainFrm.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "MyPhotoshop.h"
#endif

#include "MyPhotoshopDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMyPhotoshopDoc

IMPLEMENT_DYNCREATE(CMyPhotoshopDoc, CDocument)

BEGIN_MESSAGE_MAP(CMyPhotoshopDoc, CDocument)
	ON_COMMAND(ID_FILE_OPEN, &CMyPhotoshopDoc::OnFileOpen)// 打开文件

END_MESSAGE_MAP()

// CMyPhotoshopDoc 构造/析构

CMyPhotoshopDoc::CMyPhotoshopDoc() noexcept
{
	// TODO: 在此添加一次性构造代码
	pImage = new CImageProc;
	pTemp1 = new CImageProc; // 初始化临时图像处理对象
	pTemp2 = new CImageProc; // 初始化第二个临时图像处理对象
}

CMyPhotoshopDoc::~CMyPhotoshopDoc()
{
	delete pImage;
	delete pTemp1; // 释放临时图像处理对象
	delete pTemp2;
}

BOOL CMyPhotoshopDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)

	return TRUE;
}




// CMyPhotoshopDoc 序列化

void CMyPhotoshopDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 在此添加存储代码
	}
	else
	{
		// TODO: 在此添加加载代码
	}
}

#ifdef SHARED_HANDLERS

// 缩略图的支持
void CMyPhotoshopDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// 修改此代码以绘制文档数据
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 搜索处理程序的支持
void CMyPhotoshopDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// 从文档数据设置搜索内容。
	// 内容部分应由“;”分隔

	// 例如:     strSearchContent = _T("point;rectangle;circle;ole object;")；
	SetSearchContent(strSearchContent);
}

void CMyPhotoshopDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CMyPhotoshopDoc 诊断

#ifdef _DEBUG
void CMyPhotoshopDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMyPhotoshopDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMyPhotoshopDoc 命令


void CMyPhotoshopDoc::OnFileOpen()
{
	// TODO: 在此添加命令处理程序代码
	pImage->OpenFile();
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;// 获取主窗口
	CView* pView = pMainFrame->GetActiveView();// 获取当前视图
	pView->Invalidate(TRUE);
}

// 图像增强
void CMyPhotoshopDoc::ApplyImageEnhancement()
{
	if (!pImage->IsValid()) {
		AfxMessageBox(_T("图像数据无效"));
		return;
	}

	// 1. 保存原图像
	*pTemp1 = *pImage;
	
	// 2. 拉普拉斯算子处理
	pImage->ApplyLaplaceEdgeDetection();

	// 3. 原图和拉普拉斯相加形成锐化图像
	pImage->Add(*pTemp1, 1.0, 1.0); // 锐化图像 = 原图 + 拉普拉斯

	*pTemp2 = *pImage;// 保存锐化图像

	// 4. Sobel算子处理
	pImage->ApplySobelEdgeDetection();

	// 5. 对Sobel结果均值滤波
	pImage->ApplyMeanFilter();

	// 6. 锐化图像和滤波后梯度相乘形成掩蔽
	pImage->Multiply(*pTemp2); // 掩蔽 = 锐化图像 × 滤波梯度

	// 7. 原图和掩蔽求和
	pImage->Add(*pTemp1, 1.0, 1.0); // 原图 + 掩蔽

	// 8. 幂律变换（γ=0.5）
	pImage->PowerTransform(0.5);


}
