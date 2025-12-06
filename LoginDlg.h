#pragma once
#include "afxdialogex.h"

// Диалоговое окно CLoginDlg

class CLoginDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CLoginDlg)

public:
	CLoginDlg(CWnd* pParent = nullptr);   // стандартный конструктор
	virtual ~CLoginDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LOGIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

	DECLARE_MESSAGE_MAP()
public:
	int m_nUserID;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedBtnGotoReg();
};
