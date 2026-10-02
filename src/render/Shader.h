#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace game::render {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool loadFromFiles(const std::string& vertPath,
                       const std::string& fragPath);
    bool loadFromSource(const std::string& vertSrc, const std::string& fragSrc);

    void bind() const;
    void unbind() const;
    void destroy();

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& vec) const;
    void setVec4(const std::string& name, const glm::vec4& vec) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    std::uint32_t id() const noexcept {
        return m_program;
    }
    bool valid() const noexcept {
        return m_program != 0;
    }

private:
    std::uint32_t m_program = 0;
};
}  // namespace game::render