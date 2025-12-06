// RegisterDlg.cpp: файл реализации
//

#include "pch.h"
#include "MagicCupGame.h"
#include "afxdialogex.h"
#include "RegisterDlg.h"
#include "DBHelper.h"


// Диалоговое окно CRegisterDlg

IMPLEMENT_DYNAMIC(CRegisterDlg, CDialogEx)

CRegisterDlg::CRegisterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REGISTER, pParent)
{

}

CRegisterDlg::~CRegisterDlg()
{
}

void CRegisterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRegisterDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CRegisterDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// Обработчики сообщений CRegisterDlg

void CRegisterDlg::OnBnClickedOk()
{
    UpdateData(TRUE);

    CString strUser, strPass, strPass2;
    GetDlgItemText(IDC_EDIT_REG_USER, strUser);
    GetDlgItemText(IDC_EDIT_REG_PASS, strPass);
    GetDlgItemText(IDC_EDIT_REG_PASS2, strPass2);

    if (strUser.IsEmpty() || strPass.IsEmpty()) {
        AfxMessageBox(_T("Заполните все поля!"));
        return;
    }
    if (strPass != strPass2) {
        AfxMessageBox(_T("Пароли не совпадают!"));
        return;
    }

    int newID = DBHelper::GetInstance().RegisterUser(strUser, strPass);

    if (newID != -1) {
        m_nNewUserID = newID;
        AfxMessageBox(_T("Регистрация успешна! Вход выполнен автоматически."));
        CDialogEx::OnOK();
    }
}