//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_PROPERTIES_H
#define SPHLASH_PROPERTIES_H

#include <functional>
#include <memory>
#include <string>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "rendering/Texture.h"

struct Property {
    virtual ~Property() = default;
    virtual void draw() = 0;
};

struct FloatProperty : Property {
    FloatProperty(const std::string& name, std::function<float()> get, std::function<void (float)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<float()>          get;
    std::function<void (float)>     set;

    void draw() override;
};

struct BoolProperty : Property {
    BoolProperty(const std::string& name, std::function<bool()> get, std::function<void (bool)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<bool()>          get;
    std::function<void (bool)>     set;

    void draw() override;
};

struct Vec3Property : Property {
    Vec3Property(const std::string& name, std::function<glm::vec3()> get, std::function<void (glm::vec3)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<glm::vec3()>      get;
    std::function<void (glm::vec3)> set;

    void draw() override;
};

struct ColorProperty : Property {
    ColorProperty(const std::string& name, std::function<glm::vec3()> get, std::function<void (glm::vec3)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<glm::vec3()>      get;
    std::function<void (glm::vec3)> set;

    void draw() override;
};

struct TextureProperty : Property {
    TextureProperty(const std::string& name, std::function<std::shared_ptr<Texture>()> get, std::function<void (std::shared_ptr<Texture>)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                                    name;
    std::function<std::shared_ptr<Texture>()>      get;
    std::function<void (std::shared_ptr<Texture>)> set;

    void draw() override;
};

struct Section {
    std::string name;
    int index;
};

class Properties {
public:
    void addSection(const std::string& section);
    Section getSection(int index);

    void addProperty(std::unique_ptr<Property> property);
    Property* getProperty(int index) const;

    int getPropertyCount() const;
    int getSectionCount() const;

private:
    std::vector<Section>                    sections;
    std::vector<std::unique_ptr<Property>>  properties;
};

#endif //SPHLASH_PROPERTIES_H
