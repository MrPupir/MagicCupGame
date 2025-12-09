#include "pch.h"
#include "DBHelper.h"
#include <functional>
#include <string>
#include <vector>

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
        rs.Open(CRecordset::forwardOnly, _T("SELECT difficulty_id, level_name, shuffle_count, animation_speed_ms FROM difficulty_levels ORDER BY sort_order ASC"), CRecordset::readOnly);
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
        e->Delete();
        return false;
    }
}

bool DBHelper::IsAdminUser(int userId)
{
    if (!Connect()) return false;
    CRecordset rs(&m_db);
    try {
        CString query;
        query.Format(_T("SELECT role_id FROM users WHERE user_id = %d"), userId);
        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        if (!rs.IsEOF()) {
            CString val;
            rs.GetFieldValue((short)0, val);
            int roleId = _ttoi(val);
            return (roleId == 1);
        }
    }
    catch (CDBException* e) {
        e->Delete();
    }
    return false;
}

bool DBHelper::AddDifficultyLevel(CString name, int count, int speed)
{
    if (!Connect()) return false;
    try {
        CString query;
        query.Format(_T("INSERT INTO difficulty_levels (level_name, shuffle_count, animation_speed_ms) VALUES ('%s', %d, %d)"),
            name, count, speed);
        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
        e->Delete();
        return false;
    }
}

bool DBHelper::UpdateDifficultyLevel(int id, CString name, int count, int speed)
{
    if (!Connect()) return false;
    try {
        CString query;
        query.Format(_T("UPDATE difficulty_levels SET level_name='%s', shuffle_count=%d, animation_speed_ms=%d WHERE difficulty_id=%d"),
            name, count, speed, id);
        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
        e->Delete();
        return false;
    }
}

bool DBHelper::DeleteDifficultyLevel(int id)
{
    if (!Connect()) return false;
    try {
        CString query;
        query.Format(_T("DELETE FROM difficulty_levels WHERE difficulty_id=%d"), id);
        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
        e->Delete();
        return false;
    }
}

bool DBHelper::SwapDifficultyOrder(int id1, int id2)
{
    if (!Connect()) return false;
    try {
        CString query;
        query.Format(_T("UPDATE difficulty_levels AS t1 JOIN difficulty_levels AS t2 ON (t1.difficulty_id = %d AND t2.difficulty_id = %d) SET t1.sort_order = t2.sort_order, t2.sort_order = t1.sort_order"), id1, id2);
        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
        return false;
    }
}

bool DBHelper::SaveUserSkybox(int userId, int skyboxIndex)
{
    if (!Connect()) return false;
    try {
        CString query;
        query.Format(_T("UPDATE users SET selected_skybox = %d WHERE user_id = %d"), skyboxIndex, userId);
        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
        e->Delete();
        return false;
    }
}

int DBHelper::GetUserSkybox(int userId)
{
    if (!Connect()) return 0;
    CRecordset rs(&m_db);
    try {
        CString query;
        query.Format(_T("SELECT selected_skybox FROM users WHERE user_id = %d"), userId);
        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        if (!rs.IsEOF()) {
            CString val;
            rs.GetFieldValue((short)0, val);
            return _ttoi(val);
        }
    }
    catch (CDBException* e) {
        e->Delete();
    }
    return 0;
}

CString DBHelper::JsonVal(const CString& json, const CString& key)
{
    CString searchKey = _T("\"") + key + _T("\":");
    int pos = json.Find(searchKey);
    if (pos == -1) return _T("");

    pos += searchKey.GetLength();
    while (pos < json.GetLength() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos >= json.GetLength()) return _T("");

    bool isString = (json[pos] == '"');
    if (isString) pos++;

    int endPos;
    if (isString) {
        endPos = json.Find(_T("\""), pos);
    }
    else {
        int commaPos = json.Find(_T(","), pos);
        int bracePos = json.Find(_T("}"), pos);
        if (commaPos == -1) endPos = bracePos;
        else if (bracePos == -1) endPos = commaPos;
        else endPos = min(commaPos, bracePos);
    }

    if (endPos == -1) return _T("");

    return json.Mid(pos, endPos - pos);
}

int DBHelper::JsonInt(const CString& json, const CString& key, int defVal) {
    CString val = JsonVal(json, key);
    return val.IsEmpty() ? defVal : _ttoi(val);
}

unsigned long DBHelper::JsonULong(const CString& json, const CString& key, unsigned long defVal) {
    CString val = JsonVal(json, key);
    return val.IsEmpty() ? defVal : _ttol(val);
}

CString DBHelper::JsonStr(const CString& json, const CString& key, const CString& defVal) {
    CString val = JsonVal(json, key);
    return val.IsEmpty() ? defVal : val;
}

