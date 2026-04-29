//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_MATERIAL_H
#define SPHLASH_MATERIAL_H

#include "Shader.h"
#include <memory>
#include <glm/glm.hpp>

class Material {
public:
    Material(const std::string& name, const std::shared_ptr<Shader>& shader);

    void bind() const;

    std::string getName();
    Shader* getShader() const;

    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, glm::vec3 value);

private:
    std::string name;

    std::shared_ptr<Shader> shader;
    std::unordered_map<std::string, float> floatParams;
    std::unordered_map<std::string, glm::vec3> vec3Params;
};

#endif //SPHLASH_MATERIAL_H
