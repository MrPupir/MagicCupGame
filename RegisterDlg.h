#pragma once
#include "afxdialogex.h"


// Диалоговое окно CRegisterDlg

class CRegisterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRegisterDlg)

public:
	CRegisterDlg(CWnd* pParent = nullptr);   // стандартный конструктор
	virtual ~CRegisterDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REGISTER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();

	int m_nNewUserID = -1;
};
