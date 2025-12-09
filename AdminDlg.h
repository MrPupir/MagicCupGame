#pragma once
#include "afxdialogex.h"
#include <vector>
#include "ChartControl.h"

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

	ChartControl m_chartControl;
	CComboBox m_comboChartType;
	CButton m_check3D;

	int m_nSelectedDiffID = -1;

	void UpdateVisibility();
	void ShowControls(const std::vector<int>& ids, BOOL bShow);
	void RefreshDifficultyList();
	void RefreshChart();

	afx_msg void OnCbnSelchangeComboAdmMode();
	afx_msg void OnLbnSelchangeLstDiffLevels();
	afx_msg void OnBnClickedBtnDiffAdd();
	afx_msg void OnBnClickedBtnDiffSave();
	afx_msg void OnBnClickedBtnDiffDel();
	afx_msg void OnBnClickedBtnDiffUp();
	afx_msg void OnBnClickedBtnDiffDown();

	afx_msg void OnBnClickedBtnSortAsc();
	afx_msg void OnBnClickedBtnSortDesc();
	afx_msg void OnBnClickedCheck3d();
	afx_msg void OnBnClickedBtnExpExcel();
	afx_msg void OnBnClickedBtnExpWord();
	afx_msg void OnBnClickedBtnColors();
	afx_msg void OnCbnSelchangeComboChartType();

	virtual void OnOK();
};
