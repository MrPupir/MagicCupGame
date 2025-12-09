// SettingsDlg.cpp: файл реализации
//

#include "pch.h"
#include "MagicCupGame.h"
#include "afxdialogex.h"
#include "SettingsDlg.h"

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_SETTINGS, pParent)
    , m_bTopStroke(FALSE), m_bBtnStroke(FALSE), m_bActStroke(FALSE)
    , m_nTopThick(0), m_nBtnThick(0), m_nActThick(0)
    , m_bHudBorder(FALSE), m_bCrossEnable(FALSE), m_bCrossBorder(FALSE), m_nCrossThick(0)
    , m_bTableEnabled(TRUE) , m_bCarpetEnabled(TRUE)
    , m_strTopPreview(_T("")), m_strBtnPreview(_T("")), m_strActPreview(_T(""))
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_DIFFICULTY, m_comboDifficulty);
    DDX_Control(pDX, IDC_COMBO_SKYBOX, m_comboSkybox);
    DDX_Control(pDX, IDC_COMBO_TABLE, m_comboTable);
    DDX_Control(pDX, IDC_COMBO_CARPET, m_comboCarpet);
    DDX_Check(pDX, IDC_CHK_TABLE_ENABLE, m_bTableEnabled);
    DDX_Check(pDX, IDC_CHK_CARPET_ENABLE, m_bCarpetEnabled);
    DDX_Text(pDX, IDC_LBL_FONT_TOP_PREVIEW, m_strTopPreview);
    DDX_Check(pDX, IDC_CHK_FONT_TOP_STROKE, m_bTopStroke);
    DDX_Text(pDX, IDC_EDT_FONT_TOP_THICK, m_nTopThick);
    DDX_Text(pDX, IDC_LBL_FONT_BTN_PREVIEW, m_strBtnPreview);
    DDX_Check(pDX, IDC_CHK_FONT_BTN_STROKE, m_bBtnStroke);
    DDX_Text(pDX, IDC_EDT_FONT_BTN_THICK, m_nBtnThick);
    DDX_Text(pDX, IDC_LBL_FONT_ACT_PREVIEW, m_strActPreview);
    DDX_Check(pDX, IDC_CHK_FONT_ACT_STROKE, m_bActStroke);
    DDX_Text(pDX, IDC_EDT_FONT_ACT_THICK, m_nActThick);
    DDX_Check(pDX, IDC_CHK_HUD_BORDER, m_bHudBorder);
    DDX_Check(pDX, IDC_CHK_CROSS_ENABLE, m_bCrossEnable);
    DDX_Check(pDX, IDC_CHK_CROSS_BORDER, m_bCrossBorder);
    DDX_Text(pDX, IDC_EDT_CROSS_THICK, m_nCrossThick);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_FONT_TOP_SELECT, &CSettingsDlg::OnBnClickedBtnFontTopSelect)
    ON_BN_CLICKED(IDC_BTN_FONT_BTN_SELECT, &CSettingsDlg::OnBnClickedBtnFontBtnSelect)
    ON_BN_CLICKED(IDC_BTN_FONT_ACT_SELECT, &CSettingsDlg::OnBnClickedBtnFontActSelect)

    ON_BN_CLICKED(IDC_BTN_FONT_TOP_COLOR, &CSettingsDlg::OnBnClickedBtnFontTopColor)
    ON_BN_CLICKED(IDC_BTN_FONT_BTN_COLOR, &CSettingsDlg::OnBnClickedBtnFontBtnColor)
    ON_BN_CLICKED(IDC_BTN_FONT_ACT_COLOR, &CSettingsDlg::OnBnClickedBtnFontActColor)

    ON_BN_CLICKED(IDC_BTN_HUD_BG_COLOR, &CSettingsDlg::OnBnClickedBtnHudBgColor)
    ON_BN_CLICKED(IDC_BTN_HUD_BORDER_COLOR, &CSettingsDlg::OnBnClickedBtnHudBorderColor)
    ON_BN_CLICKED(IDC_BTN_CROSS_COLOR, &CSettingsDlg::OnBnClickedBtnCrossColor)
    ON_BN_CLICKED(IDC_BTN_CROSS_BORDER_COLOR, &CSettingsDlg::OnBnClickedBtnCrossBorderColor)
    ON_BN_CLICKED(IDC_BTN_CUP_COLOR, &CSettingsDlg::OnBnClickedBtnCupColor)
    ON_BN_CLICKED(IDC_BTN_BALL_COLOR, &CSettingsDlg::OnBnClickedBtnBallColor)
