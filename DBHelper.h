#pragma once
#include <afxdb.h>
#include <vector>

struct DifficultyLevel {
    int id;
    CString name;
    int shuffleCount;
    int animationSpeed;
};

struct FontConfig {
    TCHAR fontName[32];
    int fontSize;
    int fontWeight;
    int isItalic;
    int isUnderline;
    int isStrikeOut;
    int useStroke;
    unsigned long strokeColor;
    int strokeThickness;
};

struct GameSettings {
    FontConfig topMenuFont;
    FontConfig buttonFont;
    FontConfig actionFont;

    unsigned long hudRectColor;
    unsigned long hudBorderColor;
    int hudBorderEnabled;

    int crosshairEnabled;
    unsigned long crosshairColor;
    int crosshairBorderEnabled;
    unsigned long crosshairBorderColor;
    int crosshairBorderThickness;

    unsigned long cupColor;
    unsigned long ballColor;

    int skyboxIndex;
    int tableEnabled;
    int tableMatIndex;
    int carpetEnabled;
    int carpetMatIndex;
};

class DBHelper
{
public:
    static DBHelper& GetInstance() {
        static DBHelper instance;
        return instance;
    }

    bool Connect();
    void Disconnect();

    int TryLogin(CString username, CString password);
    int RegisterUser(CString username, CString password);
    std::vector<DifficultyLevel> GetDifficultyLevels();
    bool SaveUserDifficulty(int userId, int difficultyId);
    int GetUserDifficulty(int userId);
    bool AddGameSession(int userId, int difficultyId, int ballPos, int selectedCup, bool isWin);
    bool IsAdminUser(int userId);
    bool AddDifficultyLevel(CString name, int count, int speed);
    bool UpdateDifficultyLevel(int id, CString name, int count, int speed);
    bool DeleteDifficultyLevel(int id);
    bool SwapDifficultyOrder(int id1, int id2);
    bool SaveUserSkybox(int userId, int skyboxIndex);
    int GetUserSkybox(int userId);
    bool SaveGameSettings(int userId, const GameSettings& settings);
    bool GetGameSettings(int userId, GameSettings& settings);

private:
    CDatabase m_db;
    DBHelper() {};

    CString JsonVal(const CString& json, const CString& key);
    int JsonInt(const CString& json, const CString& key, int defVal);
    unsigned long JsonULong(const CString& json, const CString& key, unsigned long defVal);
    CString JsonStr(const CString& json, const CString& key, const CString& defVal);
};