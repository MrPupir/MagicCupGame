// MagicCupGameView.h: интерфейс класса CMagicCupGameView
//

#pragma once
#include "GL/GL.h"
#include "GL/GLU.h"
#include "GL/GLEXT.h"
#include "DBHelper.h"

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
    virtual void OnDraw(CDC* pDC);  // переопределено для отрисовки этого представления
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

    float m_cameraAngle;
    float m_cameraDistance;
    float m_cameraHeight;
    bool m_mousePressed;
    CPoint m_lastMousePos;
    GLuint cubeMapTexture;
    GLuint tableTexture;

    GLuint m_fontBase = 0;
    int m_windowWidth = 800;
    int m_windowHeight = 600;

    BOOL SetupPixelFormat();
    void InitOpenGL();
    void CleanupOpenGL();
    void DrawScene();
    void DrawSky();
    GLuint LoadTexturePNG(const char* filename);
    GLuint LoadCubeMap(const char* faces[6]);
    void DrawRoundedCorner(float cx, float cz, float y, float radius, int segments, float startAngle);
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
    void DrawCupWithDetails(float x, float y, float z, float height, bool highlight = false);
    void DrawGradientBall(float x, float y, float z, float radius);
    void HUDPrint(int x, int y, const char* text);
    int TextWidth(const char* text);
    void DrawHUD();

// Созданные функции схемы сообщений
protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

public:
    DifficultyLevel m_selectedLevel;
    afx_msg void OnSettings();
    afx_msg void OnAdmin();
};

#ifndef _DEBUG  // версия отладки в MagicCupGameView.cpp
inline CMagicCupGameDoc* CMagicCupGameView::GetDocument() const
    { return reinterpret_cast<CMagicCupGameDoc*>(m_pDocument); }
#endif