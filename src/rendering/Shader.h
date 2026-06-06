//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_SHADER_H
#define FLUIDSIMSPH_SHADER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "glm/fwd.hpp"
#include "glad/gl.h"

class Shader {
public:
    enum Type {
        VERTEX,
        FRAGMENT,
        COMPUTE,

        COUNT
    };

    Shader(std::initializer_list<std::pair<std::string, Type>> sources);
    ~Shader();

    // Sources
    int addSource(const std::string& path, Type type);
    int addSources(std::initializer_list<std::pair<std::string, Type>> srcs);
    int compile();

    void bind() const;
    void unbind() const;

    // Uniforms
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;
    void setBool(const std::string& name, bool value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;

    GLuint getId() const;

    // --- Factory ---
    static std::shared_ptr<Shader> createDefaultShaded();
    static std::shared_ptr<Shader> createDefaultUnshaded();
    static std::shared_ptr<Shader> createBox();
    static std::shared_ptr<Shader> createParticle();
    static std::shared_ptr<Shader> createScreenSpaceParticle();
    static std::shared_ptr<Shader> createTexture3DSlice();
    static std::shared_ptr<Shader> createDepth();
    static std::shared_ptr<Shader> createBlur();
    static std::shared_ptr<Shader> createNormal();
    static std::shared_ptr<Shader> createComposition();
    static std::shared_ptr<Shader> createFullscreen();
    static std::shared_ptr<Shader> createSkybox();

private:
    std::unordered_map<Type, std::string> sources;
    GLuint shader = 0;
};

#endif //FLUIDSIMSPH_SHADER_H