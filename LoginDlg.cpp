// LoginDlg.cpp: файл реализации
//

#include "pch.h"
#include "MagicCupGame.h"
#include "afxdialogex.h"
#include "LoginDlg.h"
#include "RegisterDlg.h"
#include "DBHelper.h"

// Диалоговое окно CLoginDlg

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_GOTO_REG, &CLoginDlg::OnBnClickedBtnGotoReg)
	ON_BN_CLICKED(IDOK, &CLoginDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// Обработчики сообщений CLoginDlg

void CLoginDlg::OnBnClickedOk()
{
    CString strUser, strPass;
    GetDlgItemText(IDC_EDIT_LOGIN_USER, strUser);
    GetDlgItemText(IDC_EDIT_LOGIN_PASS, strPass);

    int userId = DBHelper::GetInstance().TryLogin(strUser, strPass);

    if (userId != -1) {
        m_nUserID = userId;
        CDialogEx::OnOK();
    }
    else {
        AfxMessageBox(_T("Неверный логин или пароль!"));
    }
}

void CLoginDlg::OnBnClickedBtnGotoReg()
{
    CRegisterDlg regDlg;

    if (regDlg.DoModal() == IDOK)
    {
        if (regDlg.m_nNewUserID != -1)
        {
            this->m_nUserID = regDlg.m_nNewUserID;

            CDialogEx::OnOK();
        }
    }
}