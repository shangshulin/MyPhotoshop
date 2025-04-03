
// MyPhotoshopDoc.h: CMyPhotoshopDoc 类的接口
//


#pragma once
#include "CImageProc.h"

class CMyPhotoshopDoc : public CDocument
{
protected: // 仅从序列化创建
	CMyPhotoshopDoc() noexcept;
	DECLARE_DYNCREATE(CMyPhotoshopDoc)

// 特性
public:
	CImageProc* pImage;
	std::vector<int> m_HistogramMix;
	std::vector<std::vector<int>> m_HistogramRGB;
// 操作
public:

// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 实现
public:
	virtual ~CMyPhotoshopDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	afx_msg void OnFileOpen();
	void CalculateHistogramMix();
	void CalculateHistogramRGB();
	const std::vector<int>& GetHistogramMix() const { return m_HistogramMix; }
	const std::vector<std::vector<int>>& GetHistogramRGB() const { return m_HistogramRGB; }
};
