// MagicCupGameView.cpp: реализация класса CMagicCupGameView
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS можно определить в обработчиках фильтров просмотра реализации проекта ATL, эскизов
// и поиска; позволяет совместно использовать код документа в данным проекте.
#ifndef SHARED_HANDLERS
#include "MagicCupGame.h"
#endif

#include "MagicCupGameDoc.h"
#include "MagicCupGameView.h"

#include <time.h>
#include <math.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define M_PI 3.14159265358979323846
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "opengl32.lib")

#pragma setlocale("RUSSIAN")

#include "SettingsDlg.h"

// CMagicCupGameView

IMPLEMENT_DYNCREATE(CMagicCupGameView, CView)

BEGIN_MESSAGE_MAP(CMagicCupGameView, CView)
    // Стандартные команды печати
    ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_MOUSEWHEEL()
    ON_COMMAND(ID_SETTINGS, &CMagicCupGameView::OnSettings)
END_MESSAGE_MAP()

// Создание или уничтожение CMagicCupGameView

CMagicCupGameView::CMagicCupGameView() : m_hRC(NULL), m_pDC(NULL)
{
    m_gameState = GAME_NONE;
    m_ballPosition = 0;
    m_selectedCup = -1;
    m_gameWon = false;
    m_isAnimating = false;
    m_shuffleStep = 0;
    m_animationProgress = 0.0f;

    m_cupPositions[0][0] = -3.0f; m_cupPositions[0][1] = 0.5f; m_cupPositions[0][2] = 0.0f;
    m_cupPositions[1][0] = 0.0f;  m_cupPositions[1][1] = 0.5f; m_cupPositions[1][2] = 0.0f;
    m_cupPositions[2][0] = 3.0f;  m_cupPositions[2][1] = 0.5f; m_cupPositions[2][2] = 0.0f;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m_cupTargets[i][j] = m_cupPositions[i][j];
        }
        m_cupHeights[i] = 0.0f;
        m_cupTargetHeights[i] = 0.0f;
    }

    m_cameraAngle = 90.0f;
    m_cameraDistance = 15.0f;
    m_cameraHeight = 8.0f;
    m_mousePressed = false;

    m_selectedLevel = DifficultyLevel{ -1, L"Стандарт", 4, 500 };

    std::vector<DifficultyLevel> levels = DBHelper::GetInstance().GetDifficultyLevels();

    if (levels.empty()) {
        m_selectedLevel = { -1, _T("Стандарт"), 3, 800 };
    }
    else {
        CMagicCupGameApp* pApp = (CMagicCupGameApp*)AfxGetApp();
        int userId = pApp->m_nCurrentUserID;

        int savedDiffId = 1;
        if (userId != -1) {
            savedDiffId = DBHelper::GetInstance().GetUserDifficulty(userId);
        }

        bool found = false;
        for (const auto& lvl : levels) {
            if (lvl.id == savedDiffId) {
                m_selectedLevel = lvl;
                found = true;
                break;
            }
        }

        if (!found) {
            m_selectedLevel = levels[0];
        }
    }

    srand((unsigned int)time(NULL));
}

CMagicCupGameView::~CMagicCupGameView()
{
}

BOOL CMagicCupGameView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    return CView::PreCreateWindow(cs);
}

int CMagicCupGameView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    InitOpenGL();
    SetTimer(1, 16, NULL);

    return 0;
}

void CMagicCupGameView::OnDestroy()
{
    KillTimer(1);
    CleanupOpenGL();
    CView::OnDestroy();
}

BOOL CMagicCupGameView::SetupPixelFormat()
{
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1 };
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(m_pDC->GetSafeHdc(), &pfd);
    if (pixelFormat == 0) return FALSE;
    if (!SetPixelFormat(m_pDC->GetSafeHdc(), pixelFormat, &pfd)) return FALSE;

    return TRUE;
}

