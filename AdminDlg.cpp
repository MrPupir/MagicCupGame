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
}


BEGIN_MESSAGE_MAP(CAdminDlg, CDialogEx)
    ON_CBN_SELCHANGE(IDC_COMBO_ADM_MODE, &CAdminDlg::OnCbnSelchangeComboAdmMode)
    ON_LBN_SELCHANGE(IDC_LST_DIFF_LEVELS, &CAdminDlg::OnLbnSelchangeLstDiffLevels)
    ON_BN_CLICKED(IDC_BTN_DIFF_ADD, &CAdminDlg::OnBnClickedBtnDiffAdd)
    ON_BN_CLICKED(IDC_BTN_DIFF_SAVE, &CAdminDlg::OnBnClickedBtnDiffSave)
    ON_BN_CLICKED(IDC_BTN_DIFF_DEL, &CAdminDlg::OnBnClickedBtnDiffDel)
	ON_BN_CLICKED(IDC_BTN_DIFF_UP, &CAdminDlg::OnBnClickedBtnDiffUp)
	ON_BN_CLICKED(IDC_BTN_DIFF_DOWN, &CAdminDlg::OnBnClickedBtnDiffDown)
END_MESSAGE_MAP()


// Обработчики сообщений CAdminDlg

BOOL CAdminDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_comboMode.AddString(_T("Редактор складності"));
	m_comboMode.AddString(_T("Статистика гравців")); 
	m_comboMode.AddString(_T("Діаграми"));

	m_comboMode.SetCurSel(0);

	UpdateVisibility();

    RefreshDifficultyList();

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
		IDC_GRP_CHART, IDC_STATIC_CHART_PLACEHOLDER
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

void CAdminDlg::OnOK()
{

}
