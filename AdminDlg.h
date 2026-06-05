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
	CComboBox m_comboChartMetric;
	CComboBox m_comboChartFilter;
	CStatic m_lblMetric;
	CStatic m_lblFilter;
	CButton m_check3D;

	CComboBox m_comboAdmPlayer;
	CListCtrl m_listAdmGames;

	int m_nSelectedDiffID = -1;
	int m_nSelectedPlayerID = -1;

	void UpdateVisibility();
	void ShowControls(const std::vector<int>& ids, BOOL bShow);
	void RefreshDifficultyList();
	void SetupPlayerStatsList();
	void RefreshPlayerCombo();
	void LoadPlayerStats(int userId);

	void SetupChartOptions();
	void UpdateMetricCombo();
	void UpdateFilterCombo();
	void RefreshChart();
	CString GetCurrentMetricDBName();

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
	afx_msg void OnCbnSelchangeComboChartType();
	afx_msg void OnCbnSelchangeComboChartMetric();
	afx_msg void OnCbnSelchangeComboChartFilter();
	afx_msg void OnCbnSelchangeComboAdmPlayer();

	virtual void OnOK();
};
