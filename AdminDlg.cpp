// AdminDlg.cpp: файл реализации
//

#include "pch.h"
#include "MagicCupGame.h"
#include "afxdialogex.h"
#include "AdminDlg.h"
#include "DBHelper.h"

// Диалоговое окно CAdminDlg

IMPLEMENT_DYNAMIC(CAdminDlg, CDialogEx)

CAdminDlg::CAdminDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ADMIN, pParent)
{

}

CAdminDlg::~CAdminDlg()
{
}

void CAdminDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_ADM_MODE, m_comboMode);
    DDX_Control(pDX, IDC_LST_DIFF_LEVELS, m_lstDifficulty);
    DDX_Control(pDX, IDC_EDT_DIFF_NAME, m_edtDiffName);
    DDX_Control(pDX, IDC_EDT_DIFF_COUNT, m_edtDiffCount);
    DDX_Control(pDX, IDC_EDT_DIFF_SPEED, m_edtDiffSpeed);
    DDX_Control(pDX, IDC_CHARTCONTROLCTRL1, m_chartControl);
    DDX_Control(pDX, IDC_COMBO_CHART_TYPE, m_comboChartType);
    DDX_Control(pDX, IDC_COMBO_CHART_METRIC, m_comboChartMetric);
    DDX_Control(pDX, IDC_COMBO_CHART_FILTER, m_comboChartFilter);
    DDX_Control(pDX, IDC_STATIC_CHART_METRIC, m_lblMetric);
    DDX_Control(pDX, IDC_STATIC_CHART_FILTER, m_lblFilter);
    DDX_Control(pDX, IDC_CHECK_3D, m_check3D);
    DDX_Control(pDX, IDC_COMBO_ADM_PLAYER, m_comboAdmPlayer);
    DDX_Control(pDX, IDC_LIST_ADM_GAMES, m_listAdmGames);
}


BEGIN_MESSAGE_MAP(CAdminDlg, CDialogEx)
    ON_CBN_SELCHANGE(IDC_COMBO_ADM_MODE, &CAdminDlg::OnCbnSelchangeComboAdmMode)
    ON_LBN_SELCHANGE(IDC_LST_DIFF_LEVELS, &CAdminDlg::OnLbnSelchangeLstDiffLevels)
    ON_BN_CLICKED(IDC_BTN_DIFF_ADD, &CAdminDlg::OnBnClickedBtnDiffAdd)
    ON_BN_CLICKED(IDC_BTN_DIFF_SAVE, &CAdminDlg::OnBnClickedBtnDiffSave)
    ON_BN_CLICKED(IDC_BTN_DIFF_DEL, &CAdminDlg::OnBnClickedBtnDiffDel)
	ON_BN_CLICKED(IDC_BTN_DIFF_UP, &CAdminDlg::OnBnClickedBtnDiffUp)
	ON_BN_CLICKED(IDC_BTN_DIFF_DOWN, &CAdminDlg::OnBnClickedBtnDiffDown)
    ON_BN_CLICKED(IDC_BTN_SORT_ASC, &CAdminDlg::OnBnClickedBtnSortAsc)
    ON_BN_CLICKED(IDC_BTN_SORT_DESC, &CAdminDlg::OnBnClickedBtnSortDesc)
    ON_BN_CLICKED(IDC_CHECK_3D, &CAdminDlg::OnBnClickedCheck3d)
    ON_BN_CLICKED(IDC_BTN_EXP_EXCEL, &CAdminDlg::OnBnClickedBtnExpExcel)
    ON_BN_CLICKED(IDC_BTN_EXP_WORD, &CAdminDlg::OnBnClickedBtnExpWord)
    ON_CBN_SELCHANGE(IDC_COMBO_CHART_TYPE, &CAdminDlg::OnCbnSelchangeComboChartType)
    ON_CBN_SELCHANGE(IDC_COMBO_CHART_METRIC, &CAdminDlg::OnCbnSelchangeComboChartMetric)
    ON_CBN_SELCHANGE(IDC_COMBO_CHART_FILTER, &CAdminDlg::OnCbnSelchangeComboChartFilter)
    ON_CBN_SELCHANGE(IDC_COMBO_ADM_PLAYER, &CAdminDlg::OnCbnSelchangeComboAdmPlayer)