void CMagicCupGameView::InitOpenGL()
{
    m_pDC = new CClientDC(this);

    if (!SetupPixelFormat()) {
        AfxMessageBox(_T("SetupPixelFormat failed"));
        return;
    }

    m_hRC = wglCreateContext(m_pDC->GetSafeHdc());
    if (!m_hRC) {
        AfxMessageBox(_T("wglCreateContext failed"));
        return;
    }

    if (!wglMakeCurrent(m_pDC->GetSafeHdc(), m_hRC)) {
        AfxMessageBox(_T("wglMakeCurrent failed"));
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);

    SetupLighting();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    const char* faces[6] = {
        "res/px.png",
        "res/nx.png",
        "res/py.png",
        "res/ny.png",
        "res/pz.png",
        "res/nz.png"
    };

    cubeMapTexture = LoadCubeMap(faces);

    tableTexture = LoadTexturePNG("res/wood.png");

    HFONT hFont = CreateFontA(
        -18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        RUSSIAN_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        PROOF_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Arial");

    HDC hdc = m_pDC->GetSafeHdc();
    m_fontBase = glGenLists(256);
    SelectObject(hdc, hFont);
    wglUseFontBitmapsA(hdc, 0, 256, m_fontBase);

    DeleteObject(hFont);
}

void CMagicCupGameView::SetupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light0_pos[] = { 5.0f, 15.0f, 5.0f, 1.0f };
    GLfloat light0_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light0_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light0_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

    GLfloat light1_pos[] = { -5.0f, 10.0f, -5.0f, 1.0f };
    GLfloat light1_diffuse[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat light1_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };

    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);

    GLfloat mat_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat mat_shininess[] = { 50.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
}

void CMagicCupGameView::CleanupOpenGL()
{
    if (m_hRC) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hRC);
        m_hRC = NULL;
    }

    if (m_pDC) {
        delete m_pDC;
        m_pDC = NULL;
    }

    if (m_fontBase) {
        glDeleteLists(m_fontBase, 256);
        m_fontBase = 0;
    }
}

void CMagicCupGameView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);
    m_windowWidth = cx;
    m_windowHeight = cy;

    if (m_hRC && cy > 0) {
        glViewport(0, 0, cx, cy);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, (double)cx / cy, 1.0, 100.0);
        glMatrixMode(GL_MODELVIEW);
    }
}

void CMagicCupGameView::OnDraw(CDC* /*pDC*/)
{
    if (!m_hRC)
        return;

    wglMakeCurrent(m_pDC->GetSafeHdc(), m_hRC);
    DrawScene();
    SwapBuffers(m_pDC->GetSafeHdc());
}

void CMagicCupGameView::DrawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    float camX = m_cameraDistance * cos(m_cameraAngle * M_PI / 180.0f);
    float camZ = m_cameraDistance * sin(m_cameraAngle * M_PI / 180.0f);
    gluLookAt(camX, m_cameraHeight, camZ, 0, 0, 0, 0, 1, 0);

    DrawSky();

    DrawTable();

    for (int i = 0; i < 3; i++) {
        bool selected = (m_selectedCup == i);
        bool hovered = (m_gameState == GAME_GUESSING && m_hoveredCup == i);
        DrawCupWithDetails(m_cupPositions[i][0], m_cupPositions[i][1] + m_cupHeights[i],
            m_cupPositions[i][2], 0.0f, selected || hovered);
    }

    float ballY = -0.6f;
    DrawGradientBall(m_cupPositions[m_ballPosition][0], ballY,
        m_cupPositions[m_ballPosition][2], 0.4f);

    DrawHUD();
}

GLuint CMagicCupGameView::LoadTexturePNG(const char* filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) return 0;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    return texID;
}

GLuint CMagicCupGameView::LoadCubeMap(const char* faces[6]) {
    int width, height, channels;
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    for (int i = 0; i < 6; i++) {
        unsigned char* data = stbi_load(faces[i], &width, &height, &channels, 0);
        if (!data) {
            printf("Failed to load cubemap face: %s\n", faces[i]);
            glDeleteTextures(1, &texID);
            return 0;
        }

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data
        );

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texID;
}

