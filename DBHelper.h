#pragma once
#include <afxdb.h>
#include <vector>

struct DifficultyLevel {
    int id;
    CString name;
    int shuffleCount;
    int animationSpeed;
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
private:
    CDatabase m_db;
    DBHelper() {};
};