END_MESSAGE_MAP()


// Обработчики сообщений CAdminDlg

BOOL CAdminDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_comboMode.AddString(_T("Редактор складності"));
	m_comboMode.AddString(_T("Статистика гравців")); 
	m_comboMode.AddString(_T("Діаграми"));
	m_comboMode.SetCurSel(0);

    SetupChartOptions();

    m_check3D.SetCheck(BST_CHECKED);

    SetupPlayerStatsList();
    RefreshPlayerCombo();

	UpdateVisibility();

    RefreshDifficultyList();
    RefreshChart();

	return TRUE;
}

void CAdminDlg::SetupChartOptions()
{
    m_comboChartType.ResetContent();
    m_comboChartType.AddString(_T("Щомісячна динаміка"));
    m_comboChartType.AddString(_T("Щотижнева динаміка"));
    m_comboChartType.AddString(_T("Ефективність складності"));
    m_comboChartType.AddString(_T("Активність по годинах"));
    m_comboChartType.AddString(_T("Рейтинг (зважений)"));
    m_comboChartType.AddString(_T("Зсув позиції (Bias)"));
    m_comboChartType.AddString(_T("Швидкість vs WinRate"));
    m_comboChartType.AddString(_T("Топ за складністю"));
    m_comboChartType.AddString(_T("Топ гравців (заг.)"));
    m_comboChartType.AddString(_T("Утримання (Retention)"));
    m_comboChartType.AddString(_T("Популярність Skybox"));
    m_comboChartType.AddString(_T("Популярність Столів"));
    m_comboChartType.AddString(_T("Популярність Килимків"));

    m_comboChartType.SetCurSel(0);
    UpdateMetricCombo();
}

void CAdminDlg::OnCbnSelchangeComboAdmMode()
{
	UpdateVisibility();
}

void CAdminDlg::UpdateVisibility()
{
    int index = m_comboMode.GetCurSel();

    std::vector<int> groupDiff = {
        IDC_GRP_DIFF, IDC_LST_DIFF_LEVELS,
        IDC_STATIC_DIFF1, IDC_EDT_DIFF_NAME,
        IDC_STATIC_DIFF2, IDC_EDT_DIFF_COUNT,
        IDC_STATIC_DIFF3, IDC_EDT_DIFF_SPEED,
        IDC_BTN_DIFF_SAVE, IDC_BTN_DIFF_ADD, IDC_BTN_DIFF_DEL,
        IDC_BTN_DIFF_UP, IDC_BTN_DIFF_DOWN
    };

    std::vector<int> groupStats = {
        IDC_GRP_STATS, IDC_STATIC_STAT1, IDC_COMBO_ADM_PLAYER, IDC_LIST_ADM_GAMES
    };

    std::vector<int> groupChart = {
        IDC_GRP_CHART, IDC_CHARTCONTROLCTRL1,
        IDC_STATIC_CHART_TYPE, IDC_COMBO_CHART_TYPE,
        IDC_STATIC_CHART_METRIC, IDC_COMBO_CHART_METRIC,
        IDC_STATIC_CHART_FILTER, IDC_COMBO_CHART_FILTER,
        IDC_BTN_SORT_ASC, IDC_BTN_SORT_DESC, IDC_CHECK_3D,
        IDC_BTN_EXP_EXCEL, IDC_BTN_EXP_WORD
    };

    ShowControls(groupDiff, (index == 0));
    ShowControls(groupStats, (index == 1));
    ShowControls(groupChart, (index == 2));

    if (index == 2) {
        UpdateMetricCombo();
    }
}

void CAdminDlg::ShowControls(const std::vector<int>& ids, BOOL bShow)
{
	int cmd = bShow ? SW_SHOW : SW_HIDE;
	for (int id : ids) {
		CWnd* pItem = GetDlgItem(id);
		if (pItem) {
			pItem->ShowWindow(cmd);
		}
	}
}