bool DBHelper::SaveGameSettings(int userId, const GameSettings& s)
{
    if (!Connect()) return false;
    try {
        CString json;
        json.Format(_T("{")
            _T("\"top_name\":\"%s\",\"top_size\":%d,\"top_bold\":%d,\"top_italic\":%d,\"top_u\":%d,\"top_s\":%d,\"top_stroke\":%d,\"top_col\":%lu,\"top_th\":%d,")
            _T("\"btn_name\":\"%s\",\"btn_size\":%d,\"btn_bold\":%d,\"btn_italic\":%d,\"btn_u\":%d,\"btn_s\":%d,\"btn_stroke\":%d,\"btn_col\":%lu,\"btn_th\":%d,")
            _T("\"act_name\":\"%s\",\"act_size\":%d,\"act_bold\":%d,\"act_italic\":%d,\"act_u\":%d,\"act_s\":%d,\"act_stroke\":%d,\"act_col\":%lu,\"act_th\":%d,")
            _T("\"hud_bg\":%lu,\"hud_border\":%lu,\"hud_b_en\":%d,")
            _T("\"cross_en\":%d,\"cross_col\":%lu,\"cross_b_en\":%d,\"cross_b_col\":%lu,\"cross_th\":%d,")
            _T("\"cup_col\":%lu,\"ball_col\":%lu,")
            _T("\"skybox\":%d,")
            _T("\"tab_en\":%d,\"tab_mat\":%d,")
            _T("\"carp_en\":%d,\"carp_mat\":%d")
            _T("}"),
            s.topMenuFont.fontName, s.topMenuFont.fontSize, s.topMenuFont.fontWeight, s.topMenuFont.isItalic, s.topMenuFont.isUnderline, s.topMenuFont.isStrikeOut, s.topMenuFont.useStroke, s.topMenuFont.strokeColor, s.topMenuFont.strokeThickness,
            s.buttonFont.fontName, s.buttonFont.fontSize, s.buttonFont.fontWeight, s.buttonFont.isItalic, s.buttonFont.isUnderline, s.buttonFont.isStrikeOut, s.buttonFont.useStroke, s.buttonFont.strokeColor, s.buttonFont.strokeThickness,
            s.actionFont.fontName, s.actionFont.fontSize, s.actionFont.fontWeight, s.actionFont.isItalic, s.actionFont.isUnderline, s.actionFont.isStrikeOut, s.actionFont.useStroke, s.actionFont.strokeColor, s.actionFont.strokeThickness,
            s.hudRectColor, s.hudBorderColor, s.hudBorderEnabled,
            s.crosshairEnabled, s.crosshairColor, s.crosshairBorderEnabled, s.crosshairBorderColor, s.crosshairBorderThickness,
            s.cupColor, s.ballColor,
            s.skyboxIndex,
            s.tableEnabled, s.tableMatIndex,
            s.carpetEnabled, s.carpetMatIndex
        );

        CString query;
        json.Replace(_T("'"), _T("''"));
        query.Format(_T("UPDATE users SET settings = '%s' WHERE user_id = %d"), json, userId);

        m_db.ExecuteSQL(query);
        return true;
    }
    catch (CDBException* e) {
		AfxMessageBox(_T("Помилка збереження налаштувань: ") + e->m_strError);
        e->Delete();
        return false;
    }
}

