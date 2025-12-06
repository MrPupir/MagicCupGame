#pragma once
#include "afxdialogex.h"
#include "DBHelper.h"

// Диалоговое окно CSettingsDlg

class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = nullptr);   // стандартный конструктор
	virtual ~CSettingsDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS };
#endif

	DifficultyLevel m_selectedLevel;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_comboDifficulty;
	std::vector<DifficultyLevel> m_levels;
	int m_nInitialId = -1;
};