void CAdminDlg::RefreshDifficultyList()
{
    m_lstDifficulty.ResetContent();

    std::vector<DifficultyLevel> levels = DBHelper::GetInstance().GetDifficultyLevels();

    for (const auto& lvl : levels) {
        CString strItem;
        strItem.Format(_T("%s (Х: %d, Ш: %d)"), lvl.name, lvl.shuffleCount, lvl.animationSpeed);

        int idx = m_lstDifficulty.AddString(strItem);
        m_lstDifficulty.SetItemData(idx, (DWORD_PTR)lvl.id);
    }
}

void CAdminDlg::OnLbnSelchangeLstDiffLevels()
{
    int idx = m_lstDifficulty.GetCurSel();
    if (idx == LB_ERR) return;

    m_nSelectedDiffID = (int)m_lstDifficulty.GetItemData(idx);

    std::vector<DifficultyLevel> levels = DBHelper::GetInstance().GetDifficultyLevels();

    for (const auto& lvl : levels) {
        if (lvl.id == m_nSelectedDiffID) {
            m_edtDiffName.SetWindowText(lvl.name);

            CString val;
            val.Format(_T("%d"), lvl.shuffleCount);
            m_edtDiffCount.SetWindowText(val);

            val.Format(_T("%d"), lvl.animationSpeed);
            m_edtDiffSpeed.SetWindowText(val);
            break;
        }
    }
}

void CAdminDlg::OnBnClickedBtnDiffAdd()
{
    CString name, sCount, sSpeed;
    m_edtDiffName.GetWindowText(name);
    m_edtDiffCount.GetWindowText(sCount);
    m_edtDiffSpeed.GetWindowText(sSpeed);

    if (name.IsEmpty() || sCount.IsEmpty() || sSpeed.IsEmpty()) {
        AfxMessageBox(_T("Заповніть всі поля!"));
        return;
    }

    if (DBHelper::GetInstance().AddDifficultyLevel(name, _ttoi(sCount), _ttoi(sSpeed))) {
        AfxMessageBox(_T("Рівень додано!"));
        RefreshDifficultyList();

        m_edtDiffName.SetWindowText(_T(""));
        m_edtDiffCount.SetWindowText(_T(""));
        m_edtDiffSpeed.SetWindowText(_T(""));
    }
}

void CAdminDlg::OnBnClickedBtnDiffSave()
{
    if (m_nSelectedDiffID == -1) {
        AfxMessageBox(_T("Оберіть рівень зі списку для редагування!"));
        return;
    }

    CString name, sCount, sSpeed;
    m_edtDiffName.GetWindowText(name);
    m_edtDiffCount.GetWindowText(sCount);
    m_edtDiffSpeed.GetWindowText(sSpeed);

    if (DBHelper::GetInstance().UpdateDifficultyLevel(m_nSelectedDiffID, name, _ttoi(sCount), _ttoi(sSpeed))) {
        AfxMessageBox(_T("Зміни збережено!"));
        RefreshDifficultyList();
    }
}

void CAdminDlg::OnBnClickedBtnDiffDel()
{
    int idx = m_lstDifficulty.GetCurSel();
    if (idx == LB_ERR) {
        AfxMessageBox(_T("Оберіть рівень для видалення!"));
        return;
    }

    int id = (int)m_lstDifficulty.GetItemData(idx);

    if (MessageBox(_T("Ви впевнені? Це може видалити історію ігор, пов'язану з цим рівнем!"), _T("Увага"), MB_YESNO | MB_ICONWARNING) == IDYES) {
        if (DBHelper::GetInstance().DeleteDifficultyLevel(id)) {
            AfxMessageBox(_T("Рівень видалено."));
            RefreshDifficultyList();

            m_edtDiffName.SetWindowText(_T(""));
            m_edtDiffCount.SetWindowText(_T(""));
            m_edtDiffSpeed.SetWindowText(_T(""));
            m_nSelectedDiffID = -1;
        }
    }
}