void CMagicCupGameView::DrawSky() {
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    float s = 40.0f;

    glPushMatrix();

    glBegin(GL_QUADS);

    glTexCoord3f(1.0f, -1.0f, -1.0f); glVertex3f(s, -s, -s);
    glTexCoord3f(1.0f, -1.0f, 1.0f); glVertex3f(s, -s, s);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(s, s, s);
    glTexCoord3f(1.0f, 1.0f, -1.0f); glVertex3f(s, s, -s);

    glTexCoord3f(-1.0f, -1.0f, 1.0f); glVertex3f(-s, -s, s);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-s, -s, -s);
    glTexCoord3f(-1.0f, 1.0f, -1.0f); glVertex3f(-s, s, -s);
    glTexCoord3f(-1.0f, 1.0f, 1.0f); glVertex3f(-s, s, s);

    glTexCoord3f(-1.0f, 1.0f, -1.0f); glVertex3f(-s, s, -s);
    glTexCoord3f(1.0f, 1.0f, -1.0f); glVertex3f(s, s, -s);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(s, s, s);
    glTexCoord3f(-1.0f, 1.0f, 1.0f); glVertex3f(-s, s, s);

    glTexCoord3f(-1.0f, -1.0f, 1.0f); glVertex3f(-s, -s, s);
    glTexCoord3f(1.0f, -1.0f, 1.0f); glVertex3f(s, -s, s);
    glTexCoord3f(1.0f, -1.0f, -1.0f); glVertex3f(s, -s, -s);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-s, -s, -s);

    glTexCoord3f(-1.0f, -1.0f, 1.0f); glVertex3f(-s, -s, s);
    glTexCoord3f(1.0f, -1.0f, 1.0f); glVertex3f(s, -s, s);
    glTexCoord3f(1.0f, 1.0f, 1.0f); glVertex3f(s, s, s);
    glTexCoord3f(-1.0f, 1.0f, 1.0f); glVertex3f(-s, s, s);

    glTexCoord3f(1.0f, -1.0f, -1.0f); glVertex3f(s, -s, -s);
    glTexCoord3f(-1.0f, -1.0f, -1.0f); glVertex3f(-s, -s, -s);
    glTexCoord3f(-1.0f, 1.0f, -1.0f); glVertex3f(-s, s, -s);
    glTexCoord3f(1.0f, 1.0f, -1.0f); glVertex3f(s, s, -s);

    glEnd();

    glPopMatrix();

    glDisable(GL_TEXTURE_CUBE_MAP);

    glPopAttrib();
}

void CMagicCupGameView::DrawRoundedCorner(float cx, float cz, float y, float radius, int segments, float startAngle)
{
    float thickness = 0.5f;
    float yBottom = y - thickness;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++)
    {
        float angle = startAngle + (float)i / (float)segments * (M_PI / 2.0f);
        float x = cx + cos(angle) * radius;
        float z = cz + sin(angle) * radius;
        glNormal3f(cos(angle), 0, sin(angle));
        glTexCoord2f((float)i / (float)segments, 0); glVertex3f(x, yBottom, z);
        glTexCoord2f((float)i / (float)segments, 1); glVertex3f(x, y, z);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(cx, y, cz);
    for (int i = 0; i <= segments; i++)
    {
        float angle = startAngle + (float)i / (float)segments * (M_PI / 2.0f);
        glVertex3f(cx + cos(angle) * radius, y, cz + sin(angle) * radius);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(cx, yBottom, cz);
    for (int i = 0; i <= segments; i++)
    {
        float angle = startAngle + (float)i / (float)segments * (M_PI / 2.0f);
        glVertex3f(cx + cos(angle) * radius, yBottom, cz + sin(angle) * radius);
    }
    glEnd();
}

void CMagicCupGameView::DrawTableTop(float width, float depth, float height, float radius, int segments, GLuint textureID, float yTop)
{
    float halfW = width / 2.0f;
    float halfD = depth / 2.0f;
    float yBottom = yTop - height;
    if (textureID != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    glColor3f(1, 1, 1);
    glNormal3f(0, 1, 0);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfW + radius, yTop, -halfD + radius);
    glTexCoord2f(1, 0); glVertex3f(halfW - radius, yTop, -halfD + radius);
    glTexCoord2f(1, 1); glVertex3f(halfW - radius, yTop, halfD - radius);
    glTexCoord2f(0, 1); glVertex3f(-halfW + radius, yTop, halfD - radius);
    glEnd();
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfW + radius, yTop, -halfD + radius);
    glTexCoord2f(.1, 0); glVertex3f(-halfW, yTop, -halfD + radius);
    glTexCoord2f(.1, .1); glVertex3f(-halfW, yTop, halfD - radius);
    glTexCoord2f(0, .1); glVertex3f(-halfW + radius, yTop, halfD - radius);
    glEnd();
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(halfW, yTop, -halfD + radius);
    glTexCoord2f(.1, 0); glVertex3f(halfW - radius, yTop, -halfD + radius);
    glTexCoord2f(.1, .1); glVertex3f(halfW - radius, yTop, halfD - radius);
    glTexCoord2f(0, .1); glVertex3f(halfW, yTop, halfD - radius);
    glEnd();
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfW + radius, yTop, -halfD);
    glTexCoord2f(.1, 0); glVertex3f(halfW - radius, yTop, -halfD);
    glTexCoord2f(.1, .1); glVertex3f(halfW - radius, yTop, -halfD + radius);
    glTexCoord2f(0, .1); glVertex3f(-halfW + radius, yTop, -halfD + radius);
    glEnd();
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-halfW + radius, yTop, halfD - radius);
    glTexCoord2f(.1, 0); glVertex3f(halfW - radius, yTop, halfD - radius);
    glTexCoord2f(.1, .1); glVertex3f(halfW - radius, yTop, halfD);
    glTexCoord2f(0, .1); glVertex3f(-halfW + radius, yTop, halfD);
    glEnd();
    glBegin(GL_QUAD_STRIP);
    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(-halfW + radius, yBottom, -halfD);
    glTexCoord2f(0, 1); glVertex3f(-halfW + radius, yTop, -halfD);
    glTexCoord2f(1, 0); glVertex3f(halfW - radius, yBottom, -halfD);
    glTexCoord2f(1, 1); glVertex3f(halfW - radius, yTop, -halfD);
    glEnd();
    glBegin(GL_QUAD_STRIP);
    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(halfW, yBottom, -halfD + radius);
    glTexCoord2f(0, 1); glVertex3f(halfW, yTop, -halfD + radius);
    glTexCoord2f(1, 0); glVertex3f(halfW, yBottom, halfD - radius);
    glTexCoord2f(1, 1); glVertex3f(halfW, yTop, halfD - radius);
    glEnd();
    glBegin(GL_QUAD_STRIP);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(halfW - radius, yBottom, halfD);
    glTexCoord2f(0, 1); glVertex3f(halfW - radius, yTop, halfD);
    glTexCoord2f(1, 0); glVertex3f(-halfW + radius, yBottom, halfD);
    glTexCoord2f(1, 1); glVertex3f(-halfW + radius, yTop, halfD);
    glEnd();
    glBegin(GL_QUAD_STRIP);
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-halfW, yBottom, halfD - radius);
    glTexCoord2f(0, 1); glVertex3f(-halfW, yTop, halfD - radius);
    glTexCoord2f(1, 0); glVertex3f(-halfW, yBottom, -halfD + radius);
    glTexCoord2f(1, 1); glVertex3f(-halfW, yTop, -halfD + radius);
    glEnd();
    DrawRoundedCorner(halfW - radius, halfD - radius, yTop, radius, segments, 0);
    DrawRoundedCorner(-halfW + radius, halfD - radius, yTop, radius, segments, M_PI / 2);
    DrawRoundedCorner(-halfW + radius, -halfD + radius, yTop, radius, segments, M_PI);
    DrawRoundedCorner(halfW - radius, -halfD + radius, yTop, radius, segments, 3 * M_PI / 2);
    if (textureID != 0)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
}

