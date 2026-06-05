// MagicCupGameView.cpp: реализация класса CMagicCupGameView
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS можно определять в проектах просмотра, эскизов и поиска ATL, реализующих
// в коде; позволяет совместно использовать код реализации с этим проектом.
#ifndef SHARED_HANDLERS
#include "MagicCupGame.h"
#endif

#include "MagicCupGameDoc.h"
#include "MagicCupGameView.h"

#include <time.h>
#include <math.h>
#include <algorithm>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define M_PI 3.14159265358979323846
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h>
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")

#pragma setlocale("RUSSIAN")

#include "SettingsDlg.h"
#include "AdminDlg.h"
#include "ShaderSources.h"

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
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_COMMAND(ID_SETTINGS, &CMagicCupGameView::OnSettings)
    ON_COMMAND(ID_ADMIN, &CMagicCupGameView::OnAdmin)
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_KEYUP()
END_MESSAGE_MAP()

// Создание или уничтожение CMagicCupGameView

CMagicCupGameView::CMagicCupGameView() : m_hRC(NULL), m_pDC(NULL)
{
    m_gameState = GAME_NONE;
    m_ballPosition = 0;
    m_selectedCup = -1;
    m_gameWon = false;
    m_isAnimating = false;
    m_isFlashlightOn = false;
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

    m_keyW = m_keyA = m_keyS = m_keyD = false;
    m_keySpace = m_keyCtrl = m_keyShift = false;
    m_rmbDown = false;

    m_camX = DEF_CAM_X;
    m_camY = DEF_CAM_Y;
    m_camZ = DEF_CAM_Z;
    m_camYaw = DEF_CAM_YAW;
    m_camPitch = DEF_CAM_PITCH;

    m_velX = 0.0f;
    m_velY = 0.0f;
    m_velZ = 0.0f;

    m_selectedLevel = DifficultyLevel{ -1, L"Звичайна", 4, 500 };

    std::vector<DifficultyLevel> levels = DBHelper::GetInstance().GetDifficultyLevels();

    if (levels.empty()) {
        m_selectedLevel = { -1, _T("Звичайна"), 3, 800 };
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

    m_lastDBUpdate = GetTickCount();

    InitMaterialPresets();

    LoadGameSettings();

    srand((unsigned int)time(NULL));
}

CMagicCupGameView::~CMagicCupGameView()
{
}

void CMagicCupGameView::LoadGameSettings()
{
    CMagicCupGameApp* pApp = (CMagicCupGameApp*)AfxGetApp();
    int userId = pApp->m_nCurrentUserID;

    bool isLoaded = false;
    if (userId != -1) {
        if (DBHelper::GetInstance().GetGameSettings(userId, m_gameSettings)) {
            isLoaded = true;
        }
    }

    if (!isLoaded) {
        DBHelper::GetInstance().GetGameSettings(-1, m_gameSettings);
    }

    m_currentSkyboxIndex = m_gameSettings.skyboxIndex;
    m_currentTableMatIndex = m_gameSettings.tableMatIndex;
    m_currentCarpetMatIndex = m_gameSettings.carpetMatIndex;
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
    SetTimer(1, 1, NULL);

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

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);

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

    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        const char* err = (const char*)glewGetErrorString(glewStatus);
        CString errStr(err ? err : "unknown");
        CString msg = _T("glewInit failed: ") + errStr;
        AfxMessageBox(msg);
    } else {
        InitShaders();
    }

    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(0);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);

    SetupLighting();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    InitMaterialPresets();

    int targetSkybox = (m_currentSkyboxIndex >= 0) ? m_currentSkyboxIndex : 0;
    m_currentSkyboxIndex = -1;
    SelectSkybox(targetSkybox);

    int targetTable = (m_currentTableMatIndex >= 0) ? m_currentTableMatIndex : 0;
    m_currentTableMatIndex = -1;
    SelectTable(targetTable);

	int targetCarpet = (m_currentCarpetMatIndex >= 0) ? m_currentCarpetMatIndex : 0;
    m_currentCarpetMatIndex = -1;
	SelectCarpet(targetCarpet);

    QueryPerformanceFrequency(&m_qpFrequency);
    QueryPerformanceCounter(&m_qpLastTime);
}