END_MESSAGE_MAP()

void CSettingsDlg::UpdateFontPreviewLabel(const FontConfig& cfg, int labelID)
{
    CString str;
    CString style;
    if (cfg.fontWeight >= 700) style += _T(" B");
    if (cfg.isItalic) style += _T(" I");
    if (cfg.isUnderline) style += _T(" U");
    if (cfg.isStrikeOut) style += _T(" S");

    str.Format(_T("%s, %dpt%s"), cfg.fontName, cfg.fontSize, style);

    if (labelID == IDC_LBL_FONT_TOP_PREVIEW) m_strTopPreview = str;
    else if (labelID == IDC_LBL_FONT_BTN_PREVIEW) m_strBtnPreview = str;
    else if (labelID == IDC_LBL_FONT_ACT_PREVIEW) m_strActPreview = str;

    if (GetSafeHwnd()) SetDlgItemText(labelID, str);
}

void CSettingsDlg::ChooseFont(FontConfig& cfg, int labelID)
{
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));

    _tcscpy_s(lf.lfFaceName, cfg.fontName);
    lf.lfWeight = cfg.fontWeight;
    lf.lfItalic = cfg.isItalic;
    lf.lfUnderline = cfg.isUnderline;
    lf.lfStrikeOut = cfg.isStrikeOut;

    HDC hDC = ::GetDC(NULL);
    lf.lfHeight = -MulDiv(cfg.fontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    ::ReleaseDC(NULL, hDC);

    CFontDialog dlg(&lf, CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT, NULL, this);

    if (dlg.DoModal() == IDOK) {
        _tcscpy_s(cfg.fontName, dlg.GetFaceName());
        cfg.fontSize = dlg.GetSize() / 10;
        cfg.fontWeight = dlg.GetWeight();
        cfg.isItalic = dlg.IsItalic();
        cfg.isUnderline = dlg.IsUnderline();
        cfg.isStrikeOut = dlg.IsStrikeOut();

        UpdateFontPreviewLabel(cfg, labelID);
    }
}