void CAdminDlg::OnBnClickedBtnDiffUp()
{
    int idx = m_lstDifficulty.GetCurSel();

    if (idx == LB_ERR || idx == 0) return;

    int currentID = (int)m_lstDifficulty.GetItemData(idx);
    int prevID = (int)m_lstDifficulty.GetItemData(idx - 1);

    if (DBHelper::GetInstance().SwapDifficultyOrder(currentID, prevID)) {
        RefreshDifficultyList();
        m_lstDifficulty.SetCurSel(idx - 1);
        OnLbnSelchangeLstDiffLevels();
    }
}

void CAdminDlg::OnBnClickedBtnDiffDown()
{
    int idx = m_lstDifficulty.GetCurSel();
    int count = m_lstDifficulty.GetCount();

    if (idx == LB_ERR || idx == count - 1) return;

    int currentID = (int)m_lstDifficulty.GetItemData(idx);
    int nextID = (int)m_lstDifficulty.GetItemData(idx + 1);

    if (DBHelper::GetInstance().SwapDifficultyOrder(currentID, nextID)) {
        RefreshDifficultyList();
        m_lstDifficulty.SetCurSel(idx + 1);
        OnLbnSelchangeLstDiffLevels();
    }
}

void CAdminDlg::OnCbnSelchangeComboChartType()
{
    UpdateMetricCombo();
}

void CAdminDlg::OnCbnSelchangeComboChartMetric()
{
    RefreshChart();
}

void CAdminDlg::OnCbnSelchangeComboChartFilter()
{
    RefreshChart();
}

void CAdminDlg::UpdateMetricCombo()
{
    int type = m_comboChartType.GetCurSel();
    m_comboChartMetric.ResetContent();
    m_comboChartMetric.ShowWindow(SW_SHOW);
    m_lblMetric.ShowWindow(SW_SHOW);

    switch (type)
    {
    case 0:
    case 1:
        m_comboChartMetric.AddString(_T("Всього ігор"));
        m_comboChartMetric.AddString(_T("Всього перемог"));
        m_comboChartMetric.AddString(_T("Найпоп. стаканчик"));
        m_comboChartMetric.AddString(_T("Найпоп. кулька"));
        m_comboChartMetric.AddString(_T("Нові реєстрації"));
        break;
    case 2:
        m_comboChartMetric.AddString(_T("Всього ігор"));
        m_comboChartMetric.AddString(_T("Всього перемог"));
        m_comboChartMetric.AddString(_T("Відсоток перемог (%)"));
        break;
    case 3:
        m_comboChartMetric.AddString(_T("Всього ігор"));
        m_comboChartMetric.AddString(_T("Всього перемог"));
        break;
    case 4:
        m_comboChartMetric.AddString(_T("Рейтинг (очки)"));
        m_comboChartMetric.AddString(_T("Всього ігор"));
        m_comboChartMetric.AddString(_T("Всього перемог"));
        break;
    case 5:
        m_comboChartMetric.ShowWindow(SW_HIDE);
        m_lblMetric.ShowWindow(SW_HIDE);
        break;
    case 6:
        m_comboChartMetric.AddString(_T("Відсоток перемог (%)"));
        m_comboChartMetric.AddString(_T("Всього ігор"));
        m_comboChartMetric.AddString(_T("Кількість ходів"));
        break;
    case 7:
        m_comboChartMetric.AddString(_T("Всього перемог"));
        m_comboChartMetric.AddString(_T("Всього ігор"));
        m_comboChartMetric.AddString(_T("Відсоток перемог (%)"));
        break;
    case 8:
        m_comboChartMetric.AddString(_T("Всього перемог"));
        m_comboChartMetric.AddString(_T("Всього ігор"));
        m_comboChartMetric.AddString(_T("Відсоток перемог (%)"));
        break;
    case 9:
        m_comboChartMetric.AddString(_T("Днів активності"));
        m_comboChartMetric.AddString(_T("Всього сесій"));
        break;
    default:
        m_comboChartMetric.ShowWindow(SW_HIDE);
        m_lblMetric.ShowWindow(SW_HIDE);
        break;
    }

    if (m_comboChartMetric.GetCount() > 0) m_comboChartMetric.SetCurSel(0);

    UpdateFilterCombo();
}