void CMagicCupGameView::SetupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light0_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, m_actLightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, m_actDiffuse);
    glLightfv(GL_LIGHT0, GL_AMBIENT, m_actAmbient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

    GLfloat light1_pos[] = { -5.0f, 10.0f, -5.0f, 1.0f };
    GLfloat light1_diffuse[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);

    GLfloat mat_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat mat_shininess[] = { 50.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
}

void CMagicCupGameView::CleanupOpenGL()
{
    if (m_hRC) {
        wglMakeCurrent(m_pDC->GetSafeHdc(), m_hRC);
        m_mainShader.Destroy();
        m_depthShader.Destroy();
        m_shadowMap.Destroy();
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hRC);
        m_hRC = NULL;
    }

    if (m_pDC) {
        delete m_pDC;
        m_pDC = NULL;
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
        gluPerspective(45.0, (double)cx / cy, 1.0, 500.0);
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

void CMagicCupGameView::InitShaders()
{
    CString err;
    if (!m_mainShader.BuildFromSource(kMainVertSrc, kMainFragSrc, err)) {
        AfxMessageBox(_T("Main shader error: ") + err);
        m_shadersReady = false;
        return;
    }
    if (!m_depthShader.BuildFromSource(kDepthVertSrc, kDepthFragSrc, err)) {
        AfxMessageBox(_T("Depth shader error: ") + err);
        m_shadersReady = false;
        return;
    }
    if (!m_shadowMap.Create(4096)) {
        AfxMessageBox(_T("Shadow map FBO failed"));
        m_shadersReady = false;
        return;
    }
    m_shadersReady = true;
}

void CMagicCupGameView::RenderShadowPass(const float lightPosWorld[3])
{
    Mat4 lightView = Mat4::LookAt(
        lightPosWorld[0], lightPosWorld[1], lightPosWorld[2],
        0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f);

    Mat4 lightProj = Mat4::Ortho(-14.0f, 14.0f, -9.0f, 9.0f, 3.0f, 60.0f);
    m_lightViewProj = Mat4::Multiply(lightProj, lightView);

    m_shadowMap.BeginPass();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(lightProj.m);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(lightView.m);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 2.5f);

    m_depthShader.Use();
    DrawTable();
    DrawGameObjects(true);
    Shader::Unuse();

    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    m_shadowMap.EndPass(m_windowWidth, m_windowHeight);
}

void CMagicCupGameView::DrawScene()
{
    float radYaw = m_camYaw * M_PI / 180.0f;
    float radPitch = m_camPitch * M_PI / 180.0f;

    float lookX = cos(radPitch) * cos(radYaw);
    float lookY = sin(radPitch);
    float lookZ = cos(radPitch) * sin(radYaw);

    GLfloat currentLightPos[4] = {
        m_actLightPos[0], m_actLightPos[1], m_actLightPos[2], 1.0f
    };

    float flashPosWorld[3];
    float flashDirWorld[3];
    {
        float rightX = -sin(radYaw);
        float rightZ = cos(radYaw);
        flashPosWorld[0] = m_camX + rightX * 0.3f;
        flashPosWorld[1] = m_camY - 0.15f;
        flashPosWorld[2] = m_camZ + rightZ * 0.3f;
        flashDirWorld[0] = lookX;
        flashDirWorld[1] = lookY;
        flashDirWorld[2] = lookZ;
    }

    if (m_shadersReady) {
        float shadowLightPos[3] = { currentLightPos[0], currentLightPos[1], currentLightPos[2] };
        float len = sqrtf(shadowLightPos[0]*shadowLightPos[0] +
                          shadowLightPos[1]*shadowLightPos[1] +
                          shadowLightPos[2]*shadowLightPos[2]);
        if (len > 0.001f) {
            float target = 30.0f;
            shadowLightPos[0] *= target / len;
            shadowLightPos[1] *= target / len;
            shadowLightPos[2] *= target / len;
        } else {
            shadowLightPos[0] = 10.0f; shadowLightPos[1] = 25.0f; shadowLightPos[2] = 10.0f;
        }
        RenderShadowPass(shadowLightPos);
    }

    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (m_windowHeight > 0)
        gluPerspective(45.0, (double)m_windowWidth / m_windowHeight, 1.0, 500.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(m_camX, m_camY, m_camZ,
        m_camX + lookX, m_camY + lookY, m_camZ + lookZ,
        0.0f, 1.0f, 0.0f);

    GLfloat viewMtx[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, viewMtx);

    Mat4 view; memcpy(view.m, viewMtx, sizeof(viewMtx));
    Mat4 viewInv = Mat4::Invert(view);
    Mat4 lightFromView = Mat4::Multiply(m_lightViewProj, viewInv);

    float lpv[4];
    lpv[0] = viewMtx[0]*currentLightPos[0] + viewMtx[4]*currentLightPos[1] + viewMtx[8]*currentLightPos[2]  + viewMtx[12];
    lpv[1] = viewMtx[1]*currentLightPos[0] + viewMtx[5]*currentLightPos[1] + viewMtx[9]*currentLightPos[2]  + viewMtx[13];
    lpv[2] = viewMtx[2]*currentLightPos[0] + viewMtx[6]*currentLightPos[1] + viewMtx[10]*currentLightPos[2] + viewMtx[14];

    DrawSky();

    if (m_shadersReady) {
        m_mainShader.Use();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_shadowMap.Tex());
        glActiveTexture(GL_TEXTURE0);

        m_mainShader.Set1i("uTex", 0);
        m_mainShader.Set1i("uShadowMap", 1);
        m_mainShader.SetMat4("uLightFromView", lightFromView.m);
        m_mainShader.Set3f("uLightPosView", lpv[0], lpv[1], lpv[2]);

        bool darkNight = (m_actDiffuse[0] + m_actDiffuse[1] + m_actDiffuse[2] < 0.05f);
        if (darkNight) {
            m_mainShader.Set3f("uLightColor", 0.0f, 0.0f, 0.0f);
            m_mainShader.Set3f("uAmbient", 0.06f, 0.06f, 0.09f);
        } else {
            m_mainShader.Set3f("uLightColor", m_actDiffuse[0], m_actDiffuse[1], m_actDiffuse[2]);
            m_mainShader.Set3f("uAmbient",    m_actAmbient[0]*0.7f, m_actAmbient[1]*0.7f, m_actAmbient[2]*0.7f);
        }
        m_mainShader.Set1f("uShininess", 48.0f);
        m_mainShader.Set1f("uSpecStrength", 0.45f);
        m_mainShader.Set1f("uShadowMapTexel", 1.0f / (float)m_shadowMap.Size());
        m_mainShader.Set1i("uReceiveShadow", 1);
        m_mainShader.Set1i("uLightEnabled", 1);
        m_mainShader.Set1i("uHasTexture", 0);

        m_mainShader.Set1i("uFlashOn", m_isFlashlightOn ? 1 : 0);
        if (m_isFlashlightOn) {
            float fpv[3];
            fpv[0] = viewMtx[0]*flashPosWorld[0] + viewMtx[4]*flashPosWorld[1] + viewMtx[8] *flashPosWorld[2] + viewMtx[12];
            fpv[1] = viewMtx[1]*flashPosWorld[0] + viewMtx[5]*flashPosWorld[1] + viewMtx[9] *flashPosWorld[2] + viewMtx[13];
            fpv[2] = viewMtx[2]*flashPosWorld[0] + viewMtx[6]*flashPosWorld[1] + viewMtx[10]*flashPosWorld[2] + viewMtx[14];
            float fdv[3];
            fdv[0] = viewMtx[0]*flashDirWorld[0] + viewMtx[4]*flashDirWorld[1] + viewMtx[8] *flashDirWorld[2];
            fdv[1] = viewMtx[1]*flashDirWorld[0] + viewMtx[5]*flashDirWorld[1] + viewMtx[9] *flashDirWorld[2];
            fdv[2] = viewMtx[2]*flashDirWorld[0] + viewMtx[6]*flashDirWorld[1] + viewMtx[10]*flashDirWorld[2];
            float fdl = sqrtf(fdv[0]*fdv[0] + fdv[1]*fdv[1] + fdv[2]*fdv[2]);
            if (fdl > 0.0001f) { fdv[0]/=fdl; fdv[1]/=fdl; fdv[2]/=fdl; }

            m_mainShader.Set3f("uFlashPosView", fpv[0], fpv[1], fpv[2]);
            m_mainShader.Set3f("uFlashDirView", fdv[0], fdv[1], fdv[2]);
            m_mainShader.Set3f("uFlashColor",   1.6f, 1.5f, 1.2f);
            m_mainShader.Set1f("uFlashCosInner", 0.965f);
            m_mainShader.Set1f("uFlashCosOuter", 0.875f);
            m_mainShader.Set1f("uFlashRange",    35.0f);
        }

        glEnable(GL_DEPTH_TEST);
        DrawTable();
        DrawGameObjects(false);

        Shader::Unuse();
    } else {
        glEnable(GL_LIGHTING);
        glLightfv(GL_LIGHT0, GL_POSITION, currentLightPos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, m_actDiffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, m_actAmbient);
        DrawTable();
        DrawGameObjects(false);
    }

    DrawHUD();
}

GLuint CMagicCupGameView::LoadTexture(const char* filename)
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
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glLoadIdentity();

    float radYaw = m_camYaw * M_PI / 180.0f;
    float radPitch = m_camPitch * M_PI / 180.0f;

    float lookX = cos(radPitch) * cos(radYaw);
    float lookY = sin(radPitch);
    float lookZ = cos(radPitch) * sin(radYaw);

    gluLookAt(0.0, 0.0, 0.0,
        lookX, lookY, lookZ,
        0.0, 1.0, 0.0);

    glColor3f(1.0f, 1.0f, 1.0f);

    float radius = 50.0f;
    int stacks = 32;
    int slices = 32;

    for (int i = 0; i < stacks; ++i) {
        float lat0 = M_PI * (-0.5f + (float)(i) / stacks);
        float z0 = sin(lat0);
        float r0 = cos(lat0);

        float lat1 = M_PI * (-0.5f + (float)(i + 1) / stacks);
        float z1 = sin(lat1);
        float r1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * M_PI * (float)(j) / slices;
            float x = cos(lng);
            float y = sin(lng);

            float vx0 = x * r0;
            float vy0 = z0;
            float vz0 = y * r0;

            glTexCoord3f(vx0, vy0, vz0);
            glVertex3f(vx0 * radius, vy0 * radius, vz0 * radius);

            float vx1 = x * r1;
            float vy1 = z1;
            float vz1 = y * r1;

            glTexCoord3f(vx1, vy1, vz1);
            glVertex3f(vx1 * radius, vy1 * radius, vz1 * radius);
        }
        glEnd();
    }

    glPopMatrix();
    glDisable(GL_TEXTURE_CUBE_MAP);
    glPopAttrib();
}