void CMagicCupGameView::DrawTableLegs(float yTop, float height)
{
    float legPositions[4][2] = { {-8,-4},{8,-4},{8,4},{-8,4} };
    glColor3f(0.3f, 0.2f, 0.08f);
    for (int i = 0; i < 4; i++)
    {
        glPushMatrix();
        glTranslatef(legPositions[i][0], yTop - height, legPositions[i][1]);
        glRotatef(90, 1, 0, 0);
        GLUquadricObj* quadric = gluNewQuadric();
        gluCylinder(quadric, 0.2f, 0.15f, 3.0f, 16, 1);
        gluDeleteQuadric(quadric);
        glPopMatrix();
    }
}

void CMagicCupGameView::DrawTable()
{
    float width = 20.0f;
    float depth = 12.0f;
    float height = 0.5f;
    float radius = 1.0f;
    int segments = 16;
    float yTop = -1.0f;
    DrawTableTop(width, depth, height, radius, segments, tableTexture, yTop);
    DrawTableLegs(yTop, height);
}

void CMagicCupGameView::DrawCupWithDetails(float x, float y, float z, float height, bool highlight)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    if (highlight) {
        glColor3f(0.8f, 0.2f, 0.2f);
    } else {
        glColor3f(0.6f, 0.1f, 0.1f);
    }

    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);

    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    glRotatef(-180, 0, 1, 0);
    gluCylinder(quadric, 0.6f, 0.9f, 1.5f, 20, 4);
    glPopMatrix();

    glPushMatrix();
    glRotatef(90, 1, 0, 0);
    gluDisk(quadric, 0, 0.6f, 20, 1);
    glPopMatrix();

    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void CMagicCupGameView::DrawGradientBall(float x, float y, float z, float radius)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    glColor3f(0.8f, 0.8f, 0.8f);

    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);

    GLfloat mat_ambient_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_shininess[] = { 50.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_ambient_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    gluSphere(quadric, radius, 20, 20);

    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void CMagicCupGameView::HUDPrint(int x, int y, const char* text)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRasterPos2i(x, y);
    glPopMatrix();
    glListBase(m_fontBase);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, (const GLubyte*)text);
}