void CAdminDlg::UpdateFilterCombo()
{
    int type = m_comboChartType.GetCurSel();
    m_comboChartFilter.ResetContent();
    BOOL bShowFilter = FALSE;

    if (type == 5) {
        bShowFilter = TRUE;
        m_comboChartFilter.AddString(_T("Позиція кульки"));
        m_comboChartFilter.AddString(_T("Вибір гравця"));
        m_comboChartFilter.SetCurSel(0);
    }
    else if (type == 7) {
        bShowFilter = TRUE;
        std::vector<CString> diffs = DBHelper::GetInstance().GetUniqueDifficultyNames();
        for (const auto& name : diffs) {
            m_comboChartFilter.AddString(name);
        }
        if (m_comboChartFilter.GetCount() > 0) m_comboChartFilter.SetCurSel(0);
    }

    m_comboChartFilter.ShowWindow(bShowFilter ? SW_SHOW : SW_HIDE);
    m_lblFilter.ShowWindow(bShowFilter ? SW_SHOW : SW_HIDE);

    RefreshChart();
}

CString CAdminDlg::GetCurrentMetricDBName()
{
    int type = m_comboChartType.GetCurSel();
    int metricIdx = m_comboChartMetric.GetCurSel();

    if (metricIdx < 0 && m_comboChartMetric.IsWindowVisible()) return _T("");

    switch (type)
    {
    case 0: case 1:
        if (metricIdx == 0) return _T("total_games");
        if (metricIdx == 1) return _T("total_wins");
        if (metricIdx == 2) return _T("top_cup");
        if (metricIdx == 3) return _T("top_ball");
        if (metricIdx == 4) return _T("total_registrations");
        break;
    case 2:
        if (metricIdx == 0) return _T("total_games");
        if (metricIdx == 1) return _T("total_wins");
        if (metricIdx == 2) return _T("win_rate_percent");
        break;
    case 3:
        if (metricIdx == 0) return _T("total_games");
        if (metricIdx == 1) return _T("total_wins");
        break;
    case 4:
        if (metricIdx == 0) return _T("rank_score");
        if (metricIdx == 1) return _T("total_games");
        if (metricIdx == 2) return _T("total_wins");
        break;
    case 6:
        if (metricIdx == 0) return _T("win_rate");
        if (metricIdx == 1) return _T("total_games");
        if (metricIdx == 2) return _T("shuffle_count");
        break;
    case 7:
        if (metricIdx == 0) return _T("wins");
        if (metricIdx == 1) return _T("games_played");
        if (metricIdx == 2) return _T("win_rate");
        break;
    case 8:
        if (metricIdx == 0) return _T("total_wins");
        if (metricIdx == 1) return _T("total_games");
        if (metricIdx == 2) return _T("win_rate_percent");
        break;
    case 9:
        if (metricIdx == 0) return _T("days_active");
        if (metricIdx == 1) return _T("total_sessions");
        break;
    }
    return _T("");
}