void CMagicCupGameView::DrawGameObjects(bool bShadow)
{
    for (int i = 0; i < 3; i++) {
        bool selected = (m_selectedCup == i);
        bool hovered = (m_gameState == GAME_GUESSING && m_hoveredCup == i);

        DrawCupWithDetails(m_cupPositions[i][0], m_cupPositions[i][1] + m_cupHeights[i],
            m_cupPositions[i][2], 0.0f, selected || hovered, bShadow);
    }

    if (m_gameState != GAME_GUESSING && m_gameState != GAME_SHUFFLING) {
        float ballY = -0.6f;

        DrawGradientBall(m_cupPositions[m_ballPosition][0], ballY,
            m_cupPositions[m_ballPosition][2], 0.4f, bShadow);
    }
}

void CMagicCupGameView::DrawRoundedCorner(float cx, float cz, float y, float radius, int segments, float startAngle, float totalW, float totalD)
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

    float u = (cx + totalW / 2.0f) / totalW;
    float v = (cz + totalD / 2.0f) / totalD;
    glTexCoord2f(u, v);
    glVertex3f(cx, y, cz);

    for (int i = 0; i <= segments; i++)
    {
        float angle = startAngle + (float)i / (float)segments * (M_PI / 2.0f);
        float x = cx + cos(angle) * radius;
        float z = cz + sin(angle) * radius;

        u = (x + totalW / 2.0f) / totalW;
        v = (z + totalD / 2.0f) / totalD;
        glTexCoord2f(u, v);
        glVertex3f(x, y, z);
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
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        if (m_shadersReady && m_mainShader.IsValid()) m_mainShader.Set1i("uHasTexture", 1);
    }

    auto TexCoord = [&](float x, float z) {
        float u = (x + halfW) / width;
        float v = (z + halfD) / depth;
        glTexCoord2f(u, v);
        };

    glColor3f(1, 1, 1);
    glNormal3f(0, 1, 0);

    glBegin(GL_QUADS);
    TexCoord(-halfW + radius, -halfD + radius); glVertex3f(-halfW + radius, yTop, -halfD + radius);
    TexCoord(halfW - radius, -halfD + radius); glVertex3f(halfW - radius, yTop, -halfD + radius);
    TexCoord(halfW - radius, halfD - radius); glVertex3f(halfW - radius, yTop, halfD - radius);
    TexCoord(-halfW + radius, halfD - radius); glVertex3f(-halfW + radius, yTop, halfD - radius);
    glEnd();

    glBegin(GL_QUADS);
    TexCoord(-halfW, -halfD + radius); glVertex3f(-halfW, yTop, -halfD + radius);
    TexCoord(-halfW + radius, -halfD + radius); glVertex3f(-halfW + radius, yTop, -halfD + radius);
    TexCoord(-halfW + radius, halfD - radius); glVertex3f(-halfW + radius, yTop, halfD - radius);
    TexCoord(-halfW, halfD - radius); glVertex3f(-halfW, yTop, halfD - radius);
    glEnd();

    glBegin(GL_QUADS);
    TexCoord(halfW - radius, -halfD + radius); glVertex3f(halfW - radius, yTop, -halfD + radius);
    TexCoord(halfW, -halfD + radius); glVertex3f(halfW, yTop, -halfD + radius);
    TexCoord(halfW, halfD - radius); glVertex3f(halfW, yTop, halfD - radius);
    TexCoord(halfW - radius, halfD - radius); glVertex3f(halfW - radius, yTop, halfD - radius);
    glEnd();

    glBegin(GL_QUADS);
    TexCoord(-halfW + radius, -halfD); glVertex3f(-halfW + radius, yTop, -halfD);
    TexCoord(halfW - radius, -halfD); glVertex3f(halfW - radius, yTop, -halfD);
    TexCoord(halfW - radius, -halfD + radius); glVertex3f(halfW - radius, yTop, -halfD + radius);
    TexCoord(-halfW + radius, -halfD + radius); glVertex3f(-halfW + radius, yTop, -halfD + radius);
    glEnd();

    glBegin(GL_QUADS);
    TexCoord(-halfW + radius, halfD - radius); glVertex3f(-halfW + radius, yTop, halfD - radius);
    TexCoord(halfW - radius, halfD - radius); glVertex3f(halfW - radius, yTop, halfD - radius);
    TexCoord(halfW - radius, halfD); glVertex3f(halfW - radius, yTop, halfD);
    TexCoord(-halfW + radius, halfD); glVertex3f(-halfW + radius, yTop, halfD);
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

    DrawRoundedCorner(halfW - radius, halfD - radius, yTop, radius, segments, 0, width, depth);
    DrawRoundedCorner(-halfW + radius, halfD - radius, yTop, radius, segments, M_PI / 2, width, depth);
    DrawRoundedCorner(-halfW + radius, -halfD + radius, yTop, radius, segments, M_PI, width, depth);
    DrawRoundedCorner(halfW - radius, -halfD + radius, yTop, radius, segments, 3 * M_PI / 2, width, depth);

    if (textureID != 0)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        if (m_shadersReady && m_mainShader.IsValid()) m_mainShader.Set1i("uHasTexture", 0);
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
    if (!m_gameSettings.tableEnabled) return;

    float width = 20.0f;
    float depth = 12.0f;
    float height = 0.5f;
    float radius = 1.0f;
    int segments = 16;
    float yTop = -1.0f;
    DrawTableTop(width, depth, height, radius, segments, tableTexture, yTop);
    DrawTableLegs(yTop, height);

    if (m_gameSettings.carpetEnabled && carpetTexture != 0)
    {
        float carpetW = width * 0.8f;
        float carpetD = depth * 0.8f;
        float halfCW = carpetW / 2.0f;
        float halfCD = carpetD / 2.0f;
        float yCarpet = yTop + 0.01f;

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, carpetTexture);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor3f(1.0f, 1.0f, 1.0f);
        glNormal3f(0.0f, 1.0f, 0.0f);

        if (m_shadersReady && m_mainShader.IsValid()) m_mainShader.Set1i("uHasTexture", 1);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-halfCW, yCarpet, -halfCD);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(halfCW, yCarpet, -halfCD);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(halfCW, yCarpet, halfCD);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-halfCW, yCarpet, halfCD);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        if (m_shadersReady && m_mainShader.IsValid()) m_mainShader.Set1i("uHasTexture", 0);
    }
}

