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
    GameSettings m_gameSettings;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    DECLARE_MESSAGE_MAP()
public:
    CComboBox m_comboDifficulty;
    CComboBox m_comboSkybox;
    CComboBox m_comboTable;
    CComboBox m_comboCarpet;
    BOOL m_bTableEnabled;
    BOOL m_bCarpetEnabled;
    std::vector<DifficultyLevel> m_levels;
    std::vector<CString> m_skyboxNames;
    std::vector<CString> m_tableNames;
    std::vector<CString> m_carpetNames;

    int m_nInitialId = -1;
    int m_nInitialSkybox = 0;
    int m_nInitialTable = 0;
    int m_nInitialCarpet = 0;
    int m_selectedSkyboxIndex = 0;
    int m_selectedTableIndex = 0;
    int m_selectedCarpetIndex = 0;

    CString m_strTopPreview;
    CString m_strBtnPreview;
    CString m_strActPreview;

    BOOL m_bTopStroke;
    int m_nTopThick;
    BOOL m_bBtnStroke;
    int m_nBtnThick;
    BOOL m_bActStroke;
    int m_nActThick;

    afx_msg void OnBnClickedBtnFontTopSelect();
    afx_msg void OnBnClickedBtnFontBtnSelect();
    afx_msg void OnBnClickedBtnFontActSelect();
    afx_msg void OnBnClickedBtnFontTopColor();
    afx_msg void OnBnClickedBtnFontBtnColor();
    afx_msg void OnBnClickedBtnFontActColor();
    afx_msg void OnBnClickedBtnHudBgColor();
    afx_msg void OnBnClickedBtnHudBorderColor();
    afx_msg void OnBnClickedBtnCrossColor();
    afx_msg void OnBnClickedBtnCrossBorderColor();
    afx_msg void OnBnClickedBtnCupColor();
    afx_msg void OnBnClickedBtnBallColor();

    BOOL m_bHudBorder;
    BOOL m_bCrossEnable;
    BOOL m_bCrossBorder;
    int m_nCrossThick;

    BOOL m_bDisabledDifficulty;

private:
    void UpdateFontPreviewLabel(const FontConfig& cfg, int labelID);
    void ChooseFont(FontConfig& cfg, int labelID);
    void ChooseColor(COLORREF& color, const CString& title);
};