int CMagicCupGameView::TextWidth(const char* text)
{
    SIZE sz{};
    HDC hdc = m_pDC->GetSafeHdc();
    GetTextExtentPoint32A(hdc, text, (int)strlen(text), &sz);
    return sz.cx;
}

void CMagicCupGameView::DrawHUD()
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, m_windowWidth, 0.0, m_windowHeight, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const char* lines[5];
    int lineCount = 0;
    char buf1[256], buf2[256], buf3[256], bufDiff[256];

    switch (m_gameState) {
    case GAME_NONE:          sprintf_s(buf1, "СТАТУС: Меню"); break;
    case GAME_WAITING:       sprintf_s(buf1, "СТАТУС: Очікування"); break;
    case GAME_LIFTING_CUPS:  sprintf_s(buf1, "СТАТУС: Увага..."); break;
    case GAME_SHOWING_BALL:  sprintf_s(buf1, "СТАТУС: Запам'ятай!"); break;
    case GAME_LOWERING_CUPS: sprintf_s(buf1, "СТАТУС: Готовий?"); break;
    case GAME_SHUFFLING:     sprintf_s(buf1, "СТАТУС: Стеж за кулькою!"); break;
    case GAME_GUESSING:      sprintf_s(buf1, "СТАТУС: Твій вибір?"); break;
    case GAME_RESULT:        sprintf_s(buf1, "СТАТУС: Фінал"); break;
    default:                 sprintf_s(buf1, "СТАТУС: ?"); break;
    }
    lines[lineCount++] = buf1;

    sprintf_s(bufDiff, "[ %s ]", (LPCSTR)CT2A(m_selectedLevel.name));
    lines[lineCount++] = bufDiff;

    if (m_gameState == GAME_SHUFFLING)
        sprintf_s(buf2, "Перемішування: %d / %d", m_shuffleStep + 1, m_selectedLevel.shuffleCount);
    else
        sprintf_s(buf2, "ПРОБІЛ - Старт");
    lines[lineCount++] = buf2;

    if ((m_gameState == GAME_RESULT || m_gameState == GAME_NONE) && m_selectedCup >= 0) {
        if (m_gameWon) sprintf_s(buf3, "*** ПЕРЕМОГА! ***");
        else           sprintf_s(buf3, "ПРОГРАШ (Кулька: %d)", m_ballPosition + 1);
        lines[lineCount++] = buf3;
    }

    int maxTextW = 0;
    for (int i = 0; i < lineCount; ++i)
        maxTextW = max(maxTextW, TextWidth(lines[i]));

    const int paddingX = 30;
    const int paddingY = 15;
    const int lineHeight = 22;

    int panelW = maxTextW + (paddingX * 2);
    int panelH = (lineCount * lineHeight) + (paddingY * 2);

    int centerX = m_windowWidth / 2;
    int left = centerX - (panelW / 2);
    int right = left + panelW;
    int top = m_windowHeight - 20;
    int bottom = top - panelH;

    glBegin(GL_QUADS);
    glColor4f(0.05f, 0.1f, 0.3f, 0.9f);
    glVertex2f(left, top);
    glVertex2f(right, top);

    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glVertex2f(right, bottom);
    glVertex2f(left, bottom);
    glEnd();

    glLineWidth(2.0f);
    glColor4f(1.0f, 0.8f, 0.2f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(left, top);
    glVertex2f(right, top);
    glVertex2f(right, bottom);
    glVertex2f(left, bottom);
    glEnd();

    for (int i = 0; i < lineCount; ++i) {
        int textW = TextWidth(lines[i]);
        int x = centerX - (textW / 2);
        int y = top - paddingY - 15 - (i * lineHeight);

        if (i == 0) glColor3f(1.0f, 0.9f, 0.0f);
        else if (i == lineCount - 1 && (m_gameState == GAME_RESULT || m_gameState == GAME_NONE)) {
            if (m_gameWon) glColor3f(0.2f, 1.0f, 0.2f);
            else glColor3f(1.0f, 0.3f, 0.3f);
        }
        else glColor3f(1.0f, 1.0f, 1.0f);

        HUDPrint(x, y, lines[i]);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void CMagicCupGameView::OnTimer(UINT_PTR nIDEvent)
{
    UpdateAnimation();
    Invalidate(FALSE);
    CView::OnTimer(nIDEvent);
}

void CMagicCupGameView::UpdateAnimation()
{
    DWORD currentTime = GetTickCount();

    switch (m_gameState) {
    case GAME_WAITING:
        if (currentTime - m_stateStartTime > 2000) {
            m_gameState = GAME_LIFTING_CUPS;
            m_stateStartTime = currentTime;
            for (int i = 0; i < 3; i++) {
                m_cupTargetHeights[i] = 2.0f;
            }
            m_isAnimating = true;
        }
        break;

    case GAME_LIFTING_CUPS:
        if (m_isAnimating) {
            float progress = min(1.0f, (currentTime - m_stateStartTime) / 1000.0f);
            for (int i = 0; i < 3; i++) {
                m_cupHeights[i] = progress * m_cupTargetHeights[i];
            }
            if (progress >= 1.0f) {
                m_gameState = GAME_SHOWING_BALL;
                m_stateStartTime = currentTime;
                m_isAnimating = false;
            }
        }
        break;

    case GAME_SHOWING_BALL:
        if (currentTime - m_stateStartTime > 2000) {
            m_gameState = GAME_LOWERING_CUPS;
            m_stateStartTime = currentTime;
            for (int i = 0; i < 3; i++) {
                m_cupTargetHeights[i] = 0.0f;
            }
            m_isAnimating = true;
        }
        break;

    case GAME_LOWERING_CUPS:
        if (m_isAnimating) {
            float progress = min(1.0f, (currentTime - m_stateStartTime) / 1000.0f);
            for (int i = 0; i < 3; i++) {
                m_cupHeights[i] = (1.0f - progress) * 2.0f;
            }
            if (progress >= 1.0f) {
                m_gameState = GAME_SHUFFLING;
                m_stateStartTime = currentTime;
                m_shuffleStep = 0;
                m_isAnimating = true;
                SetupShuffleStep();
            }
        }
        break;

    case GAME_SHUFFLING:
        if (m_isAnimating) {
            float progress = min(1.0f, (currentTime - m_stateStartTime) / (float)m_selectedLevel.animationSpeed);

            for (int i = 0; i < 3; i++) {
                float sx = m_startPositions[i][0];
                float sz = m_startPositions[i][2];
                float ex = m_cupTargets[i][0];
                float ez = m_cupTargets[i][2];

                float dx = ex - sx;
                float dz = ez - sz;

                float perpX = -dz;
                float perpZ = dx;
                float len = sqrt(perpX * perpX + perpZ * perpZ);
                if (len > 0.001f) {
                    perpX /= len;
                    perpZ /= len;
                }

                float h = 0.8f;
                float arc = sin(progress * M_PI) * h;

                float cx = sx + dx * progress;
                float cz = sz + dz * progress;

                m_cupPositions[i][0] = cx + perpX * arc;
                m_cupPositions[i][2] = cz + perpZ * arc;
            }

            if (progress >= 1.0f) {
                for (int i = 0; i < 3; i++) {
                    m_cupPositions[i][0] = m_cupTargets[i][0];
                    m_cupPositions[i][2] = m_cupTargets[i][2];
                }

                m_shuffleStep++;
                if (m_shuffleStep < m_selectedLevel.shuffleCount) {
                    SetupShuffleStep();
                    m_stateStartTime = currentTime;
                }
                else {
                    m_gameState = GAME_GUESSING;
                    m_isAnimating = false;
                }
            }
        }
        break;

    case GAME_RESULT:
        if (m_isAnimating) {
            float progress = min(1.0f, (currentTime - m_stateStartTime) / 1000.0f);
            m_cupHeights[m_selectedCup] = progress * m_cupTargetHeights[m_selectedCup];

            if (progress >= 1.0f) {
                CMagicCupGameApp* pApp = (CMagicCupGameApp*)AfxGetApp();
                int currentUserId = pApp->m_nCurrentUserID;

                if (currentUserId != -1) {
                    DBHelper::GetInstance().AddGameSession(
                        currentUserId,
                        m_selectedLevel.id,
                        m_ballPosition + 1,
                        m_selectedCup + 1,
                        m_gameWon
                    );
                }

                m_isAnimating = false;
                m_stateStartTime = GetTickCount();
            }
        }
        else
        {
            if (currentTime - m_stateStartTime > 5000) {
                m_gameState = GAME_NONE;
                m_stateStartTime = GetTickCount();
                m_isAnimating = true;
                m_cupTargetHeights[m_selectedCup] = 0.0f;
            }
        }
        break;

    case GAME_NONE:
        if (m_isAnimating) {
            float progress = min(1.0f, (currentTime - m_stateStartTime) / 1000.0f);
            m_cupHeights[m_selectedCup] = (1.0f - progress) * 2.0f;
            if (progress >= 1.0f) {
                m_cupHeights[m_selectedCup] = 0.0f;
                m_selectedCup = -1;
                m_isAnimating = false;
            }
        }
        break;
    }
}

void CMagicCupGameView::SetupShuffleStep()
{
    int cup1_idx = rand() % 3;
    int cup2_idx;
    do {
        cup2_idx = rand() % 3;
    } while (cup2_idx == cup1_idx);

    for (int k = 0; k < 3; k++) {
        m_startPositions[cup1_idx][k] = m_cupPositions[cup1_idx][k];
        m_startPositions[cup2_idx][k] = m_cupPositions[cup2_idx][k];
    }

    for (int k = 0; k < 3; k++) {
        m_cupTargets[cup1_idx][k] = m_cupPositions[cup2_idx][k];
        m_cupTargets[cup2_idx][k] = m_cupPositions[cup1_idx][k];
    }

    int cup3_idx = 3 - cup1_idx - cup2_idx;
    for (int k = 0; k < 3; k++) {
        m_startPositions[cup3_idx][k] = m_cupPositions[cup3_idx][k];
        m_cupTargets[cup3_idx][k] = m_cupPositions[cup3_idx][k];
    }

    m_swapIndices[0] = cup1_idx;
    m_swapIndices[1] = cup2_idx;

    m_isAnimating = true;
}

void CMagicCupGameView::StartNewGame()
{
    m_ballPosition = rand() % 3;
    m_selectedCup = -1;
    m_gameWon = false;
    m_gameState = GAME_WAITING;
    m_stateStartTime = GetTickCount();
    m_isAnimating = false;
    m_shuffleStep = 0;

    m_cupPositions[0][0] = -3.0f; m_cupPositions[0][1] = 0.5f; m_cupPositions[0][2] = 0.0f;
    m_cupPositions[1][0] = 0.0f;  m_cupPositions[1][1] = 0.5f; m_cupPositions[1][2] = 0.0f;
    m_cupPositions[2][0] = 3.0f;  m_cupPositions[2][1] = 0.5f; m_cupPositions[2][2] = 0.0f;

    for (int i = 0; i < 3; i++) {
        m_cupHeights[i] = 0.0f;
        m_cupTargetHeights[i] = 0.0f;
    }
}

int CMagicCupGameView::GetCupIndexFromScreenPos(CPoint point)
{
    CRect rect;
    GetClientRect(&rect);

    float x_norm = (float)point.x / rect.Width();
    float y_norm = (float)point.y / rect.Height();

    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY, winZ;
    GLdouble posX, posY, posZ;

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    winX = (float)point.x;
    winY = (float)viewport[3] - (float)point.y;
    glReadPixels(winX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

    gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

    int nearestCup = -1;
    float minDistance = 10000.0f;

    for (int i = 0; i < 3; ++i) {
        float dx = posX - m_cupPositions[i][0];
        float dz = posZ - m_cupPositions[i][2];
        float distance = sqrtf(dx * dx + dz * dz);

        if (distance < minDistance) {
            minDistance = distance;
            nearestCup = i;
        }
    }

    if (minDistance < 2.0f) {
        return nearestCup;
    }

    return -1;
}

int CMagicCupGameView::GetCupIndexByPosition(int positionIndex)
{
    float targetX = 0.0f;
    if (positionIndex == 0) targetX = -3.0f;
    else if (positionIndex == 1) targetX = 0.0f;
    else if (positionIndex == 2) targetX = 3.0f;

    for (int i = 0; i < 3; i++) {
        if (fabs(m_cupPositions[i][0] - targetX) < 1.0f) {
            return i;
        }
    }
    return -1;
}

void CMagicCupGameView::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_gameState == GAME_GUESSING) {
        int cupIndex = GetCupIndexFromScreenPos(point);
        if (cupIndex != -1) {
            OnCupClick(cupIndex);
        }
    }

    m_mousePressed = true;
    m_lastMousePos = point;
    SetCapture();

    CView::OnLButtonDown(nFlags, point);
}

void CMagicCupGameView::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_mousePressed) {
        m_mousePressed = false;
        ReleaseCapture();
    }

    CView::OnLButtonUp(nFlags, point);
}