void CMagicCupGameView::DrawCupWithDetails(float x, float y, float z, float height, bool highlight, bool bShadow)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    if (!bShadow) {
        if (highlight) {
            glColor3f(0.8f, 0.2f, 0.2f);
        }
        else {
            float r = GetRValue(m_gameSettings.cupColor) / 255.0f;
            float g = GetGValue(m_gameSettings.cupColor) / 255.0f;
            float b = GetBValue(m_gameSettings.cupColor) / 255.0f;
            glColor3f(r, g, b);
        }
    }

    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);

    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    glRotatef(-180, 0, 1, 0);
    gluCylinder(quadric, 0.6f, 0.9f, 1.5f, 20, 4);
    glPopMatrix();

    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluDisk(quadric, 0, 0.6f, 20, 1);
    glPopMatrix();

    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void CMagicCupGameView::DrawGradientBall(float x, float y, float z, float radius, bool bShadow)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    if (!bShadow) {
        float r = GetRValue(m_gameSettings.ballColor) / 255.0f;
        float g = GetGValue(m_gameSettings.ballColor) / 255.0f;
        float b = GetBValue(m_gameSettings.ballColor) / 255.0f;
        glColor3f(r, g, b);

        GLfloat mat_ambient_diffuse[] = { r, g, b, 1.0f };
        GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat mat_shininess[] = { 50.0f };

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_ambient_diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    }

    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricNormals(quadric, GLU_SMOOTH);

    gluSphere(quadric, radius, 20, 20);

    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void CMagicCupGameView::DrawStringTexture(int x, int y, const char* text, const FontConfig& font, COLORREF color)
{
    if (!text || strlen(text) == 0) return;

    HDC hDC = ::GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hDC);

    CT2A asciiFont(font.fontName);

    HFONT hFont = CreateFontA(
        -font.fontSize, 0, 0, 0,
        font.fontWeight,
        font.isItalic,
        font.isUnderline,
        font.isStrikeOut,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        VARIABLE_PITCH, asciiFont);

    HGDIOBJ hOldFont = SelectObject(hMemDC, hFont);

    SIZE size;
    GetTextExtentPoint32A(hMemDC, text, (int)strlen(text), &size);

    int extraBorder = font.useStroke ? font.strokeThickness * 2 : 0;
    size.cx += extraBorder;
    size.cy += extraBorder;

    if (size.cx % 2 != 0) size.cx++;
    if (size.cy % 2 != 0) size.cy++;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size.cx;
    bmi.bmiHeader.biHeight = -size.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);

    SetBkMode(hMemDC, TRANSPARENT);

    RECT r = { 0, 0, size.cx, size.cy };
    FillRect(hMemDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));

    int offset = font.useStroke ? font.strokeThickness : 0;

    if (font.useStroke) {
        SetTextColor(hMemDC, font.strokeColor);
        for (int dx = -font.strokeThickness; dx <= font.strokeThickness; dx++) {
            for (int dy = -font.strokeThickness; dy <= font.strokeThickness; dy++) {
                if (dx == 0 && dy == 0) continue;
                TextOutA(hMemDC, offset + dx, offset + dy, text, (int)strlen(text));
            }
        }
    }

    SetTextColor(hMemDC, color);
    TextOutA(hMemDC, offset, offset, text, (int)strlen(text));

    unsigned char* pixels = (unsigned char*)pBits;
    int pixelCount = size.cx * size.cy;
    for (int i = 0; i < pixelCount; i++) {
        unsigned char b = pixels[i * 4 + 0];
        unsigned char g = pixels[i * 4 + 1];
        unsigned char r = pixels[i * 4 + 2];
        unsigned char alpha = max(r, max(g, b));

        pixels[i * 4 + 3] = alpha;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.cx, size.cy, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f((float)x, (float)y + size.cy);
    glTexCoord2f(1.0f, 0.0f); glVertex2f((float)x + size.cx, (float)y + size.cy);
    glTexCoord2f(1.0f, 1.0f); glVertex2f((float)x + size.cx, (float)y);
    glTexCoord2f(0.0f, 1.0f); glVertex2f((float)x, (float)y);
    glEnd();

    glDeleteTextures(1, &texID);
    glDisable(GL_TEXTURE_2D);

    SelectObject(hMemDC, hOldBitmap);
    SelectObject(hMemDC, hOldFont);
    DeleteObject(hBitmap);
    DeleteObject(hFont);
    DeleteDC(hMemDC);
    ::ReleaseDC(NULL, hDC);
}

void CMagicCupGameView::HUDPrint(int x, int y, const char* text, const FontConfig& font, COLORREF color)
{
    DrawStringTexture(x, y, text, font, color);
}

int CMagicCupGameView::TextWidth(const char* text, const FontConfig& font)
{
    if (text == NULL || strlen(text) == 0) return 0;

    HDC hDC = ::GetDC(NULL);

    CT2A asciiFont(font.fontName);

    HFONT hFont = CreateFontA(-font.fontSize, 0, 0, 0,
        font.fontWeight,
        font.isItalic,
        font.isUnderline,
        font.isStrikeOut,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, VARIABLE_PITCH, asciiFont);
    HGDIOBJ hOld = SelectObject(hDC, hFont);

    SIZE size;
    GetTextExtentPoint32A(hDC, text, (int)strlen(text), &size);

    int extra = font.useStroke ? font.strokeThickness * 2 : 0;

    SelectObject(hDC, hOld);
    DeleteObject(hFont);
    ::ReleaseDC(NULL, hDC);

    return size.cx + extra;
}

