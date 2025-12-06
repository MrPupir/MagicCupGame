#pragma once
#include "afxdialogex.h"
#include <vector>

// Диалоговое окно CAdminDlg

class CAdminDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAdminDlg)

public:
	CAdminDlg(CWnd* pParent = nullptr);   // стандартный конструктор
	virtual ~CAdminDlg();

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADMIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_comboMode;
	CListBox m_lstDifficulty;

	CEdit m_edtDiffName;
	CEdit m_edtDiffCount;
	CEdit m_edtDiffSpeed;

	int m_nSelectedDiffID = -1;

	void UpdateVisibility();
	void ShowControls(const std::vector<int>& ids, BOOL bShow);
	void RefreshDifficultyList();

	afx_msg void OnCbnSelchangeComboAdmMode();
	afx_msg void OnLbnSelchangeLstDiffLevels();
	afx_msg void OnBnClickedBtnDiffAdd();
	afx_msg void OnBnClickedBtnDiffSave();
	afx_msg void OnBnClickedBtnDiffDel();
	afx_msg void OnBnClickedBtnDiffUp();
	afx_msg void OnBnClickedBtnDiffDown();
	virtual void OnOK();
};
