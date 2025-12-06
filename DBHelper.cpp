#include "pch.h"
#include "DBHelper.h"
#include <functional>
#include <string>

CString HashPassword(CString password)
{
    std::wstring str(password.GetString());

    size_t hash = std::hash<std::wstring>{}(str);

    CString strHash;
    strHash.Format(_T("%zu"), hash);
    return strHash;
}

bool DBHelper::Connect()
{
    if (m_db.IsOpen()) return true;

    try {
        CString sConnectionString = _T("Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;Database=MagicCupDB;User=root;Password=;Option=3;");

        m_db.OpenEx(sConnectionString, CDatabase::noOdbcDialog);
        return true;
    }
    catch (CDBException* e) {
        AfxMessageBox(_T("Помилка підключення до БД: ") + e->m_strError);
        e->Delete();
        return false;
    }
}

void DBHelper::Disconnect()
{
    if (m_db.IsOpen()) m_db.Close();
}

int DBHelper::TryLogin(CString username, CString password)
{
    if (!Connect()) return -1;

    CRecordset rs(&m_db);
    try {
        CString query;
        CString passwordHash = HashPassword(password);
        query.Format(_T("SELECT user_id FROM users WHERE username='%s' AND password_hash='%s'"), username, passwordHash);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);

        if (!rs.IsEOF()) {
            CString val;
            rs.GetFieldValue((short)0, val);
            return _ttoi(val);
        }
    }
    catch (CDBException* e) {
        AfxMessageBox(e->m_strError);
        e->Delete();
    }
    return -1;
}

int DBHelper::RegisterUser(CString username, CString password)
{
    if (!Connect()) return -1;

    CRecordset rs(&m_db);
    CString checkQuery;
    checkQuery.Format(_T("SELECT user_id FROM users WHERE username='%s'"), username);
    rs.Open(CRecordset::forwardOnly, checkQuery, CRecordset::readOnly);
    if (!rs.IsEOF()) {
        AfxMessageBox(_T("Користувач з таким ім'ям вже існує!"));
        return -1;
    }
    rs.Close();

    try {
        CString insertQuery;
        CString passwordHash = HashPassword(password);
        insertQuery.Format(_T("INSERT INTO users (username, password_hash, role_id) VALUES ('%s', '%s', 2)"), username, passwordHash);
        m_db.ExecuteSQL(insertQuery);

        return TryLogin(username, password);
    }
    catch (CDBException* e) {
        AfxMessageBox(_T("Помилка реєстрації: ") + e->m_strError);
        e->Delete();
        return -1;
    }
}

std::vector<DifficultyLevel> DBHelper::GetDifficultyLevels()
{
    std::vector<DifficultyLevel> levels;
    if (!Connect()) return levels;

    CRecordset rs(&m_db);
    try {
        rs.Open(CRecordset::forwardOnly, _T("SELECT difficulty_id, level_name, shuffle_count, animation_speed_ms FROM difficulty_levels ORDER BY difficulty_id ASC"), CRecordset::readOnly);

        while (!rs.IsEOF()) {
            DifficultyLevel lvl;
            CString val;

            rs.GetFieldValue((short)0, val); lvl.id = _ttoi(val);
            rs.GetFieldValue((short)1, lvl.name);
            rs.GetFieldValue((short)2, val); lvl.shuffleCount = _ttoi(val);
            rs.GetFieldValue((short)3, val); lvl.animationSpeed = _ttoi(val);

            levels.push_back(lvl);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) {
        AfxMessageBox(e->m_strError);
        e->Delete();
    }
    return levels;
}

bool DBHelper::SaveUserDifficulty(int userId, int difficultyId)
{
    if (!Connect()) return false;

    try {
        CString query;
        query.Format(_T("UPDATE users SET selected_difficulty = %d WHERE user_id = %d"), difficultyId, userId);

        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
        AfxMessageBox(_T("Помилка збереження складності: ") + e->m_strError);
        e->Delete();
        return false;
    }
}

int DBHelper::GetUserDifficulty(int userId)
{
    if (!Connect()) return 1;

    CRecordset rs(&m_db);
    try {
        CString query;
        query.Format(_T("SELECT selected_difficulty FROM users WHERE user_id = %d"), userId);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);

        if (!rs.IsEOF()) {
            CString val;
            rs.GetFieldValue((short)0, val);
            int id = _ttoi(val);

            return (id > 0) ? id : 1;
        }
    }
    catch (CDBException* e) {
        AfxMessageBox(_T("Помилка отримання складності: ") + e->m_strError);
        e->Delete();
    }
    return 1;
}

bool DBHelper::AddGameSession(int userId, int difficultyId, int ballPos, int selectedCup, bool isWin)
{
    if (!Connect()) return false;

    try {
        CString query;
        query.Format(_T("INSERT INTO game_sessions (user_id, difficulty_id, ball_position, selected_cup, is_win) VALUES (%d, %d, %d, %d, %d)"),
            userId, difficultyId, ballPos, selectedCup, isWin ? 1 : 0);

        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
        AfxMessageBox(_T("Помилка запису статистики: ") + e->m_strError);
        e->Delete();
        return false;
    }
}