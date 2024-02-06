
// BKDE.h : главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
#error "включить stdafx.h до включения этого файла в PCH"
#endif

#include "resource.h"       // основные символы


// CBKDEApp:
// О реализации данного класса см. BKDE.cpp
//

class CBKDEApp : public CWinApp
{
//      UINT                m_nInterAppToolGlobalMessage;

	public:
		CBKDEApp();

// Переопределение
	public:
		virtual BOOL InitInstance() override;
//      virtual int ExitInstance() override;
// Реализация

		DECLARE_MESSAGE_MAP()
};

extern CBKDEApp theApp;