bool DBHelper::GetGameSettings(int userId, GameSettings& s)
{
    if (!Connect()) return false;
    CRecordset rs(&m_db);
    try {
        CString query;
        query.Format(_T("SELECT settings FROM users WHERE user_id = %d"), userId);
        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);

        if (!rs.IsEOF()) {
            CString val;
            rs.GetFieldValue((short)0, val);

            if (val.IsEmpty() || val == _T("NULL")) {
                _tcscpy_s(s.topMenuFont.fontName, _T("Calibri")); s.topMenuFont.fontSize = 22; s.topMenuFont.fontWeight = 700; s.topMenuFont.isItalic = 0; s.topMenuFont.isUnderline = 0; s.topMenuFont.isStrikeOut = 0; s.topMenuFont.useStroke = 0; s.topMenuFont.strokeColor = 0; s.topMenuFont.strokeThickness = 1;
                _tcscpy_s(s.buttonFont.fontName, _T("Consolas")); s.buttonFont.fontSize = 16; s.buttonFont.fontWeight = 700; s.buttonFont.isItalic = 0; s.buttonFont.isUnderline = 0; s.buttonFont.isStrikeOut = 0; s.buttonFont.useStroke = 0; s.buttonFont.strokeColor = 0; s.buttonFont.strokeThickness = 1;
                _tcscpy_s(s.actionFont.fontName, _T("Arial"));    s.actionFont.fontSize = 12; s.actionFont.fontWeight = 700; s.actionFont.isItalic = 0; s.actionFont.isUnderline = 0; s.actionFont.isStrikeOut = 0; s.actionFont.useStroke = 0; s.actionFont.strokeColor = 0; s.actionFont.strokeThickness = 1;
                s.hudRectColor = RGB(13, 25, 76); s.hudBorderColor = RGB(255, 204, 51); s.hudBorderEnabled = 1;
                s.crosshairEnabled = 1; s.crosshairColor = RGB(255, 255, 255); s.crosshairBorderEnabled = 0; s.crosshairBorderColor = 0; s.crosshairBorderThickness = 1;
                s.cupColor = RGB(153, 25, 25); s.ballColor = RGB(204, 204, 204);
                s.skyboxIndex = 0;
                s.tableEnabled = 1; s.tableMatIndex = 0;
                s.carpetEnabled = 1; s.carpetMatIndex = 0;
            }
            else {
                _tcscpy_s(s.topMenuFont.fontName, JsonStr(val, _T("top_name"), _T("Calibri")));
                s.topMenuFont.fontSize = JsonInt(val, _T("top_size"), 22);
                s.topMenuFont.fontWeight = JsonInt(val, _T("top_bold"), 700);
                s.topMenuFont.isItalic = JsonInt(val, _T("top_italic"), 0);
                s.topMenuFont.isUnderline = JsonInt(val, _T("top_u"), 0);
                s.topMenuFont.isStrikeOut = JsonInt(val, _T("top_s"), 0);
                s.topMenuFont.useStroke = JsonInt(val, _T("top_stroke"), 0);
                s.topMenuFont.strokeColor = JsonULong(val, _T("top_col"), 0);
                s.topMenuFont.strokeThickness = JsonInt(val, _T("top_th"), 1);

                _tcscpy_s(s.buttonFont.fontName, JsonStr(val, _T("btn_name"), _T("Consolas")));
                s.buttonFont.fontSize = JsonInt(val, _T("btn_size"), 16);
                s.buttonFont.fontWeight = JsonInt(val, _T("btn_bold"), 700);
                s.buttonFont.isItalic = JsonInt(val, _T("btn_italic"), 0);
                s.buttonFont.isUnderline = JsonInt(val, _T("btn_u"), 0);
                s.buttonFont.isStrikeOut = JsonInt(val, _T("btn_s"), 0);
                s.buttonFont.useStroke = JsonInt(val, _T("btn_stroke"), 0);
                s.buttonFont.strokeColor = JsonULong(val, _T("btn_col"), 0);
                s.buttonFont.strokeThickness = JsonInt(val, _T("btn_th"), 1);

                _tcscpy_s(s.actionFont.fontName, JsonStr(val, _T("act_name"), _T("Arial")));
                s.actionFont.fontSize = JsonInt(val, _T("act_size"), 12);
                s.actionFont.fontWeight = JsonInt(val, _T("act_bold"), 700);
                s.actionFont.isItalic = JsonInt(val, _T("act_italic"), 0);
                s.actionFont.isUnderline = JsonInt(val, _T("act_u"), 0);
                s.actionFont.isStrikeOut = JsonInt(val, _T("act_s"), 0);
                s.actionFont.useStroke = JsonInt(val, _T("act_stroke"), 0);
                s.actionFont.strokeColor = JsonULong(val, _T("act_col"), 0);
                s.actionFont.strokeThickness = JsonInt(val, _T("act_th"), 1);

                s.hudRectColor = JsonULong(val, _T("hud_bg"), RGB(13, 25, 76));
                s.hudBorderColor = JsonULong(val, _T("hud_border"), RGB(255, 204, 51));
                s.hudBorderEnabled = JsonInt(val, _T("hud_b_en"), 1);

                s.crosshairEnabled = JsonInt(val, _T("cross_en"), 1);
                s.crosshairColor = JsonULong(val, _T("cross_col"), RGB(255, 255, 255));
                s.crosshairBorderEnabled = JsonInt(val, _T("cross_b_en"), 0);
                s.crosshairBorderColor = JsonULong(val, _T("cross_b_col"), 0);
                s.crosshairBorderThickness = JsonInt(val, _T("cross_th"), 1);

                s.cupColor = JsonULong(val, _T("cup_col"), RGB(153, 25, 25));
                s.ballColor = JsonULong(val, _T("ball_col"), RGB(204, 204, 204));

                s.skyboxIndex = JsonInt(val, _T("skybox"), 0);
                s.tableEnabled = JsonInt(val, _T("tab_en"), 1);
                s.tableMatIndex = JsonInt(val, _T("tab_mat"), 0);
                s.carpetEnabled = JsonInt(val, _T("carp_en"), 1);
                s.carpetMatIndex = JsonInt(val, _T("carp_mat"), 0);
            }
            return true;
        }
    }
    catch (CDBException* e) {
		AfxMessageBox(_T("Помилка завантаження налаштувань: ") + e->m_strError);
        e->Delete();
    }
    return false;
}