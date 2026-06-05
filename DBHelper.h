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

struct ChartEntry {
    CString label;
    double value;
    COLORREF color;

    ChartEntry() : label(_T("")), value(0.0), color(0xFFFFFFFF) {}
    ChartEntry(CString l, double v, COLORREF c = 0xFFFFFFFF) : label(l), value(v), color(c) {}
};

struct PlayerListEntry {
    int userId;
    CString username;
};

struct PlayerStats {
    int userId;
    CString username;
    CString roleName;
    CString signedUpAt;
    CString currentDifficultyName;
    int totalGames;
    int totalWins;
    int totalLosses;
    double winRatePercent;
    CString firstPlayed;
    CString lastPlayed;
    int gamesLast24h;
    int gamesLast7d;
    int gamesLast30d;
    CString favouriteDifficulty;
    CString favouriteSkybox;
    CString favouriteTabMat;
    CString favouriteCarpMat;
};

struct PlayerBreakdownRow {
    int difficultyId;
    CString levelName;
    int sortOrder;
    int totalGames;
    int totalWins;
    double winRatePercent;
};

struct PlayerSessionRow {
    long long sessionId;
    CString playedAt;
    int ballPosition;
    int selectedCup;
    bool isWin;
    CString levelName;
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

    std::vector<CString> GetUniqueDifficultyNames();
    std::vector<ChartEntry> GetMonthlyStats(CString metric);
    std::vector<ChartEntry> GetWeeklyStats(CString metric);
    std::vector<ChartEntry> GetDifficultyPerformance(CString metric);
    std::vector<ChartEntry> GetHourlyActivity(CString metric);
    std::vector<ChartEntry> GetLeaderboardWeighted(CString metric);
    std::vector<ChartEntry> GetPositionBias(CString type);
    std::vector<ChartEntry> GetSpeedVsWinrate(CString metric);
    std::vector<ChartEntry> GetTopByDifficulty(CString levelName, CString metric);
    std::vector<ChartEntry> GetTopPlayers(CString metric);
    std::vector<ChartEntry> GetUserRetention(CString metric);
    std::vector<ChartEntry> GetSkyboxPopularity();
    std::vector<ChartEntry> GetTableMatPopularity();
    std::vector<ChartEntry> GetCarpetMatPopularity();

    std::vector<PlayerListEntry> GetAllPlayers();
    bool GetPlayerStats(int userId, PlayerStats& outStats);
    std::vector<PlayerBreakdownRow> GetPlayerBreakdown(int userId);
    std::vector<PlayerSessionRow> GetPlayerRecentSessions(int userId, int limit = 50);

    CString GetLastDbError() const { return m_lastError; }

private:
    CDatabase m_db;
    CString m_lastError;
    DBHelper() {};

    CString JsonVal(const CString& json, const CString& key);
    int JsonInt(const CString& json, const CString& key, int defVal);
    unsigned long JsonULong(const CString& json, const CString& key, unsigned long defVal);
    CString JsonStr(const CString& json, const CString& key, const CString& defVal);
};