
// MagicCupGame.h: основной файл заголовка для приложения MagicCupGame
//
#pragma once

#ifndef __AFXWIN_H__
	#error "включить pch.h до включения этого файла в PCH"
#endif

#include "resource.h"       // основные символы


// CMagicCupGameApp:
// Сведения о реализации этого класса: MagicCupGame.cpp
//

class CMagicCupGameApp : public CWinApp
{
public:
	CMagicCupGameApp() noexcept;

	int m_nCurrentUserID = -1;
// Переопределение
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Реализация
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CMagicCupGameApp theApp;