BOOL CSettingsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_levels = DBHelper::GetInstance().GetDifficultyLevels();
    if (m_levels.empty()) {
        AfxMessageBox(_T("Не вдалося завантажити рівні!"));
        return TRUE;
    }

    int selectedIndex = 0;
    for (size_t i = 0; i < m_levels.size(); i++) {
        int index = m_comboDifficulty.AddString(m_levels[i].name);
        m_comboDifficulty.SetItemData(index, (DWORD_PTR)i);
        if (m_levels[i].id == m_nInitialId) selectedIndex = index;
    }
    m_comboDifficulty.SetCurSel(selectedIndex);

    if (!m_skyboxNames.empty()) {
        for (const auto& name : m_skyboxNames) m_comboSkybox.AddString(name);
        if (m_nInitialSkybox >= 0 && m_nInitialSkybox < m_skyboxNames.size())
            m_comboSkybox.SetCurSel(m_nInitialSkybox);
        else
            m_comboSkybox.SetCurSel(0);
    }

    if (!m_tableNames.empty()) {
        for (const auto& name : m_tableNames) m_comboTable.AddString(name);
        int sel = m_gameSettings.tableMatIndex;
        if (m_nInitialTable >= 0) sel = m_nInitialTable;

        if (sel >= 0 && sel < m_tableNames.size()) m_comboTable.SetCurSel(sel);
        else m_comboTable.SetCurSel(0);
    }

    if (!m_carpetNames.empty()) {
        for (const auto& name : m_carpetNames) m_comboCarpet.AddString(name);

        int sel = m_gameSettings.carpetMatIndex;
        if (m_nInitialCarpet >= 0) sel = m_nInitialCarpet;

        if (sel >= 0 && sel < m_carpetNames.size()) m_comboCarpet.SetCurSel(sel);
        else m_comboCarpet.SetCurSel(0);
    }

    CMagicCupGameApp* pApp = (CMagicCupGameApp*)AfxGetApp();
    int userId = pApp->m_nCurrentUserID;

    if (userId != -1) {
        if (!DBHelper::GetInstance().GetGameSettings(userId, m_gameSettings)) {
            DBHelper::GetInstance().GetGameSettings(-1, m_gameSettings);
        }
    }
    else {
        DBHelper::GetInstance().GetGameSettings(-1, m_gameSettings);
    }

    m_bTableEnabled = m_gameSettings.tableEnabled;
    m_bCarpetEnabled = m_gameSettings.carpetEnabled;

    if (m_nInitialTable < 0) m_nInitialTable = m_gameSettings.tableMatIndex;
    if (m_nInitialCarpet < 0) m_nInitialCarpet = m_gameSettings.carpetMatIndex;

    if (m_comboTable.GetCount() > 0) {
        if (m_nInitialTable >= 0 && m_nInitialTable < m_comboTable.GetCount())
            m_comboTable.SetCurSel(m_nInitialTable);
        else m_comboTable.SetCurSel(0);
    }

    if (m_comboCarpet.GetCount() > 0) {
        if (m_nInitialCarpet >= 0 && m_nInitialCarpet < m_comboCarpet.GetCount())
            m_comboCarpet.SetCurSel(m_nInitialCarpet);
        else m_comboCarpet.SetCurSel(0);
    }

    m_bTopStroke = m_gameSettings.topMenuFont.useStroke;
    m_nTopThick = m_gameSettings.topMenuFont.strokeThickness;

    m_bBtnStroke = m_gameSettings.buttonFont.useStroke;
    m_nBtnThick = m_gameSettings.buttonFont.strokeThickness;

    m_bActStroke = m_gameSettings.actionFont.useStroke;
    m_nActThick = m_gameSettings.actionFont.strokeThickness;

    m_bHudBorder = m_gameSettings.hudBorderEnabled;
    m_bCrossEnable = m_gameSettings.crosshairEnabled;
    m_bCrossBorder = m_gameSettings.crosshairBorderEnabled;
    m_nCrossThick = m_gameSettings.crosshairBorderThickness;

    UpdateFontPreviewLabel(m_gameSettings.topMenuFont, IDC_LBL_FONT_TOP_PREVIEW);
    UpdateFontPreviewLabel(m_gameSettings.buttonFont, IDC_LBL_FONT_BTN_PREVIEW);
    UpdateFontPreviewLabel(m_gameSettings.actionFont, IDC_LBL_FONT_ACT_PREVIEW);

    UpdateData(FALSE);
    return TRUE;
}

