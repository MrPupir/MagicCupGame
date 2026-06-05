#include "pch.h"
#include "DBHelper.h"
#include <functional>
#include <string>
#include <vector>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

static const int    PBKDF2_ITER     = 100000;
static const ULONG  PBKDF2_SALT_LEN = 16;
static const ULONG  PBKDF2_HASH_LEN = 32;

static void to_hex(const unsigned char* bytes, size_t len, CString& out) {
    out.Empty();
    for (size_t i = 0; i < len; i++) {
        CString b; b.Format(_T("%02x"), bytes[i]);
        out += b;
    }
}

static bool from_hex(const CString& hex, std::vector<unsigned char>& out) {
    if (hex.GetLength() % 2 != 0) return false;
    out.resize(hex.GetLength() / 2);
    for (int i = 0; i < hex.GetLength(); i += 2) {
        unsigned int b = 0;
        if (_stscanf_s(hex.Mid(i, 2), _T("%x"), &b) != 1) return false;
        out[i / 2] = (unsigned char)b;
    }
    return true;
}

static void random_salt(unsigned char* salt, ULONG len) {
    NTSTATUS s = BCryptGenRandom(NULL, salt, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (s != STATUS_SUCCESS) {
        srand((unsigned)(GetTickCount() ^ (unsigned)(uintptr_t)salt));
        for (ULONG i = 0; i < len; i++) salt[i] = (unsigned char)(rand() & 0xFF);
    }
}

static bool pbkdf2_sha256(const std::string& pw,
                          const unsigned char* salt, ULONG saltLen,
                          ULONG iter,
                          unsigned char* out, ULONG outLen) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    NTSTATUS s = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM,
                                             NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (s != STATUS_SUCCESS) return false;
    s = BCryptDeriveKeyPBKDF2(hAlg,
                              (PUCHAR)pw.data(), (ULONG)pw.size(),
                              (PUCHAR)salt, saltLen,
                              iter, out, outLen, 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return s == STATUS_SUCCESS;
}

static CString HashPassword(const CString& password)
{
    unsigned char salt[PBKDF2_SALT_LEN];
    random_salt(salt, PBKDF2_SALT_LEN);

    CT2A utf8(password, CP_UTF8);
    std::string pwStr(utf8.m_psz);

    unsigned char hash[PBKDF2_HASH_LEN];
    if (!pbkdf2_sha256(pwStr, salt, PBKDF2_SALT_LEN, (ULONG)PBKDF2_ITER, hash, PBKDF2_HASH_LEN)) {
        return _T("");
    }

    CString saltHex, hashHex;
    to_hex(salt, PBKDF2_SALT_LEN, saltHex);
    to_hex(hash, PBKDF2_HASH_LEN, hashHex);

    CString out;
    out.Format(_T("pbkdf2$%d$%s$%s"), PBKDF2_ITER, saltHex.GetString(), hashHex.GetString());
    return out;
}

static bool VerifyPassword(const CString& password, const CString& stored)
{
    std::vector<CString> parts;
    CString cur;
    for (int i = 0; i < stored.GetLength(); i++) {
        if (stored[i] == _T('$')) { parts.push_back(cur); cur.Empty(); }
        else { cur += stored[i]; }
    }
    parts.push_back(cur);

    if (parts.size() != 4) return false;
    if (parts[0] != _T("pbkdf2")) return false;

    int iter = _ttoi(parts[1]);
    if (iter < 1) return false;

    std::vector<unsigned char> salt, expectedHash;
    if (!from_hex(parts[2], salt)) return false;
    if (!from_hex(parts[3], expectedHash)) return false;
    if (expectedHash.empty()) return false;

    CT2A utf8(password, CP_UTF8);
    std::string pwStr(utf8.m_psz);

    std::vector<unsigned char> computedHash(expectedHash.size());
    if (!pbkdf2_sha256(pwStr, salt.data(), (ULONG)salt.size(),
                       (ULONG)iter,
                       computedHash.data(), (ULONG)computedHash.size())) {
        return false;
    }

    unsigned char diff = 0;
    for (size_t i = 0; i < expectedHash.size(); i++) {
        diff |= (unsigned char)(computedHash[i] ^ expectedHash[i]);
    }
    return diff == 0;
}

bool DBHelper::Connect()
{
    if (m_db.IsOpen()) return true;
    try {
#ifdef _WIN64
        CString sConnectionString = _T("Driver={MySQL ODBC 9.7 Unicode Driver};Server=127.0.0.1;Port=3306;Database=mcg;User=root;Password=root;Option=3;");
#else
        CString sConnectionString = _T("Driver={MySQL ODBC 8.0 Unicode Driver};Server=127.0.0.1;Port=3306;Database=mcg;User=root;Password=root;Option=3;");
#endif
        m_db.OpenEx(sConnectionString, CDatabase::noOdbcDialog);
        return true;
    }
    catch (CDBException* e) {
        AfxMessageBox(_T("Помилка з'єднання з БД: ") + e->m_strError);
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
        CString safeName = username; safeName.Replace(_T("'"), _T("''"));
        CString query;
        query.Format(_T("SELECT user_id, password_hash FROM users WHERE username='%s'"), safeName);
        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        if (!rs.IsEOF()) {
            CString idVal, storedHash;
            rs.GetFieldValue((short)0, idVal);
            rs.GetFieldValue((short)1, storedHash);
            if (VerifyPassword(password, storedHash)) {
                return _ttoi(idVal);
            }
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
    CString safeName = username; safeName.Replace(_T("'"), _T("''"));
    CString checkQuery;
    checkQuery.Format(_T("SELECT user_id FROM users WHERE username='%s'"), safeName);
    rs.Open(CRecordset::forwardOnly, checkQuery, CRecordset::readOnly);
    if (!rs.IsEOF()) {
        AfxMessageBox(_T("Користувач з таким ім'ям вже існує!"));
        return -1;
    }
    rs.Close();
    try {
        CString passwordHash = HashPassword(password);
        CString insertQuery;
        insertQuery.Format(_T("INSERT INTO users (username, password_hash, role_id) VALUES ('%s', '%s', 2)"), safeName, passwordHash);
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
		AfxMessageBox(_T("Помилка завантаження налаштувань: ") + e->m_strError);
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
		AfxMessageBox(_T("Помилка збереження налаштувань: ") + e->m_strError);
        e->Delete();
    }
    return false;
}

std::vector<CString> DBHelper::GetUniqueDifficultyNames()
{
    std::vector<CString> names;
    if (!Connect()) return names;
    CRecordset rs(&m_db);
    try {
        rs.Open(CRecordset::forwardOnly, _T("SELECT level_name FROM difficulty_levels GROUP BY level_name ORDER BY MIN(sort_order)"), CRecordset::readOnly);
        while (!rs.IsEOF()) {
            CString val;
            rs.GetFieldValue((short)0, val);
            if (!val.IsEmpty()) names.push_back(val);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return names;
}

std::vector<ChartEntry> DBHelper::GetMonthlyStats(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "total_games" && metric != "total_wins" && metric != "top_cup" &&
            metric != "top_ball" && metric != "total_registrations") return data;

        CString query;
        query.Format(_T("SELECT stat_date, %s FROM v_monthly_detailed_stats ORDER BY STR_TO_DATE(stat_date, '%%d.%%m.%%Y') ASC LIMIT 30"), metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString valStr;
            rs.GetFieldValue((short)0, entry.label);
            rs.GetFieldValue((short)1, valStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetWeeklyStats(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "total_games" && metric != "total_wins" && metric != "top_cup" &&
            metric != "top_ball" && metric != "total_registrations") return data;

        CString query;
        query.Format(_T("SELECT stat_date, %s FROM v_weekly_detailed_stats ORDER BY STR_TO_DATE(stat_date, '%%d.%%m.%%Y') ASC"), metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString valStr;
            rs.GetFieldValue((short)0, entry.label);
            rs.GetFieldValue((short)1, valStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetDifficultyPerformance(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "total_games" && metric != "total_wins" && metric != "win_rate_percent") return data;

        CString query;
        query.Format(_T("SELECT level_name, %s FROM v_stats_difficulty_performance"), metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString valStr;
            rs.GetFieldValue((short)0, entry.label);
            rs.GetFieldValue((short)1, valStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetHourlyActivity(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "total_games" && metric != "total_wins") return data;

        CString query;
        query.Format(_T("SELECT hour_of_day, %s FROM v_stats_hourly_activity ORDER BY hour_of_day"), metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString hourStr, valStr;
            rs.GetFieldValue((short)0, hourStr);
            rs.GetFieldValue((short)1, valStr);

            entry.label.Format(_T("%s:00"), hourStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetLeaderboardWeighted(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "total_games" && metric != "total_wins" && metric != "rank_score") return data;

        CString query;
        query.Format(_T("SELECT username, %s FROM v_stats_leaderboard_weighted ORDER BY %s DESC LIMIT 10"), metric, metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString valStr;
            rs.GetFieldValue((short)0, entry.label);
            rs.GetFieldValue((short)1, valStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetPositionBias(CString type)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        CString query;
        query.Format(_T("SELECT position_index, occurrences FROM v_stats_position_bias WHERE type COLLATE utf8mb4_unicode_ci LIKE '%s' ORDER BY position_index"), type);

        rs.Open(CRecordset::snapshot, query, CRecordset::readOnly);

        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString posStr, valStr;

            rs.GetFieldValue((short)0, posStr);
            rs.GetFieldValue((short)1, valStr);

            int pos = _ttoi(posStr);
            if (pos == 1) entry.label = _T("Ліва (1)");
            else if (pos == 2) entry.label = _T("Центр (2)");
            else if (pos == 3) entry.label = _T("Права (3)");
            else entry.label = posStr;

            entry.value = _ttof(valStr);

            if (type == _T("ball")) entry.color = RGB(100, 200, 100);
            else entry.color = RGB(100, 100, 255);

            data.push_back(entry);
            rs.MoveNext();
        }

        double total = 0.0;
        for (const auto& e : data) total += e.value;
        if (total > 0.0) {
            for (auto& e : data) e.value = e.value / total * 100.0;
        }
    }
    catch (CDBException* e) {
        e->Delete();
    }
    return data;
}

std::vector<ChartEntry> DBHelper::GetSpeedVsWinrate(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "shuffle_count" && metric != "total_games" && metric != "win_rate") return data;

        CString query;
        query.Format(_T("SELECT animation_speed_ms, %s FROM v_stats_speed_vs_winrate ORDER BY animation_speed_ms DESC"), metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString speedStr, valStr;
            rs.GetFieldValue((short)0, speedStr);
            rs.GetFieldValue((short)1, valStr);

            entry.label.Format(_T("%s мс"), speedStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetTopByDifficulty(CString levelName, CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "games_played" && metric != "wins" && metric != "win_rate") return data;

        CString query;
        CString safeName = levelName;
        safeName.Replace(_T("'"), _T("''"));

        query.Format(_T("SELECT username, %s FROM v_stats_top_by_difficulty WHERE level_name = '%s' ORDER BY %s DESC LIMIT 10"), metric, safeName, metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString valStr;
            rs.GetFieldValue((short)0, entry.label);
            rs.GetFieldValue((short)1, valStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetTopPlayers(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "total_games" && metric != "total_wins" && metric != "win_rate_percent") return data;

        CString query;
        query.Format(_T("SELECT username, %s FROM v_stats_top_players ORDER BY %s DESC LIMIT 10"), metric, metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString valStr;
            rs.GetFieldValue((short)0, entry.label);
            rs.GetFieldValue((short)1, valStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetUserRetention(CString metric)
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        if (metric != "days_active" && metric != "total_sessions") return data;

        CString query;
        query.Format(_T("SELECT username, %s FROM v_stats_user_retention ORDER BY %s DESC LIMIT 15"), metric, metric);

        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString valStr;
            rs.GetFieldValue((short)0, entry.label);
            rs.GetFieldValue((short)1, valStr);
            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetSkyboxPopularity()
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        rs.Open(CRecordset::forwardOnly, _T("SELECT selected_skybox, users_count FROM v_stats_skybox_popularity ORDER BY users_count DESC"), CRecordset::readOnly);
        const TCHAR* skyboxNames[] = { _T("Ставок"), _T("Футбольне поле"), _T("Нічне поле"), _T("Галявина"), _T("Сорселе 1"), _T("Сорселе 2"), _T("Сорселе 3") };

        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString idStr, valStr;
            rs.GetFieldValue((short)0, idStr);
            rs.GetFieldValue((short)1, valStr);

            int idx = _ttoi(idStr);
            if (idx >= 0 && idx < 7) entry.label = skyboxNames[idx];
            else entry.label.Format(_T("Skybox %d"), idx);

            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetTableMatPopularity()
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        rs.Open(CRecordset::forwardOnly, _T("SELECT selected_tab_mat, users_count FROM v_stats_tab_mat_popularity ORDER BY users_count DESC"), CRecordset::readOnly);
        const TCHAR* matNames[] = { _T("Какао"), _T("Бежевий"), _T("Шоколад"), _T("Світл-кор."), _T("Олива"), _T("Оранж") };

        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString idStr, valStr;
            rs.GetFieldValue((short)0, idStr);
            rs.GetFieldValue((short)1, valStr);

            int idx = _ttoi(idStr);
            if (idx >= 0 && idx < 6) entry.label = matNames[idx];
            else entry.label.Format(_T("Material %d"), idx);

            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<ChartEntry> DBHelper::GetCarpetMatPopularity()
{
    std::vector<ChartEntry> data;
    if (!Connect()) return data;
    CRecordset rs(&m_db);
    try {
        rs.Open(CRecordset::forwardOnly, _T("SELECT selected_carp_mat, users_count FROM v_stats_carp_mat_popularity ORDER BY users_count DESC"), CRecordset::readOnly);
        const TCHAR* matNames[] = { _T("Червоний"), _T("Синій"), _T("Темно-черв."), _T("Зелений"), _T("Світл-син."), _T("Райдуга") };

        while (!rs.IsEOF()) {
            ChartEntry entry;
            CString idStr, valStr;
            rs.GetFieldValue((short)0, idStr);
            rs.GetFieldValue((short)1, valStr);

            int idx = _ttoi(idStr);
            if (idx >= 0 && idx < 6) entry.label = matNames[idx];
            else entry.label.Format(_T("Carpet %d"), idx);

            entry.value = _ttof(valStr);
            data.push_back(entry);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return data;
}

std::vector<PlayerListEntry> DBHelper::GetAllPlayers()
{
    std::vector<PlayerListEntry> players;
    if (!Connect()) return players;
    CRecordset rs(&m_db);
    try {
        rs.Open(CRecordset::forwardOnly,
            _T("SELECT user_id, username FROM users ORDER BY username ASC"),
            CRecordset::readOnly);
        while (!rs.IsEOF()) {
            PlayerListEntry p;
            CString val;
            rs.GetFieldValue((short)0, val); p.userId = _ttoi(val);
            rs.GetFieldValue((short)1, p.username);
            players.push_back(p);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return players;
}

bool DBHelper::GetPlayerStats(int userId, PlayerStats& out)
{
    if (!Connect()) return false;
    CRecordset rs(&m_db);
    try {
        m_lastError.Empty();
        CString query;
#ifdef _WIN64
        query.Format(_T("{CALL sp_get_player_stats(%d)}"), userId);
#else
        query.Format(
            _T("SELECT u.user_id, u.username, r.role_name, u.created_at, ")
            _T("u.selected_difficulty, dl_cur.level_name, ")
            _T("COUNT(gs.session_id), ")
            _T("COALESCE(SUM(CASE WHEN gs.is_win = 1 THEN 1 ELSE 0 END), 0), ")
            _T("COALESCE(SUM(CASE WHEN gs.is_win = 0 THEN 1 ELSE 0 END), 0), ")
            _T("COALESCE(ROUND(SUM(CASE WHEN gs.is_win = 1 THEN 1 ELSE 0 END) / NULLIF(COUNT(gs.session_id), 0) * 100, 1), 0), ")
            _T("MIN(gs.played_at), MAX(gs.played_at), ")
            _T("SUM(CASE WHEN gs.played_at >= NOW() - INTERVAL 1 DAY THEN 1 ELSE 0 END), ")
            _T("SUM(CASE WHEN gs.played_at >= NOW() - INTERVAL 7 DAY THEN 1 ELSE 0 END), ")
            _T("SUM(CASE WHEN gs.played_at >= NOW() - INTERVAL 30 DAY THEN 1 ELSE 0 END), ")
            _T("(SELECT dl.level_name FROM game_sessions gs2 JOIN difficulty_levels dl ON gs2.difficulty_id = dl.difficulty_id WHERE gs2.user_id = u.user_id GROUP BY dl.difficulty_id, dl.level_name ORDER BY COUNT(*) DESC LIMIT 1), ")
            _T("JSON_UNQUOTE(JSON_EXTRACT(u.settings, '$.skybox')), ")
            _T("JSON_UNQUOTE(JSON_EXTRACT(u.settings, '$.tab_mat')), ")
            _T("JSON_UNQUOTE(JSON_EXTRACT(u.settings, '$.carp_mat')) ")
            _T("FROM users u ")
            _T("JOIN roles r ON r.role_id = u.role_id ")
            _T("LEFT JOIN difficulty_levels dl_cur ON u.selected_difficulty = dl_cur.difficulty_id ")
            _T("LEFT JOIN game_sessions gs ON gs.user_id = u.user_id ")
            _T("WHERE u.user_id = %d ")
            _T("GROUP BY u.user_id, u.username, r.role_name, u.created_at, u.selected_difficulty, dl_cur.level_name, u.settings"),
            userId);
#endif
        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        if (rs.IsEOF()) { m_lastError = _T("Користувача не знайдено"); return false; }

        CString val;
        rs.GetFieldValue((short)0, val);  out.userId = _ttoi(val);
        rs.GetFieldValue((short)1, out.username);
        rs.GetFieldValue((short)2, out.roleName);
        rs.GetFieldValue((short)3, out.signedUpAt);
        rs.GetFieldValue((short)4, val);
        rs.GetFieldValue((short)5, out.currentDifficultyName);
        rs.GetFieldValue((short)6, val);  out.totalGames = _ttoi(val);
        rs.GetFieldValue((short)7, val);  out.totalWins = _ttoi(val);
        rs.GetFieldValue((short)8, val);  out.totalLosses = _ttoi(val);
        rs.GetFieldValue((short)9, val);  out.winRatePercent = _ttof(val);
        rs.GetFieldValue((short)10, out.firstPlayed);
        rs.GetFieldValue((short)11, out.lastPlayed);
        rs.GetFieldValue((short)12, val); out.gamesLast24h = _ttoi(val);
        rs.GetFieldValue((short)13, val); out.gamesLast7d = _ttoi(val);
        rs.GetFieldValue((short)14, val); out.gamesLast30d = _ttoi(val);
        rs.GetFieldValue((short)15, out.favouriteDifficulty);
        rs.GetFieldValue((short)16, out.favouriteSkybox);
        rs.GetFieldValue((short)17, out.favouriteTabMat);
        rs.GetFieldValue((short)18, out.favouriteCarpMat);
        return true;
    }
    catch (CDBException* e) {
        m_lastError = e->m_strError;
        e->Delete();
        return false;
    }
}

std::vector<PlayerBreakdownRow> DBHelper::GetPlayerBreakdown(int userId)
{
    std::vector<PlayerBreakdownRow> rows;
    if (!Connect()) return rows;
    CRecordset rs(&m_db);
    try {
        CString query;
#ifdef _WIN64
        query.Format(_T("{CALL sp_get_player_breakdown(%d)}"), userId);
#else
        query.Format(
            _T("SELECT dl.difficulty_id, dl.level_name, dl.sort_order, ")
            _T("COUNT(gs.session_id), ")
            _T("COALESCE(SUM(CASE WHEN gs.is_win = 1 THEN 1 ELSE 0 END), 0), ")
            _T("COALESCE(ROUND(SUM(CASE WHEN gs.is_win = 1 THEN 1 ELSE 0 END) / NULLIF(COUNT(gs.session_id), 0) * 100, 1), 0) ")
            _T("FROM difficulty_levels dl ")
            _T("LEFT JOIN game_sessions gs ON gs.difficulty_id = dl.difficulty_id AND gs.user_id = %d ")
            _T("GROUP BY dl.difficulty_id, dl.level_name, dl.sort_order ")
            _T("ORDER BY dl.sort_order"),
            userId);
#endif
        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            PlayerBreakdownRow r;
            CString val;
            rs.GetFieldValue((short)0, val); r.difficultyId = _ttoi(val);
            rs.GetFieldValue((short)1, r.levelName);
            rs.GetFieldValue((short)2, val); r.sortOrder = _ttoi(val);
            rs.GetFieldValue((short)3, val); r.totalGames = _ttoi(val);
            rs.GetFieldValue((short)4, val); r.totalWins = _ttoi(val);
            rs.GetFieldValue((short)5, val); r.winRatePercent = _ttof(val);
            rows.push_back(r);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return rows;
}

std::vector<PlayerSessionRow> DBHelper::GetPlayerRecentSessions(int userId, int limit)
{
    std::vector<PlayerSessionRow> rows;
    if (!Connect()) return rows;
    CRecordset rs(&m_db);
    try {
        CString query;
        query.Format(
            _T("SELECT gs.session_id, gs.played_at, gs.ball_position, gs.selected_cup, gs.is_win, dl.level_name ")
            _T("FROM game_sessions gs JOIN difficulty_levels dl ON gs.difficulty_id = dl.difficulty_id ")
            _T("WHERE gs.user_id = %d ORDER BY gs.played_at DESC LIMIT %d"),
            userId, limit);
        rs.Open(CRecordset::forwardOnly, query, CRecordset::readOnly);
        while (!rs.IsEOF()) {
            PlayerSessionRow r;
            CString val;
            rs.GetFieldValue((short)0, val); r.sessionId = _ttoi64(val);
            rs.GetFieldValue((short)1, r.playedAt);
            rs.GetFieldValue((short)2, val); r.ballPosition = _ttoi(val);
            rs.GetFieldValue((short)3, val); r.selectedCup = _ttoi(val);
            rs.GetFieldValue((short)4, val); r.isWin = (_ttoi(val) == 1);
            rs.GetFieldValue((short)5, r.levelName);
            rows.push_back(r);
            rs.MoveNext();
        }
    }
    catch (CDBException* e) { e->Delete(); }
    return rows;
}