void CMagicCupGameView::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_mousePressed && (nFlags & MK_LBUTTON)) {
        int deltaX = point.x - m_lastMousePos.x;
        m_cameraAngle += deltaX * 0.5f;

        if (m_cameraAngle >= 360.0f) m_cameraAngle -= 360.0f;
        if (m_cameraAngle < 0.0f) m_cameraAngle += 360.0f;

        m_lastMousePos = point;
    }

    int cupIndex = GetCupIndexFromScreenPos(point);
    m_hoveredCup = cupIndex;
    Invalidate(FALSE);

    CView::OnMouseMove(nFlags, point);
}

BOOL CMagicCupGameView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    m_cameraDistance -= zDelta * 0.01f;
    if (m_cameraDistance < 8.0f) m_cameraDistance = 8.0f;
    if (m_cameraDistance > 25.0f) m_cameraDistance = 25.0f;

    Invalidate(FALSE);
    return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CMagicCupGameView::OnCupClick(int cupIndex)
{
    if (cupIndex < 0 || cupIndex > 2)
        return;

    m_selectedCup = cupIndex;
    m_gameWon = (cupIndex == m_ballPosition);
    m_cupTargetHeights[m_selectedCup] = 2.0f;
    m_gameState = GAME_RESULT;
    m_stateStartTime = GetTickCount();
    m_isAnimating = true;
}

