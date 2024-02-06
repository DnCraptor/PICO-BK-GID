
// HDDImgMaker.h : главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
#error "включить stdafx.h до включения этого файла в PCH"
#endif

#include "resource.h"       // основные символы


// CHDDImgMakerApp:
// О реализации данного класса см. HDDImgMaker.cpp
//

class CHDDImgMakerApp : public CWinApp
{
	public:
		CHDDImgMakerApp();

// Переопределение
	public:
		virtual BOOL InitInstance() override;

// Реализация

		DECLARE_MESSAGE_MAP()
};

extern CHDDImgMakerApp theApp;