void CSettingsDlg::OnOK()
{
    UpdateData(TRUE);

    int curSel = m_comboDifficulty.GetCurSel();
    int curSelSky = m_comboSkybox.GetCurSel();
    int curSelTable = m_comboTable.GetCurSel();
    int curSelCarpet = m_comboCarpet.GetCurSel();

    if (curSel != CB_ERR) {
        int vectorIndex = (int)m_comboDifficulty.GetItemData(curSel);
        m_selectedLevel = m_levels[vectorIndex];
        m_selectedSkyboxIndex = curSelSky;
        m_selectedTableIndex = (curSelTable == CB_ERR) ? 0 : curSelTable;
        m_selectedCarpetIndex = (curSelCarpet == CB_ERR) ? 0 : curSelCarpet;

        m_gameSettings.skyboxIndex = curSelSky;
        m_gameSettings.tableMatIndex = m_selectedTableIndex;
        m_gameSettings.carpetMatIndex = m_selectedCarpetIndex;

        m_gameSettings.tableEnabled = m_bTableEnabled;
        m_gameSettings.carpetEnabled = m_bCarpetEnabled;

        m_gameSettings.topMenuFont.useStroke = m_bTopStroke;
        m_gameSettings.topMenuFont.strokeThickness = m_nTopThick;

        m_gameSettings.buttonFont.useStroke = m_bBtnStroke;
        m_gameSettings.buttonFont.strokeThickness = m_nBtnThick;

        m_gameSettings.actionFont.useStroke = m_bActStroke;
        m_gameSettings.actionFont.strokeThickness = m_nActThick;

        m_gameSettings.hudBorderEnabled = m_bHudBorder;
        m_gameSettings.crosshairEnabled = m_bCrossEnable;
        m_gameSettings.crosshairBorderEnabled = m_bCrossBorder;
        m_gameSettings.crosshairBorderThickness = m_nCrossThick;

        CMagicCupGameApp* pApp = (CMagicCupGameApp*)AfxGetApp();
        int currentUserId = pApp->m_nCurrentUserID;
        if (currentUserId != -1) {
            DBHelper::GetInstance().SaveGameSettings(currentUserId, m_gameSettings);
        }
        CDialogEx::OnOK();
    }
    else {
        AfxMessageBox(_T("Будь ласка, заповніть всі поля!"));
    }
}

void CSettingsDlg::OnBnClickedBtnFontTopSelect() { ChooseFont(m_gameSettings.topMenuFont, IDC_LBL_FONT_TOP_PREVIEW); }
void CSettingsDlg::OnBnClickedBtnFontBtnSelect() { ChooseFont(m_gameSettings.buttonFont, IDC_LBL_FONT_BTN_PREVIEW); }
void CSettingsDlg::OnBnClickedBtnFontActSelect() { ChooseFont(m_gameSettings.actionFont, IDC_LBL_FONT_ACT_PREVIEW); }

void CSettingsDlg::ChooseColor(COLORREF& color, const CString& title)
{
    CColorDialog dlg(color, CC_FULLOPEN, this);
    if (dlg.DoModal() == IDOK) {
        color = dlg.GetColor();
    }
}

void CSettingsDlg::OnBnClickedBtnFontTopColor() { ChooseColor(m_gameSettings.topMenuFont.strokeColor, _T("Колір обводки меню")); }
void CSettingsDlg::OnBnClickedBtnFontBtnColor() { ChooseColor(m_gameSettings.buttonFont.strokeColor, _T("Колір обводки кнопок")); }
void CSettingsDlg::OnBnClickedBtnFontActColor() { ChooseColor(m_gameSettings.actionFont.strokeColor, _T("Колір обводки дій")); }
void CSettingsDlg::OnBnClickedBtnHudBgColor() { ChooseColor(m_gameSettings.hudRectColor, _T("Колір фону HUD")); }
void CSettingsDlg::OnBnClickedBtnHudBorderColor() { ChooseColor(m_gameSettings.hudBorderColor, _T("Колір рамки HUD")); }
void CSettingsDlg::OnBnClickedBtnCrossColor() { ChooseColor(m_gameSettings.crosshairColor, _T("Колір прицілу")); }
void CSettingsDlg::OnBnClickedBtnCrossBorderColor() { ChooseColor(m_gameSettings.crosshairBorderColor, _T("Колір обводки прицілу")); }
void CSettingsDlg::OnBnClickedBtnCupColor() { ChooseColor(m_gameSettings.cupColor, _T("Колір стаканчика")); }
void CSettingsDlg::OnBnClickedBtnBallColor() { ChooseColor(m_gameSettings.ballColor, _T("Колір кульки")); }