void CAdminDlg::RefreshChart()
{
    BOOL bIs3D = (m_check3D.GetCheck() == BST_CHECKED);
    m_chartControl.SetEnable3D(bIs3D);
    m_chartControl.SetShowGrid(TRUE);
    m_chartControl.SetShowLabels(TRUE);
    m_chartControl.SetBackColor(RGB(255, 255, 255));
    m_chartControl.SetCoefA(1.0f);
    m_chartControl.SetCoefB(1.0f);

    int type = m_comboChartType.GetCurSel();
    CString title; m_comboChartType.GetLBText(type, title);

    if (m_comboChartMetric.IsWindowVisible()) {
        CString metricName;
        int mIdx = m_comboChartMetric.GetCurSel();
        if (mIdx >= 0) {
            m_comboChartMetric.GetLBText(mIdx, metricName);
            title += _T(" : ") + metricName;
        }
    }
    m_chartControl.SetChartTitle(title);

    std::vector<ChartEntry> data;
    CString dbMetric = GetCurrentMetricDBName();

    switch (type)
    {
    case 0: data = DBHelper::GetInstance().GetMonthlyStats(dbMetric); break;
    case 1: data = DBHelper::GetInstance().GetWeeklyStats(dbMetric); break;
    case 2: data = DBHelper::GetInstance().GetDifficultyPerformance(dbMetric); break;
    case 3: data = DBHelper::GetInstance().GetHourlyActivity(dbMetric); break;
    case 4: data = DBHelper::GetInstance().GetLeaderboardWeighted(dbMetric); break;
    case 5: {
        CString filter = _T("ball");
        int sel = m_comboChartFilter.GetCurSel();
        if (sel == 1) {
            filter = _T("player");
        }
        data = DBHelper::GetInstance().GetPositionBias(filter);
        break;
    }
    case 6: data = DBHelper::GetInstance().GetSpeedVsWinrate(dbMetric); break;
    case 7: {
        CString levelName;
        int fIdx = m_comboChartFilter.GetCurSel();
        if (fIdx >= 0) {
            m_comboChartFilter.GetLBText(fIdx, levelName);
            data = DBHelper::GetInstance().GetTopByDifficulty(levelName, dbMetric);
        }
        break;
    }
    case 8: data = DBHelper::GetInstance().GetTopPlayers(dbMetric); break;
    case 9: data = DBHelper::GetInstance().GetUserRetention(dbMetric); break;
    case 10: data = DBHelper::GetInstance().GetSkyboxPopularity(); break;
    case 11: data = DBHelper::GetInstance().GetTableMatPopularity(); break;
    case 12: data = DBHelper::GetInstance().GetCarpetMatPopularity(); break;
    }

    COLORREF chartPalette[] = {
        RGB(0x1F,0x77,0xB4),
        RGB(0xFF,0x7F,0x0E),
        RGB(0x2C,0xA0,0x2C),
        RGB(0xD6,0x27,0x28),
        RGB(0x94,0x67,0xBD),
        RGB(0x8C,0x56,0x4B),
        RGB(0xE3,0x77,0xC2),
        RGB(0x7F,0x7F,0x7F),
        RGB(0xBC,0xBD,0x22),
        RGB(0x17,0xBE,0xCF)
    };
    int paletteSize = sizeof(chartPalette) / sizeof(COLORREF);

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = (ULONG)max(1, data.size());
    SAFEARRAY* psaColors = SafeArrayCreate(VT_I4, 1, rgsabound);

    long idx = 0;
    for (const auto& item : data) {
        long colorVal;

        if (item.color != 0xFFFFFFFF) {
            colorVal = (long)item.color;
        }
        else {
            colorVal = (long)chartPalette[idx % paletteSize];
        }

        SafeArrayPutElement(psaColors, &idx, &colorVal);
        idx++;
    }

    VARIANT vColors;
    VariantInit(&vColors);
    vColors.vt = VT_ARRAY | VT_I4;
    vColors.parray = psaColors;
    m_chartControl.SetColors(vColors);
    VariantClear(&vColors);

    SAFEARRAYBOUND outerBound;
    outerBound.lLbound = 0;
    outerBound.cElements = (ULONG)data.size();
    SAFEARRAY* psaOuter = SafeArrayCreate(VT_VARIANT, 1, &outerBound);

    LONG rowIdx = 0;
    for (auto& it : data)
    {
        SAFEARRAYBOUND innerBound;
        innerBound.lLbound = 0;
        innerBound.cElements = 2;
        SAFEARRAY* psaInner = SafeArrayCreate(VT_VARIANT, 1, &innerBound);

        VARIANT v;
        LONG colIdx = 0;

        VariantInit(&v);
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(it.label);
        SafeArrayPutElement(psaInner, &colIdx, &v);
        VariantClear(&v);
        colIdx++;

        VariantInit(&v);
        v.vt = VT_R8;
        v.dblVal = it.value;
        SafeArrayPutElement(psaInner, &colIdx, &v);
        VariantClear(&v);

        VARIANT vRow;
        VariantInit(&vRow);
        vRow.vt = VT_ARRAY | VT_VARIANT;
        vRow.parray = psaInner;
        SafeArrayPutElement(psaOuter, &rowIdx, &vRow);

        rowIdx++;
    }

    VARIANT vData;
    VariantInit(&vData);
    vData.vt = VT_ARRAY | VT_VARIANT;
    vData.parray = psaOuter;

    if (!data.empty()) {
        m_chartControl.LoadData(vData);
    }
    else {
    }

    VariantClear(&vData);
}

