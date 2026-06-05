#pragma once
#include <GL/glew.h>

class ShadowMap {
public:
    ShadowMap() : m_fbo(0), m_tex(0), m_size(0) {}
    ~ShadowMap() { Destroy(); }

    bool Create(int size);
    void Destroy();

    void BeginPass();
    void EndPass(int viewportW, int viewportH);

    void BindForReading(GLenum textureUnit) const;

    GLuint Tex() const { return m_tex; }
    int Size() const { return m_size; }
    bool IsValid() const { return m_fbo != 0 && m_tex != 0; }

private:
    GLuint m_fbo;
    GLuint m_tex;
    int m_size;
};
