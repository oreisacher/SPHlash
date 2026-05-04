//
// Created by ruut on 4/29/26.
//

#include "Material.h"

Material::Material(const std::string& name, const std::shared_ptr<Shader>& shader)
    : name(name), shader(shader)
{ }

void Material::bind() const {
    if (!shader)
        return;

    shader->bind();

    for (auto& [param, value] : floatParams)
        shader->setFloat(param, value);
    for (auto& [param, value] : vec3Params)
        shader->setVec3(param, value);
    for (auto& [param, value] : vec4Params)
        shader->setVec4(param, value);
}

std::string Material::getName() {
    return name;
}

Shader* Material::getShader() const {
    return shader.get();
}

void Material::setFloat(const std::string& name, float value) {
    floatParams[name] = value;
}

void Material::setVec3(const std::string& name, glm::vec3 value) {
    vec3Params[name] = value;
}

void Material::setVec4(const std::string& name, glm::vec4 value) {
    vec4Params[name] = value;
}

bool Material::isTransparent() {
    if (vec4Params.contains("albedo")) {
        return vec4Params["albedo"].a < 1.0f;
    }

    return false;
}