void CMagicCupGameView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (nChar == VK_SPACE) {
        StartNewGame();
    }
    else if (m_gameState == GAME_GUESSING && nChar >= '1' && nChar <= '3') {
        int positionIndex = nChar - '1';
        int realCupIndex = GetCupIndexByPosition(positionIndex);

        if (realCupIndex != -1) {
            OnCupClick(realCupIndex);
        }
    }
    else if (nChar == VK_LEFT) {
        m_cameraAngle += 5.0f;
        if (m_cameraAngle >= 360.0f) m_cameraAngle -= 360.0f;
        Invalidate(FALSE);
    }
    else if (nChar == VK_RIGHT) {
        m_cameraAngle -= 5.0f;
        if (m_cameraAngle < 0.0f) m_cameraAngle += 360.0f;
        Invalidate(FALSE);
    }
    else if (nChar == VK_UP) {
        m_cameraHeight += 0.5f;
        if (m_cameraHeight > 15.0f) m_cameraHeight = 15.0f;
        Invalidate(FALSE);
    }
    else if (nChar == VK_DOWN) {
        m_cameraHeight -= 0.5f;
        if (m_cameraHeight < 3.0f) m_cameraHeight = 3.0f;
        Invalidate(FALSE);
    }

    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}


// Печать CMagicCupGameView

