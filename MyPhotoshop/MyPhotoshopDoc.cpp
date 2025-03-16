
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
	ON_COMMAND(ID_FILE_OPEN, &CMyPhotoshopDoc::OnFileOpen)
END_MESSAGE_MAP()


// CMyPhotoshopDoc 构造/析构

CMyPhotoshopDoc::CMyPhotoshopDoc() noexcept
{
	// TODO: 在此添加一次性构造代码
	pImage = new CImageProc;

}

CMyPhotoshopDoc::~CMyPhotoshopDoc()
{
	delete pImage;
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
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CView* pView = pMainFrame->GetActiveView();
	pView->Invalidate(TRUE);
}
