
// BKDoc.cpp : реализация класса CBKDoc
//
#ifdef UI
#include "pch.h"
// SHARED_HANDLERS можно определить в обработчиках фильтров просмотра реализации проекта ATL, эскизов
// и поиска; позволяет совместно использовать код документа в данным проекте.
#ifndef SHARED_HANDLERS
#include "BK.h"
#endif

#include "BKDoc.h"

#include <propkey.h>
#include "Config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CBKDoc

IMPLEMENT_DYNCREATE(CBKDoc, CDocument)

BEGIN_MESSAGE_MAP(CBKDoc, CDocument)
END_MESSAGE_MAP()


// создание/уничтожение CBKDoc

CBKDoc::CBKDoc()
    = default;

CBKDoc::~CBKDoc()
    = default;

BOOL CBKDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
	{
		return FALSE;
	}

	// TODO: добавьте код повторной инициализации
	// (Документы SDI будут повторно использовать этот документ)
	return TRUE;
}


// сериализация CBKDoc

void CBKDoc::Serialize(CArchive &ar)
{
	if (ar.IsStoring())
	{
		// TODO: добавьте код сохранения
	}
	else
	{
		// TODO: добавьте код загрузки
	}
}

#ifdef SHARED_HANDLERS

// Поддержка для эскизов
void CBKDoc::OnDrawThumbnail(CDC &dc, LPRECT lprcBounds)
{
	// Измените этот код для отображения данных документа
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));
	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;
	CFont *pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;
	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);
	CFont *pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Поддержка обработчиков поиска
void CBKDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Задайте содержимое поиска из данных документа.
	// Части содержимого должны разделяться точкой с запятой ";"
	// Например:  strSearchContent = _T("точка;прямоугольник;круг;объект ole;");
	SetSearchContent(strSearchContent);
}

void CBKDoc::SetSearchContent(const CString &value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);

		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// диагностика CBKDoc

#ifdef _DEBUG
void CBKDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CBKDoc::Dump(CDumpContext &dc) const
{
	CDocument::Dump(dc);
}
#endif // _DEBUG


// команды CBKDoc


void CBKDoc::SetTitle(LPCTSTR lpszTitle)
{
	CONF_BKMODEL n = g_Config.GetBKModel();
	CString str = CString(MAKEINTRESOURCE(g_mstrConfigBKModelParameters[static_cast<int>(n)].nIDBKModelName));
	CDocument::SetTitle(str.GetString());
}
#endif