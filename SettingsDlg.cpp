// SettingsDlg.cpp: файл реализации
//

#include "pch.h"
#include "MagicCupGame.h"
#include "afxdialogex.h"
#include "SettingsDlg.h"


// Диалоговое окно CSettingsDlg

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTINGS, pParent)
{

}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_DIFFICULTY, m_comboDifficulty);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
END_MESSAGE_MAP()


// Обработчики сообщений CSettingsDlg
BOOL CSettingsDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_levels = DBHelper::GetInstance().GetDifficultyLevels();

    if (m_levels.empty()) {
        AfxMessageBox(_T("Не вдалося завантажити рівні або таблиця порожня!"));
        return TRUE;
    }

    int selectedIndex = 0;

    for (size_t i = 0; i < m_levels.size(); i++) {
        int index = m_comboDifficulty.AddString(m_levels[i].name);
        m_comboDifficulty.SetItemData(index, (DWORD_PTR)i);

        if (m_levels[i].id == m_nInitialId) {
            selectedIndex = index;
        }
    }

    m_comboDifficulty.SetCurSel(selectedIndex);

    return TRUE;
}

void CSettingsDlg::OnOK()
{
    int curSel = m_comboDifficulty.GetCurSel();

    if (curSel != CB_ERR) {
        int vectorIndex = (int)m_comboDifficulty.GetItemData(curSel);

        m_selectedLevel = m_levels[vectorIndex];

        CDialogEx::OnOK();
    }
    else {
        AfxMessageBox(_T("Выберите сложность!"));
    }
}