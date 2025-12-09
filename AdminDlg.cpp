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
    DDX_Control(pDX, IDC_CHECK_3D, m_check3D);
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
    ON_BN_CLICKED(IDC_BTN_COLORS, &CAdminDlg::OnBnClickedBtnColors)
    ON_CBN_SELCHANGE(IDC_COMBO_CHART_TYPE, &CAdminDlg::OnCbnSelchangeComboChartType)
END_MESSAGE_MAP()


// Обработчики сообщений CAdminDlg

BOOL CAdminDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_comboMode.AddString(_T("Редактор складності"));
	m_comboMode.AddString(_T("Статистика гравців")); 
	m_comboMode.AddString(_T("Діаграми"));
	m_comboMode.SetCurSel(0);

    m_comboChartType.AddString(_T("Топ гравців (Win)"));
    m_comboChartType.AddString(_T("Активність (Ігри)"));
    m_comboChartType.SetCurSel(0);

    m_check3D.SetCheck(BST_CHECKED);

	UpdateVisibility();

    RefreshDifficultyList();
    RefreshChart();

	return TRUE;
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
        IDC_BTN_SORT_ASC, IDC_BTN_SORT_DESC,
        IDC_CHECK_3D, IDC_BTN_COLORS,
        IDC_BTN_EXP_EXCEL, IDC_BTN_EXP_WORD
    };

    ShowControls(groupDiff, (index == 0));
    ShowControls(groupStats, (index == 1));
    ShowControls(groupChart, (index == 2));
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

void CAdminDlg::RefreshChart()
{
    BOOL bIs3D = (m_check3D.GetCheck() == BST_CHECKED);
    m_chartControl.SetEnable3D(bIs3D);

    int type = m_comboChartType.GetCurSel();
    if (type == 0) {
        m_chartControl.SetChartTitle(_T("Результаты Турнира: Top 3"));
    }
    else {
        m_chartControl.SetChartTitle(_T("Активність гравців"));
    }

    m_chartControl.SetCoefA(1.5);
    m_chartControl.SetCoefB(0.8);

    m_chartControl.SetShowGrid(TRUE);
    m_chartControl.SetShowLabels(TRUE);

    m_chartControl.SetBackColor(RGB(255, 255, 255));
    m_chartControl.SetAxisColor(RGB(50, 50, 50));
    m_chartControl.SetGridColor(RGB(220, 220, 220));

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = 9;
    SAFEARRAY* psaColors = SafeArrayCreate(VT_R4, 1, rgsabound);

    long idx = 0;
    float val;

    val = 0.78f; SafeArrayPutElement(psaColors, &idx, &val); idx++;
    val = 0.20f; SafeArrayPutElement(psaColors, &idx, &val); idx++;
    val = 0.20f; SafeArrayPutElement(psaColors, &idx, &val); idx++;

    val = 0.20f; SafeArrayPutElement(psaColors, &idx, &val); idx++;
    val = 0.70f; SafeArrayPutElement(psaColors, &idx, &val); idx++;
    val = 0.20f; SafeArrayPutElement(psaColors, &idx, &val); idx++;

    val = 0.20f; SafeArrayPutElement(psaColors, &idx, &val); idx++;
    val = 0.20f; SafeArrayPutElement(psaColors, &idx, &val); idx++;
    val = 0.78f; SafeArrayPutElement(psaColors, &idx, &val); idx++;

    VARIANT vColors;
    VariantInit(&vColors);
    vColors.vt = VT_ARRAY | VT_R4;
    vColors.parray = psaColors;

    m_chartControl.SetColors(vColors);
    VariantClear(&vColors);

    struct ChartItem { CString name; double p1; double p2; };

    std::vector<ChartItem> items;
    if (type == 0) {
        items = {
            { _T("PlayerOne"),    5.0, 10.0 },
            { _T("CyberSlayer"),  8.0, 12.0 },
            { _T("Winner2000"),  15.0, 15.0 }
        };
    }
    else {
        items = {
            { _T("Пн"), 10.0, 2.0 },
            { _T("Вт"), 12.0, 5.0 },
            { _T("Ср"), 8.0,  3.0 },
            { _T("Чт"), 20.0, 10.0 },
            { _T("Пт"), 25.0, 15.0 }
        };
    }

    SAFEARRAYBOUND outerBound;
    outerBound.lLbound = 0;
    outerBound.cElements = (ULONG)items.size();

    SAFEARRAY* psaOuter = SafeArrayCreate(VT_VARIANT, 1, &outerBound);

    LONG rowIdx = 0;
    for (auto& it : items)
    {
        SAFEARRAYBOUND innerBound;
        innerBound.lLbound = 0;
        innerBound.cElements = 3;
        SAFEARRAY* psaInner = SafeArrayCreate(VT_VARIANT, 1, &innerBound);

        VARIANT v;
        LONG colIdx = 0;

        VariantInit(&v);
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(it.name);
        SafeArrayPutElement(psaInner, &colIdx, &v);
        VariantClear(&v);
        colIdx++;

        VariantInit(&v);
        v.vt = VT_R8;
        v.dblVal = it.p1;
        SafeArrayPutElement(psaInner, &colIdx, &v);
        VariantClear(&v);
        colIdx++;

        VariantInit(&v);
        v.vt = VT_R8;
        v.dblVal = it.p2;
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

    m_chartControl.LoadData(vData);

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

void CAdminDlg::OnBnClickedBtnColors()
{
    AfxMessageBox(_T("Діалог налаштування кольорів (Coming soon)"));
}

void CAdminDlg::OnCbnSelchangeComboChartType()
{
    RefreshChart();
}

void CAdminDlg::OnOK() {}