void CMagicCupGameView::DrawHUD()
{
    const int marginTop = 20;
    const int lineSpacing = 5;

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

    if (m_gameSettings.crosshairEnabled && m_rmbDown) {
        int cx = m_windowWidth / 2;
        int cy = m_windowHeight / 2;

        if (m_gameSettings.crosshairBorderEnabled) {
            glLineWidth((float)m_gameSettings.crosshairBorderThickness + 2.0f);
            float r = GetRValue(m_gameSettings.crosshairBorderColor) / 255.0f;
            float g = GetGValue(m_gameSettings.crosshairBorderColor) / 255.0f;
            float b = GetBValue(m_gameSettings.crosshairBorderColor) / 255.0f;
            glColor3f(r, g, b);

            glBegin(GL_LINES);
            glVertex2f(cx - 10 - m_gameSettings.crosshairBorderThickness / 2, cy); glVertex2f(cx + 10 + m_gameSettings.crosshairBorderThickness / 2, cy);
            glVertex2f(cx, cy - 10 - m_gameSettings.crosshairBorderThickness / 2); glVertex2f(cx, cy + 10 + m_gameSettings.crosshairBorderThickness / 2);
            glEnd();
        }

        glLineWidth(2.0f);
        float r = GetRValue(m_gameSettings.crosshairColor) / 255.0f;
        float g = GetGValue(m_gameSettings.crosshairColor) / 255.0f;
        float b = GetBValue(m_gameSettings.crosshairColor) / 255.0f;
        glColor3f(r, g, b);

        glBegin(GL_LINES);
        glVertex2f(cx - 10, cy); glVertex2f(cx + 10, cy);
        glVertex2f(cx, cy - 10); glVertex2f(cx, cy + 10);
        glEnd();
    }

    const char* lines[5];
    int lineCount = 0;
    char buf1[256], buf2[256], buf3[256], bufDiff[256];

    switch (m_gameState) {
    case GAME_NONE:          sprintf_s(buf1, "Статус: Нема"); break;
    case GAME_WAITING:       sprintf_s(buf1, "Статус: Очікування"); break;
    case GAME_LIFTING_CUPS:  sprintf_s(buf1, "Статус: Підйом..."); break;
    case GAME_SHOWING_BALL:  sprintf_s(buf1, "Статус: Запам'ятай!"); break;
    case GAME_LOWERING_CUPS: sprintf_s(buf1, "Статус: Готовий?"); break;
    case GAME_SHUFFLING:     sprintf_s(buf1, "Статус: Стеж за чашками!"); break;
    case GAME_GUESSING:      sprintf_s(buf1, "Статус: Тут куля?"); break;
    case GAME_RESULT:        sprintf_s(buf1, "Статус: Фінал"); break;
    default:                 sprintf_s(buf1, "Статус: ?"); break;
    }
    lines[lineCount++] = buf1;

    sprintf_s(bufDiff, "[ %s ]", (LPCSTR)CT2A(m_selectedLevel.name));
    lines[lineCount++] = bufDiff;

    bool ableToGame = (m_gameState == GAME_NONE || m_gameState == GAME_WAITING);

    if (m_gameState == GAME_SHUFFLING) {
        sprintf_s(buf2, "Перемішування: %d / %d", m_shuffleStep + 1, m_selectedLevel.shuffleCount);
        lines[lineCount++] = buf2;
    }
    else if (ableToGame && m_selectedCup <= 0) {
        sprintf_s(buf2, "ENTER - Старт");
        lines[lineCount++] = buf2;
    }

    COLORREF resultColor = RGB(255, 255, 255);

    if ((m_gameState == GAME_RESULT || m_gameState == GAME_NONE) && m_selectedCup >= 0) {
        if (m_gameWon) {
            sprintf_s(buf3, "*** Перемога! ***");
            resultColor = RGB(50, 255, 50);
        }
        else {
            int ballPosIndex = 2;
            float ballX = m_cupPositions[m_ballPosition][0];
            if (ballX < -1.0f) ballPosIndex = 1;
            else if (ballX > 1.0f) ballPosIndex = 3;

            sprintf_s(buf3, "Поразка (Кулька: %d)", ballPosIndex);
            resultColor = RGB(255, 80, 80);
        }
        lines[lineCount++] = buf3;
    }

    int maxTextW = 0;
    for (int i = 0; i < lineCount; ++i)
        maxTextW = max(maxTextW, TextWidth(lines[i], m_gameSettings.topMenuFont));

    int lineHeight = m_gameSettings.topMenuFont.fontSize + lineSpacing;
    int panelW = maxTextW + (marginTop * 2);
    int panelH = (lineCount * lineHeight) + (marginTop * 2);

    int centerX = m_windowWidth / 2;
    int panelLeft = centerX - (panelW / 2);
    int panelRight = panelLeft + panelW;
    int panelTop = m_windowHeight - 20;
    int panelBottom = panelTop - panelH;

    float br = GetRValue(m_gameSettings.hudRectColor) / 255.0f;
    float bg = GetGValue(m_gameSettings.hudRectColor) / 255.0f;
    float bb = GetBValue(m_gameSettings.hudRectColor) / 255.0f;

    glBegin(GL_QUADS);
    glColor4f(br, bg, bb, 0.9f);
    glVertex2f(panelLeft, panelTop);
    glVertex2f(panelRight, panelTop);
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glVertex2f(panelRight, panelBottom);
    glVertex2f(panelLeft, panelBottom);
    glEnd();

    if (m_gameSettings.hudBorderEnabled) {
        glLineWidth(2.0f);
        float borR = GetRValue(m_gameSettings.hudBorderColor) / 255.0f;
        float borG = GetGValue(m_gameSettings.hudBorderColor) / 255.0f;
        float borB = GetBValue(m_gameSettings.hudBorderColor) / 255.0f;
        glColor4f(borR, borG, borB, 0.8f);

        glBegin(GL_LINE_LOOP);
        glVertex2f(panelLeft, panelTop);
        glVertex2f(panelRight, panelTop);
        glVertex2f(panelRight, panelBottom);
        glVertex2f(panelLeft, panelBottom);
        glEnd();
    }

    for (int i = 0; i < lineCount; ++i) {
        int textW = TextWidth(lines[i], m_gameSettings.topMenuFont);
        int x = centerX - (textW / 2);
        int y = panelTop - marginTop - ((i + 1) * lineHeight);

        COLORREF textColor = RGB(255, 255, 255);
        if (i == 0) textColor = RGB(255, 230, 0);
        else if (i == lineCount - 1 && (m_gameState == GAME_RESULT || m_gameState == GAME_NONE)) {
            textColor = resultColor;
        }

        HUDPrint(x, y, lines[i], m_gameSettings.topMenuFont, textColor);
    }

    struct KeyInfo {
        const char* keyName;
        const char* actionName;
        int vkCode;
    };

    KeyInfo leftKeys[] = {
        { "W", "Вперед", 'W' },
        { "A", "Ліво", 'A' },
        { "S", "Назад", 'S' },
        { "D", "Вправо", 'D' }
    };

    KeyInfo rightKeys[] = {
        { "CTRL", "Униз", VK_CONTROL },
        { "SPACE", "Угору", VK_SPACE },
        { "LMB", "Обрати", VK_LBUTTON },
        { "RMB", "Огляд", VK_RBUTTON }
    };

    KeyInfo centerKeys[] = {
        { "F", "Ліхтар", 'F' },
        { "R", "Відновити", 'R' }
    };

    const int marginBtn = 10;
    const int btnGap = 15;
    int minBtnSize = 40;
    int btnHeight = max(m_gameSettings.buttonFont.fontSize + (marginBtn * 2), minBtnSize);
    int btnBottomY = marginBtn;
    int btnTopY = btnBottomY + btnHeight;
    int actionTextY = btnTopY + 5;

    int currentX = marginBtn;

    for (int i = 0; i < 4; i++) {
        const char* key = leftKeys[i].keyName;
        const char* action = leftKeys[i].actionName;
        int code = leftKeys[i].vkCode;

        int keyTextW = TextWidth(key, m_gameSettings.buttonFont);
        int btnW = max(keyTextW + (marginBtn * 2), minBtnSize);

        int L = currentX;
        int R = L + btnW;
        int B = btnBottomY;
        int T = btnTopY;

        bool isActive = ((GetAsyncKeyState(code) & 0x8000) != 0);

        glBegin(GL_QUADS);
        if (isActive) glColor4f(0.3f, 0.3f, 0.45f, 0.9f);
        else              glColor4f(br, bg, bb, 0.9f);
        glVertex2f(L, T); glVertex2f(R, T);
        if (isActive) glColor4f(0.2f, 0.2f, 0.2f, 0.6f);
        else          glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
        glVertex2f(R, B); glVertex2f(L, B);
        glEnd();

        if (m_gameSettings.hudBorderEnabled) {
            glLineWidth(2.0f);
            float borR = GetRValue(m_gameSettings.hudBorderColor) / 255.0f;
            float borG = GetGValue(m_gameSettings.hudBorderColor) / 255.0f;
            float borB = GetBValue(m_gameSettings.hudBorderColor) / 255.0f;
            glColor4f(borR, borG, borB, 0.8f);

            glBegin(GL_LINE_LOOP);
            glVertex2f(L, T); glVertex2f(R, T);
            glVertex2f(R, B); glVertex2f(L, B);
            glEnd();
        }

        HUDPrint(L + (btnW - keyTextW) / 2, B + (btnHeight - m_gameSettings.buttonFont.fontSize) / 2, key, m_gameSettings.buttonFont, RGB(255, 255, 255));

        int actionW = TextWidth(action, m_gameSettings.actionFont);
        int actionX = L + (btnW - actionW) / 2;

        HUDPrint(actionX, actionTextY, action, m_gameSettings.actionFont, RGB(255, 255, 255));

        currentX += btnW + btnGap;
    }

    currentX = m_windowWidth - marginBtn;

    for (int i = 3; i >= 0; i--) {
        const char* key = rightKeys[i].keyName;
        const char* action = rightKeys[i].actionName;
        int code = rightKeys[i].vkCode;

        int keyTextW = TextWidth(key, m_gameSettings.buttonFont);
        int btnW = max(keyTextW + (marginBtn * 2), minBtnSize);

        int R = currentX;
        int L = R - btnW;
        int B = btnBottomY;
        int T = btnTopY;

        bool isActive = ((GetAsyncKeyState(code) & 0x8000) != 0);

        glBegin(GL_QUADS);
        if (isActive) glColor4f(0.3f, 0.3f, 0.45f, 0.9f);
        else              glColor4f(br, bg, bb, 0.9f);
        glVertex2f(L, T); glVertex2f(R, T);
        if (isActive) glColor4f(0.2f, 0.2f, 0.2f, 0.6f);
        else          glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
        glVertex2f(R, B); glVertex2f(L, B);
        glEnd();

        if (m_gameSettings.hudBorderEnabled) {
            glLineWidth(2.0f);
            float borR = GetRValue(m_gameSettings.hudBorderColor) / 255.0f;
            float borG = GetGValue(m_gameSettings.hudBorderColor) / 255.0f;
            float borB = GetBValue(m_gameSettings.hudBorderColor) / 255.0f;
            glColor4f(borR, borG, borB, 0.8f);

            glBegin(GL_LINE_LOOP);
            glVertex2f(L, T); glVertex2f(R, T);
            glVertex2f(R, B); glVertex2f(L, B);
            glEnd();
        }

        HUDPrint(L + (btnW - keyTextW) / 2, B + (btnHeight - m_gameSettings.buttonFont.fontSize) / 2, key, m_gameSettings.buttonFont, RGB(255, 255, 255));

        int actionW = TextWidth(action, m_gameSettings.actionFont);
        int actionX = L + (btnW - actionW) / 2;

        HUDPrint(actionX, actionTextY, action, m_gameSettings.actionFont, RGB(255, 255, 255));

        currentX -= (btnW + btnGap);
    }

    int totalCenterW = 0;

    for (int i = 0; i < 2; i++) {
        int keyTextW = TextWidth(centerKeys[i].keyName, m_gameSettings.buttonFont);
        int btnW = max(keyTextW + (marginBtn * 2), minBtnSize);
        totalCenterW += btnW;
    }

    totalCenterW += btnGap;

    currentX = (m_windowWidth / 2) - (totalCenterW / 2);

    for (int i = 0; i < 2; i++) {
        const char* key = centerKeys[i].keyName;
        const char* action = centerKeys[i].actionName;
        int code = centerKeys[i].vkCode;

        int keyTextW = TextWidth(key, m_gameSettings.buttonFont);
        int btnW = max(keyTextW + (marginBtn * 2), minBtnSize);

        int L = currentX;
        int R = L + btnW;
        int B = btnBottomY;
        int T = btnTopY;

        bool isActive = false;
        if (code == 'F') isActive = m_isFlashlightOn;
        else isActive = ((GetAsyncKeyState(code) & 0x8000) != 0);

        glBegin(GL_QUADS);
        if (isActive) glColor4f(0.3f, 0.3f, 0.45f, 0.9f);
        else          glColor4f(br, bg, bb, 0.9f);
        glVertex2f(L, T); glVertex2f(R, T);

        if (isActive) glColor4f(0.2f, 0.2f, 0.2f, 0.6f);
        else          glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
        glVertex2f(R, B); glVertex2f(L, B);
        glEnd();

        if (m_gameSettings.hudBorderEnabled) {
            glLineWidth(2.0f);
            float borR = GetRValue(m_gameSettings.hudBorderColor) / 255.0f;
            float borG = GetGValue(m_gameSettings.hudBorderColor) / 255.0f;
            float borB = GetBValue(m_gameSettings.hudBorderColor) / 255.0f;
            glColor4f(borR, borG, borB, 0.8f);

            glBegin(GL_LINE_LOOP);
            glVertex2f(L, T); glVertex2f(R, T);
            glVertex2f(R, B); glVertex2f(L, B);
            glEnd();
        }

        HUDPrint(L + (btnW - keyTextW) / 2, B + (btnHeight - m_gameSettings.buttonFont.fontSize) / 2, key, m_gameSettings.buttonFont, RGB(255, 255, 255));

        int actionW = TextWidth(action, m_gameSettings.actionFont);
        int actionX = L + (btnW - actionW) / 2;

        HUDPrint(actionX, actionTextY, action, m_gameSettings.actionFont, RGB(255, 255, 255));

        currentX += btnW + btnGap;
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
    UpdateCameraMovement();

    DWORD currentTime = GetTickCount();
    if (currentTime - m_lastDBUpdate > 1000) {
        UpdateDifficultyFromDB();
        m_lastDBUpdate = currentTime;
    }

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
                    int realBallPos = 2;
                    float ballX = m_cupPositions[m_ballPosition][0];
                    if (ballX < -1.0f) realBallPos = 1;
                    else if (ballX > 1.0f) realBallPos = 3;

                    int realUserPos = 2;
                    float userX = m_cupPositions[m_selectedCup][0];
                    if (userX < -1.0f) realUserPos = 1;
                    else if (userX > 1.0f) realUserPos = 3;

                    DBHelper::GetInstance().AddGameSession(
                        currentUserId,
                        m_selectedLevel.id,
                        realBallPos,
                        realUserPos,
                        m_gameWon
                    );
                }

                m_isAnimating = false;
                m_stateStartTime = GetTickCount();
            }
        }
        else
        {
            if (currentTime - m_stateStartTime > 3000) {
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

void CMagicCupGameView::UpdateCameraMovement()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    float dt = (float)(currentTime.QuadPart - m_qpLastTime.QuadPart) /
        (float)m_qpFrequency.QuadPart;
    m_qpLastTime = currentTime;

    if (dt > 0.033f) dt = 0.033f;
    if (dt < 0.0f) dt = 0.0f;

    const float moveSpeed = m_keyShift ? 50.0f : 20.0f;

    float radYaw = m_camYaw * (float)M_PI / 180.0f;
    float radPitch = m_camPitch * (float)M_PI / 180.0f;

    float dirX = cos(radPitch) * cos(radYaw);
    float dirZ = cos(radPitch) * sin(radYaw);
    float rightX = -sin(radYaw);
    float rightZ = cos(radYaw);

    float tX = 0, tY = 0, tZ = 0;

    if (m_keyW) { tX += dirX; tZ += dirZ; }
    if (m_keyS) { tX -= dirX; tZ -= dirZ; }
    if (m_keyA) { tX -= rightX; tZ -= rightZ; }
    if (m_keyD) { tX += rightX; tZ += rightZ; }
    if (m_keySpace) tY += 1.0f;
    if (m_keyCtrl)  tY -= 1.0f;

    float len = sqrt(tX * tX + tY * tY + tZ * tZ);
    if (len > 0.0001f) {
        tX /= len; tY /= len; tZ /= len;
        tX *= moveSpeed;
        tY *= moveSpeed;
        tZ *= moveSpeed;
    }

    float smoothing = powf(0.0001f, dt);

    m_velX = m_velX * smoothing + tX * (1 - smoothing);
    m_velY = m_velY * smoothing + tY * (1 - smoothing);
    m_velZ = m_velZ * smoothing + tZ * (1 - smoothing);

    m_camX += m_velX * dt;
    m_camY += m_velY * dt;
    m_camZ += m_velZ * dt;

    const float box = 38.0f;

    auto softLimit = [&](float& pos, float& vel, float minVal, float maxVal)
        {
            if (pos > maxVal) {
                float over = pos - maxVal;
                pos -= over * 0.5f;
                vel *= -0.2f;
            }
            if (pos < minVal) {
                float over = minVal - pos;
                pos += over * 0.5f;
                vel *= -0.2f;
            }
        };

    softLimit(m_camX, m_velX, -box, box);
    softLimit(m_camZ, m_velZ, -box, box);
    softLimit(m_camY, m_velY, 1.0f, box);
}


void CMagicCupGameView::UpdateDifficultyFromDB()
{
    std::vector<DifficultyLevel> levels = DBHelper::GetInstance().GetDifficultyLevels();

    bool foundCurrent = false;

    for (const auto& lvl : levels) {
        if (lvl.id == m_selectedLevel.id) {
            m_selectedLevel.name = lvl.name;
            m_selectedLevel.shuffleCount = lvl.shuffleCount;
            m_selectedLevel.animationSpeed = lvl.animationSpeed;
            foundCurrent = true;
            break;
        }
    }

    if (!foundCurrent && !levels.empty()) {
        m_selectedLevel = levels[0];
    }
}

// Печать CMagicCupGameView

BOOL CMagicCupGameView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // Реализация по умолчанию
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

CMagicCupGameDoc* CMagicCupGameView::GetDocument() const // встроена отладочная версия
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMagicCupGameDoc)));
    return (CMagicCupGameDoc*)m_pDocument;
}
#endif //_DEBUG


