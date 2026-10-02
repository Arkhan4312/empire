#include "render/Shader.h"

#include <glad/glad.h>

#include <cstdio>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

namespace game::render {
static bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return !out.empty();
}

static std::uint32_t compileStage(GLenum type, const std::string& src,
                                  const char* label) {
    const GLuint shader = glCreateShader(type);
    const char* srcPtr = src.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[Shader] %s compile failed:\n%s\n", label, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

Shader::~Shader() {
    destroy();
}

bool Shader::loadFromFiles(const std::string& vertPath,
                           const std::string& fragPath) {
    std::string vertSrc;
    std::string fragSrc;
    if (!readFile(vertPath, vertSrc)) {
        std::fprintf(stderr, "[Shader] cannot open vertex '%s'\n",
                     vertPath.c_str());
        return false;
    }
    if (!readFile(fragPath, fragSrc)) {
        std::fprintf(stderr, "[Shader] cannot open fragment '%s'\n",
                     fragPath.c_str());
        return false;
    }
    return loadFromSource(vertSrc, fragSrc);
}

bool Shader::loadFromSource(const std::string& vertSrc,
                            const std::string& fragSrc) {
    destroy();
    const GLuint vs = compileStage(GL_VERTEX_SHADER, vertSrc, "vertex");
    if (!vs) {
        return false;
    }
    const GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragSrc, "fragment");
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }

    const GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[Shader] link failed: \n%s\n", log);
        glDeleteProgram(prog);
        return false;
    }
    m_program = prog;
    return true;
}

void Shader::bind() const {
    glUseProgram(m_program);
}

void Shader::unbind() const {
    glUseProgram(0);
}

void Shader::destroy() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_program, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& vec) const {
    glUniform2fv(glGetUniformLocation(m_program, name.c_str()), 1,
                 glm::value_ptr(vec));
}

void Shader::setVec4(const std::string& name, const glm::vec4& vec) const {
    glUniform4fv(glGetUniformLocation(m_program, name.c_str()), 1,
                 glm::value_ptr(vec));
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(m_program, name.c_str()), 1,
                       GL_FALSE, glm::value_ptr(mat));
}
}  // namespace game::render