void CAdminDlg::OnBnClickedBtnSortAsc()
{
    m_chartControl.Sort(1);
}

void CAdminDlg::OnBnClickedBtnSortDesc()
{
    m_chartControl.Sort(0);
}

void CAdminDlg::OnBnClickedCheck3d()
{
    RefreshChart();
}

void CAdminDlg::OnBnClickedBtnExpExcel()
{
    m_chartControl.ExportToExcel();
}

void CAdminDlg::OnBnClickedBtnExpWord()
{
    m_chartControl.ExportToWord();
}

void CAdminDlg::OnOK() {}

void CAdminDlg::SetupPlayerStatsList()
{
    m_listAdmGames.SetExtendedStyle(
        m_listAdmGames.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    while (m_listAdmGames.DeleteColumn(0)) {}

    m_listAdmGames.InsertColumn(0, _T("Параметр"), LVCFMT_LEFT, 200);
    m_listAdmGames.InsertColumn(1, _T("Значення"),  LVCFMT_LEFT, 380);
}

void CAdminDlg::RefreshPlayerCombo()
{
    m_comboAdmPlayer.ResetContent();
    std::vector<PlayerListEntry> players = DBHelper::GetInstance().GetAllPlayers();
    for (const auto& p : players) {
        int idx = m_comboAdmPlayer.AddString(p.username);
        m_comboAdmPlayer.SetItemData(idx, (DWORD_PTR)p.userId);
    }
}

void CAdminDlg::OnCbnSelchangeComboAdmPlayer()
{
    int idx = m_comboAdmPlayer.GetCurSel();
    if (idx == CB_ERR) return;
    m_nSelectedPlayerID = (int)m_comboAdmPlayer.GetItemData(idx);
    LoadPlayerStats(m_nSelectedPlayerID);
}

static void InsertKV(CListCtrl& list, int& row, const CString& key, const CString& value)
{
    int i = list.InsertItem(row, key);
    list.SetItemText(i, 1, value);
    row++;
}

static CString FormatInt(int v)
{
    CString s; s.Format(_T("%d"), v); return s;
}

static CString FormatDouble(double v, int digits = 1)
{
    CString s; s.Format(_T("%.*f"), digits, v); return s;
}

static CString FormatDateOrDash(const CString& v)
{
    return v.IsEmpty() ? CString(_T("—")) : v;
}

static CString CupName(int pos)
{
    switch (pos) {
        case 1: return _T("Ліворуч");
        case 2: return _T("По центру");
        case 3: return _T("Праворуч");
        default: { CString s; s.Format(_T("#%d"), pos); return s; }
    }
}

void CAdminDlg::LoadPlayerStats(int userId)
{
    m_listAdmGames.DeleteAllItems();
    int row = 0;

    PlayerStats stats{};
    bool ok = DBHelper::GetInstance().GetPlayerStats(userId, stats);
    if (!ok) {
        CString err = DBHelper::GetInstance().GetLastDbError();
        InsertKV(m_listAdmGames, row, _T("Помилка"),
            err.IsEmpty() ? CString(_T("Не вдалося завантажити дані")) : err);
        return;
    }

    InsertKV(m_listAdmGames, row, _T("ID користувача"),   FormatInt(stats.userId));
    InsertKV(m_listAdmGames, row, _T("Логін"),            stats.username);
    InsertKV(m_listAdmGames, row, _T("Роль"),             stats.roleName);
    InsertKV(m_listAdmGames, row, _T("Зареєстровано"),    FormatDateOrDash(stats.signedUpAt));
    InsertKV(m_listAdmGames, row, _T("Обрана складність"),
        stats.currentDifficultyName.IsEmpty() ? CString(_T("—")) : stats.currentDifficultyName);

    InsertKV(m_listAdmGames, row, _T(""), _T(""));
    InsertKV(m_listAdmGames, row, _T("• Загальна статистика •"), _T(""));

    InsertKV(m_listAdmGames, row, _T("Всього матчів"),    FormatInt(stats.totalGames));
    InsertKV(m_listAdmGames, row, _T("Перемог"),          FormatInt(stats.totalWins));
    InsertKV(m_listAdmGames, row, _T("Поразок"),          FormatInt(stats.totalLosses));
    InsertKV(m_listAdmGames, row, _T("Winrate, %"),       FormatDouble(stats.winRatePercent));
    InsertKV(m_listAdmGames, row, _T("Перший матч"),      FormatDateOrDash(stats.firstPlayed));
    InsertKV(m_listAdmGames, row, _T("Останній матч"),    FormatDateOrDash(stats.lastPlayed));

    InsertKV(m_listAdmGames, row, _T(""), _T(""));
    InsertKV(m_listAdmGames, row, _T("• Активність •"), _T(""));
    InsertKV(m_listAdmGames, row, _T("Матчів за 24г"),    FormatInt(stats.gamesLast24h));
    InsertKV(m_listAdmGames, row, _T("Матчів за 7 днів"), FormatInt(stats.gamesLast7d));
    InsertKV(m_listAdmGames, row, _T("Матчів за 30 днів"),FormatInt(stats.gamesLast30d));

    InsertKV(m_listAdmGames, row, _T(""), _T(""));
    InsertKV(m_listAdmGames, row, _T("• Уподобання •"), _T(""));
    InsertKV(m_listAdmGames, row, _T("Улюблена складність"),
        stats.favouriteDifficulty.IsEmpty() ? CString(_T("—")) : stats.favouriteDifficulty);
    InsertKV(m_listAdmGames, row, _T("Skybox (id)"),  FormatDateOrDash(stats.favouriteSkybox));
    InsertKV(m_listAdmGames, row, _T("Стіл (id)"),    FormatDateOrDash(stats.favouriteTabMat));
    InsertKV(m_listAdmGames, row, _T("Килим (id)"),   FormatDateOrDash(stats.favouriteCarpMat));

    std::vector<PlayerBreakdownRow> breakdown = DBHelper::GetInstance().GetPlayerBreakdown(userId);
    if (!breakdown.empty()) {
        InsertKV(m_listAdmGames, row, _T(""), _T(""));
        InsertKV(m_listAdmGames, row, _T("• Розбивка за складністю •"), _T(""));
        for (const auto& r : breakdown) {
            CString val;
            val.Format(_T("%d / %d  (%.1f%%)"), r.totalWins, r.totalGames, r.winRatePercent);
            InsertKV(m_listAdmGames, row, r.levelName, val);
        }
    }

    std::vector<PlayerSessionRow> recent = DBHelper::GetInstance().GetPlayerRecentSessions(userId, 30);
    if (!recent.empty()) {
        InsertKV(m_listAdmGames, row, _T(""), _T(""));
        InsertKV(m_listAdmGames, row, _T("• Останні матчі •"), _T(""));
        for (const auto& s : recent) {
            CString key, val;
            key.Format(_T("%s — %s"),
                s.playedAt.IsEmpty() ? _T("?") : (LPCTSTR)s.playedAt,
                (LPCTSTR)s.levelName);
            val.Format(_T("Кулька %s · вибір %s · %s"),
                (LPCTSTR)CupName(s.ballPosition),
                (LPCTSTR)CupName(s.selectedCup),
                s.isWin ? _T("ПЕРЕМОГА") : _T("поразка"));
            InsertKV(m_listAdmGames, row, key, val);
        }
    }
}