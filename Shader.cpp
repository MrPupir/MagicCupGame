#include "pch.h"
#include "Shader.h"
#include <string>

static GLuint compileShader(GLenum type, const char* src, CString& outErr) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);

    GLint status = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 0 ? len : 1, '\0');
        if (len > 0) glGetShaderInfoLog(sh, len, nullptr, &log[0]);
        outErr = CA2T(log.c_str(), CP_UTF8);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

bool Shader::BuildFromSource(const char* vsSrc, const char* fsSrc, CString& outErr) {
    Destroy();
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc, outErr);
    if (!vs) { outErr = _T("VS: ") + outErr; return false; }

    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc, outErr);
    if (!fs) { glDeleteShader(vs); outErr = _T("FS: ") + outErr; return false; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint status = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 0 ? len : 1, '\0');
        if (len > 0) glGetProgramInfoLog(prog, len, nullptr, &log[0]);
        outErr = _T("LINK: ") + CString(CA2T(log.c_str(), CP_UTF8));
        glDeleteProgram(prog);
        return false;
    }

    m_program = prog;
    return true;
}

void Shader::Destroy() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}
