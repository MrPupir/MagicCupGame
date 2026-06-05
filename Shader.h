#pragma once
#include <GL/glew.h>

class Shader {
public:
    Shader() : m_program(0) {}
    ~Shader() { Destroy(); }

    bool BuildFromSource(const char* vsSrc, const char* fsSrc, CString& outErr);
    void Destroy();

    void Use() const { glUseProgram(m_program); }
    static void Unuse() { glUseProgram(0); }

    GLuint Program() const { return m_program; }
    bool IsValid() const { return m_program != 0; }

    GLint Loc(const char* name) const { return glGetUniformLocation(m_program, name); }

    void Set1i(const char* name, int v) const { glUniform1i(Loc(name), v); }
    void Set1f(const char* name, float v) const { glUniform1f(Loc(name), v); }
    void Set3f(const char* name, float x, float y, float z) const { glUniform3f(Loc(name), x, y, z); }
    void Set4f(const char* name, float x, float y, float z, float w) const { glUniform4f(Loc(name), x, y, z, w); }
    void SetMat4(const char* name, const float* m) const { glUniformMatrix4fv(Loc(name), 1, GL_FALSE, m); }

private:
    GLuint m_program;
};