// Обработчики сообщений CMagicCupGameView

void CMagicCupGameView::OnSettings()
{
    CSettingsDlg dlg;
    dlg.m_nInitialId = m_selectedLevel.id;
    dlg.m_nInitialSkybox = m_currentSkyboxIndex;
    dlg.m_nInitialTable = m_currentTableMatIndex;
    dlg.m_nInitialCarpet = m_currentCarpetMatIndex;

    for (const auto& preset : m_skyboxPresets) {
        dlg.m_skyboxNames.push_back(preset.name);
    }

    for (const auto& mat : m_tableMats) {
        dlg.m_tableNames.push_back(mat.name);
    }
    for (const auto& mat : m_carpetMats) {
        dlg.m_carpetNames.push_back(mat.name);
    }

    dlg.m_bDisabledDifficulty = (this->m_gameState != GAME_NONE);

    if (dlg.DoModal() == IDOK) {
        m_selectedLevel = dlg.m_selectedLevel;
        m_gameSettings = dlg.m_gameSettings;
        m_gameSettings.skyboxIndex = dlg.m_selectedSkyboxIndex;

        if (dlg.m_selectedSkyboxIndex != m_currentSkyboxIndex) {
            SelectSkybox(dlg.m_selectedSkyboxIndex);
        }

        if (dlg.m_selectedTableIndex != m_currentTableMatIndex) {
            SelectTable(dlg.m_selectedTableIndex);
        }

        if (dlg.m_selectedCarpetIndex != m_currentCarpetMatIndex) {
            SelectCarpet(dlg.m_selectedCarpetIndex);
        }

        CMagicCupGameApp* pApp = (CMagicCupGameApp*)AfxGetApp();
        int currentUserId = pApp->m_nCurrentUserID;

        if (currentUserId != -1) {
            if (this->m_gameState != GAME_NONE) {
                DBHelper::GetInstance().SaveUserDifficulty(currentUserId, m_selectedLevel.id);
            }

            DBHelper::GetInstance().SaveGameSettings(currentUserId, m_gameSettings);
        }

        CString str;
        str.Format(_T("Налаштування збережено!"),
            m_selectedLevel.name,
            m_skyboxPresets[dlg.m_selectedSkyboxIndex].name);
        AfxMessageBox(str);
    }
}

