// MagicCupGameView.h: интерфейс класса CMagicCupGameView
//

#pragma once
#include <GL/glew.h>
#include <GL/GLU.h>
#include "DBHelper.h"
#include "Shader.h"
#include "ShadowMap.h"
#include "Mat4.h"

struct SkyboxSettings {
    CString folderName;
    CString extension;
    CString name;
    GLfloat lightPos[4];
    GLfloat ambient[4];
    GLfloat diffuse[4];
};

struct Material {
    CString fileName;
    CString extension;
    CString name;
};

class CMagicCupGameView : public CView
{
protected: // создать только из сериализации
    CMagicCupGameView();
    DECLARE_DYNCREATE(CMagicCupGameView)

    // Атрибуты
public:
    CMagicCupGameDoc* GetDocument() const;

    // Операции
public:

    // Переопределение
public:
    virtual void OnDraw(CDC* pDC);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

    // Реализация
public:
    virtual ~CMagicCupGameView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    HGLRC m_hRC;
    CDC* m_pDC;

    enum GameState {
        GAME_WAITING,
        GAME_LIFTING_CUPS,
        GAME_SHOWING_BALL,
        GAME_LOWERING_CUPS,
        GAME_SHUFFLING,
        GAME_GUESSING,
        GAME_RESULT,
        GAME_NONE
    };

    GameState m_gameState;
    DWORD m_gameStartTime;
    DWORD m_stateStartTime;

    int m_ballPosition;
    int m_selectedCup;
    int m_hoveredCup = -1;
    bool m_gameWon;

    float m_cupPositions[3][3];
    float m_cupTargets[3][3];
    float m_cupHeights[3];
    float m_cupTargetHeights[3];
    float m_startPositions[3][3];
    int m_swapIndices[2];

    bool m_isAnimating;
    int m_shuffleStep;
    DWORD m_stepStartTime;
    float m_animationProgress;
    CPoint m_lastMousePos;

    GLuint cubeMapTexture;
    GLuint tableTexture;
    GLuint carpetTexture;

    int m_windowWidth = 800;
    int m_windowHeight = 600;

    bool m_keyW;
    bool m_keyA;
    bool m_keyS;
    bool m_keyD;
    bool m_keySpace;
    bool m_keyCtrl;
    bool m_keyShift;
    bool m_isFlashlightOn;
    bool m_rmbDown;
    const float DEF_CAM_X = 0.0f;
    const float DEF_CAM_Y = 8.0f;
    const float DEF_CAM_Z = 15.0f;
    const float DEF_CAM_YAW = -90.0f;
    const float DEF_CAM_PITCH = -30.0f;
    float m_camX, m_camY, m_camZ;
    float m_camYaw, m_camPitch;
    float m_velX, m_velY, m_velZ;

    DWORD m_lastDBUpdate;

    LARGE_INTEGER m_qpFrequency;
    LARGE_INTEGER m_qpLastTime;

    std::vector<SkyboxSettings> m_skyboxPresets;
    int m_currentSkyboxIndex = -1;

    std::vector<Material> m_tableMats;
	int m_currentTableMatIndex = -1;

    std::vector<Material> m_carpetMats;
	int m_currentCarpetMatIndex = -1;

    GameSettings m_gameSettings;

    GLfloat m_actLightPos[4];
    GLfloat m_actAmbient[4];
    GLfloat m_actDiffuse[4];

    Shader m_mainShader;
    Shader m_depthShader;
    ShadowMap m_shadowMap;
    bool m_shadersReady = false;
    Mat4 m_lightViewProj;

    BOOL SetupPixelFormat();
    void InitOpenGL();
    void InitShaders();
    void CleanupOpenGL();
    void RenderShadowPass(const float lightPosWorld[3]);
    void DrawScene();
    void DrawSky();
    void DrawGameObjects(bool bShadow = false);
    GLuint LoadTexture(const char* filename);
    GLuint LoadCubeMap(const char* faces[6]);
    void DrawRoundedCorner(float cx, float cz, float y, float radius, int segments, float startAngle, float totalW, float totalD);
    void DrawTableTop(float width, float depth, float height, float radius, int segments, GLuint textureID, float yTop);
    void DrawTableLegs(float yTop, float height);
    void DrawTable();
    void UpdateAnimation();
    void StartNewGame();
    void SetupShuffleStep();
    void OnCupClick(int cupIndex);
    int GetCupIndexFromScreenPos(CPoint point);
    int GetCupIndexByPosition(int positionIndex);
    void SetupLighting();
    void DrawCupWithDetails(float x, float y, float z, float height, bool highlight = false, bool bShadow = false);
    void DrawGradientBall(float x, float y, float z, float radius, bool bShadow = false);

    void DrawStringTexture(int x, int y, const char* text, const FontConfig& font, COLORREF color);
    void HUDPrint(int x, int y, const char* text, const FontConfig& font, COLORREF color);
    int TextWidth(const char* text, const FontConfig& font);

    void DrawHUD();
    void UpdateCameraMovement();
    void UpdateDifficultyFromDB();
    void LoadGameSettings();
    void SelectSkybox(int index);
    void SelectTable(int index);
	void SelectCarpet(int index);
    void InitMaterialPresets();

    // Созданные функции схемы сообщений
protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);

public:
    DifficultyLevel m_selectedLevel;
    afx_msg void OnSettings();
    afx_msg void OnAdmin();
};

#ifndef _DEBUG  // версия отладки в MagicCupGameView.cpp
inline CMagicCupGameDoc* CMagicCupGameView::GetDocument() const
{
    return reinterpret_cast<CMagicCupGameDoc*>(m_pDocument);
}
#endif