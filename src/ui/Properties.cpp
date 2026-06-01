//
// Created by ruut on 5/31/26.
//

#include "Properties.h"

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

void FloatProperty::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    float v = get();
    if (ImGui::DragFloat(id.c_str(), &v, 0.01f))
        set(v);
}

void BoolProperty::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    bool v = get();
    if (ImGui::Checkbox(id.c_str(), &v))
        set(v);
};

void Vec3Property::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    glm::vec3 v = get();
    if (ImGui::DragFloat3(id.c_str(), glm::value_ptr(v), 0.01f))
        set(v);
}

void ColorProperty::draw() {
    ImGui::Text("%s:", name.c_str());
    std::string id = "##" + name;

    glm::vec4 v = get();
    if (ImGui::ColorEdit4(id.c_str(), glm::value_ptr(v)))
        set(v);
}

void Properties::addSection(const std::string& section) {
    sections.push_back({ section, static_cast<int>(properties.size()) });
}

Section Properties::getSection(int index) {
    return sections[index];
}

void Properties::addProperty(std::unique_ptr<Property> property) {
    properties.push_back(std::move(property));
}

Property* Properties::getProperty(int index) const {
    return properties[index].get();
}

int Properties::getPropertyCount() const {
    return properties.size();
}

int Properties::getSectionCount() const {
    return sections.size();
}