void CMagicCupGameView::OnAdmin()
{
    CAdminDlg dlg;
    dlg.DoModal();
}

void CMagicCupGameView::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_gameState == GAME_GUESSING) {
        int cupIndex = GetCupIndexFromScreenPos(point);
        if (cupIndex != -1) {
            OnCupClick(cupIndex);
        }
    }

    CView::OnLButtonDown(nFlags, point);
}

void CMagicCupGameView::OnRButtonDown(UINT nFlags, CPoint point)
{
    m_rmbDown = true;
    m_lastMousePos = point;
    SetCapture();
    ShowCursor(FALSE);
    CView::OnRButtonDown(nFlags, point);
}

void CMagicCupGameView::OnRButtonUp(UINT nFlags, CPoint point)
{
    m_rmbDown = false;
    ReleaseCapture();
    ShowCursor(TRUE);
    CView::OnRButtonUp(nFlags, point);
}

void CMagicCupGameView::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_rmbDown) {
        float sensitivity = 0.1f;
        float deltaX = (float)(point.x - m_lastMousePos.x);
        float deltaY = (float)(point.y - m_lastMousePos.y);

        m_camYaw += deltaX * sensitivity;
        m_camPitch -= deltaY * sensitivity;

        if (m_camPitch > 89.0f) m_camPitch = 89.0f;
        if (m_camPitch < -89.0f) m_camPitch = -89.0f;

        m_lastMousePos = point;

        CPoint center(m_windowWidth / 2, m_windowHeight / 2);
        ClientToScreen(&center);
        SetCursorPos(center.x, center.y);
        ScreenToClient(&center);
        m_lastMousePos = center;
    }

    int cupIndex = GetCupIndexFromScreenPos(point);
    m_hoveredCup = cupIndex;

    if (!m_rmbDown) {
        CView::OnMouseMove(nFlags, point);
    }
}