BOOL CMagicCupGameView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // подготовка по умолчанию
    return DoPreparePrinting(pInfo);
}

void CMagicCupGameView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: добавьте дополнительную инициализацию перед печатью
}

void CMagicCupGameView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    // TODO: добавьте очистку после печати
}


// Диагностика CMagicCupGameView

#ifdef _DEBUG
void CMagicCupGameView::AssertValid() const
{
    CView::AssertValid();
}

void CMagicCupGameView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CMagicCupGameDoc* CMagicCupGameView::GetDocument() const // встроена неотлаженная версия
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMagicCupGameDoc)));
    return (CMagicCupGameDoc*)m_pDocument;
}
#endif //_DEBUG


// Обработчики сообщений CMagicCupGameView

void CMagicCupGameView::OnSettings()
{
    if (m_gameState != GAME_NONE && m_gameState != GAME_WAITING) {
        AfxMessageBox(_T("Налаштування доступні лише перед початком гри!"));
        return;
    }

    CSettingsDlg dlg;
    dlg.m_nInitialId = m_selectedLevel.id;

    if (dlg.DoModal() == IDOK) {
        m_selectedLevel = dlg.m_selectedLevel;

        CMagicCupGameApp* pApp = (CMagicCupGameApp*)AfxGetApp();
        int currentUserId = pApp->m_nCurrentUserID;

        if (currentUserId != -1) {
            DBHelper::GetInstance().SaveUserDifficulty(currentUserId, m_selectedLevel.id);
        }

        CString str;
        str.Format(_T("Обрано: %s\nКроків: %d\nШвидкість: %d\n(Збережено в профілі)"),
            m_selectedLevel.name,
            m_selectedLevel.shuffleCount,
            m_selectedLevel.animationSpeed);
        AfxMessageBox(str);
    }
}