void CMagicCupGameView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar) {
    case 'W': m_keyW = true; break;
    case 'S': m_keyS = true; break;
    case 'A': m_keyA = true; break;
    case 'D': m_keyD = true; break;
    case VK_SPACE: m_keySpace = true; break;
    case VK_CONTROL: m_keyCtrl = true; break;
    case VK_SHIFT: m_keyShift = true; break;
    case 'F':
        m_isFlashlightOn = !m_isFlashlightOn;
        Invalidate(FALSE);
        break;
    case 'R':
        m_camX = DEF_CAM_X;
        m_camY = DEF_CAM_Y;
        m_camZ = DEF_CAM_Z;
        m_camYaw = DEF_CAM_YAW;
        m_camPitch = DEF_CAM_PITCH;
        m_velX = 0; m_velY = 0; m_velZ = 0;
        Invalidate(FALSE);
        break;
    }

    bool ableToGame = (m_gameState == GAME_NONE);

    if (ableToGame && m_selectedCup <= 0 && nChar == VK_RETURN) {
        StartNewGame();
    }
    else if (m_gameState == GAME_GUESSING && nChar >= '1' && nChar <= '3') {
        int positionIndex = nChar - '1';
        int realCupIndex = GetCupIndexByPosition(positionIndex);

        if (realCupIndex != -1) {
            OnCupClick(realCupIndex);
        }
    }

    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMagicCupGameView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar) {
    case 'W': m_keyW = false; break;
    case 'S': m_keyS = false; break;
    case 'A': m_keyA = false; break;
    case 'D': m_keyD = false; break;
    case VK_SPACE: m_keySpace = false; break;
    case VK_CONTROL: m_keyCtrl = false; break;
    case VK_SHIFT: m_keyShift = false; break;
    }

    CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CMagicCupGameView::InitMaterialPresets()
{
    m_skyboxPresets.clear();

    m_skyboxPresets.push_back({
        _T("fishpond"), _T(".jpg"), _T("Ставок"),
        { 10.0f, 20.0f, 10.0f, 1.0f },
        { 0.6f, 0.6f, 0.6f, 1.0f },
        { 0.9f, 0.9f, 0.8f, 1.0f }
        });

    m_skyboxPresets.push_back({
        _T("footballfield"), _T(".jpg"), _T("Футбольне поле"),
        { -15.0f, 10.0f, 5.0f, 1.0f },
        { 0.4f, 0.4f, 0.4f, 1.0f },
        { 0.8f, 0.8f, 0.8f, 1.0f }
        });

    m_skyboxPresets.push_back({
        _T("footballfield2"), _T(".jpg"), _T("Нічне поле"),
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.1f, 0.1f, 0.1f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f }
        });

    m_skyboxPresets.push_back({
        _T("meadow"), _T(".jpg"), _T("Галявина"),
        { 20.0f, 5.0f, 0.0f, 1.0f },
        { 0.3f, 0.2f, 0.2f, 1.0f },
        { 0.9f, 0.6f, 0.4f, 1.0f }
        });

    m_skyboxPresets.push_back({
        _T("sorsele"), _T(".jpg"), _T("Сорселе 1"),
        { 5.0f, 10.0f, 5.0f, 1.0f },
        { 0.25f, 0.25f, 0.3f, 1.0f },
        { 0.8f, 0.8f, 0.9f, 1.0f }
        });

    m_skyboxPresets.push_back({
        _T("sorsele2"), _T(".jpg"), _T("Сорселе 2"),
        { -5.0f, 12.0f, -5.0f, 1.0f },
        { 0.3f, 0.3f, 0.3f, 1.0f },
        { 0.85f, 0.85f, 0.85f, 1.0f }
        });

    m_skyboxPresets.push_back({
        _T("sorsele3"), _T(".jpg"), _T("Сорселе 3"),
        { 10.0f, 15.0f, 10.0f, 1.0f },
        { 0.2f, 0.2f, 0.25f, 1.0f },
        { 0.9f, 0.9f, 1.0f, 1.0f }
        });

    m_tableMats.clear();

    m_tableMats.push_back({ _T("cocoa"), _T(".png"), _T("Какао")});
    m_tableMats.push_back({ _T("beige"), _T(".png"), _T("Бежевий") });
    m_tableMats.push_back({ _T("chocolate"), _T(".png"), _T("Шоколадний") });
    m_tableMats.push_back({ _T("light_brown"), _T(".png"), _T("Світло-коричневий") });
    m_tableMats.push_back({ _T("olive"), _T(".png"), _T("Оливковий") });
    m_tableMats.push_back({ _T("orange"), _T(".png"), _T("Оранжевий") });

    m_carpetMats.clear();

    m_carpetMats.push_back({ _T("red"), _T(".png"), _T("Червоний") });
    m_carpetMats.push_back({ _T("blue"), _T(".png"), _T("Синій") });
    m_carpetMats.push_back({ _T("dark_red"), _T(".png"), _T("Темно-червоний") });
    m_carpetMats.push_back({ _T("green"), _T(".png"), _T("Зелений") });
    m_carpetMats.push_back({ _T("light_blue"), _T(".png"), _T("Світло-синій") });
    m_carpetMats.push_back({ _T("rainbow"), _T(".png"), _T("Різнокольоровий") });
}

void CMagicCupGameView::SelectSkybox(int index)
{
    if (index < 0 || index >= (int)m_skyboxPresets.size()) return;
    if (m_currentSkyboxIndex == index) return;

    m_currentSkyboxIndex = index;
    const SkyboxSettings& set = m_skyboxPresets[index];

    if (cubeMapTexture != 0) {
        glDeleteTextures(1, &cubeMapTexture);
    }

    const char* suffixes[6] = { "posx", "negx", "posy", "negy", "posz", "negz" };

    std::vector<std::string> filePaths;
    for (int i = 0; i < 6; i++) {
        CString path;
        path.Format(_T("res/skyboxes/%s/%s%s"), set.folderName, CString(suffixes[i]), set.extension);
        CT2A asciiPath(path);
        filePaths.push_back(std::string(asciiPath));
    }

    const char* faces[6] = {
        filePaths[0].c_str(), filePaths[1].c_str(),
        filePaths[2].c_str(), filePaths[3].c_str(),
        filePaths[4].c_str(), filePaths[5].c_str()
    };

    cubeMapTexture = LoadCubeMap(faces);

    for (int i = 0; i < 4; i++) {
        m_actLightPos[i] = set.lightPos[i];
        m_actAmbient[i] = set.ambient[i];
        m_actDiffuse[i] = set.diffuse[i];
    }

    SetupLighting();
}

void CMagicCupGameView::SelectTable(int index)
{
    if (index < 0 || index >= (int)m_tableMats.size()) return;

    m_currentTableMatIndex = index;

    if (tableTexture != 0) {
        glDeleteTextures(1, &tableTexture);
        tableTexture = 0;
    }

    const Material& set = m_tableMats[index];

    CString path;
    path.Format(_T("res/table/%s%s"), set.fileName, set.extension);
    CT2A asciiPath(path);

    tableTexture = LoadTexture(asciiPath);
}

void CMagicCupGameView::SelectCarpet(int index)
{
    if (index < 0 || index >= (int)m_carpetMats.size()) return;

    m_currentCarpetMatIndex = index;

    if (carpetTexture != 0) {
        glDeleteTextures(1, &carpetTexture);
        carpetTexture = 0;
    }

    const Material& set = m_carpetMats[index];

    CString path;
    path.Format(_T("res/carpet/%s%s"), set.fileName, set.extension);
    CT2A asciiPath(path);

    carpetTexture = LoadTexture(asciiPath);
}