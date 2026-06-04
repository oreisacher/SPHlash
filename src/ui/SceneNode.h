//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_SCENENODE_H
#define SPHLASH_SCENENODE_H

#include "ISelectable.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "rendering/IRenderable.h"
#include <unordered_set>

class SceneNode : public ISelectable, public IRenderable {
public:
    SceneNode() = default;
    SceneNode(const nlohmann::json j);
    ~SceneNode() override = default;

    // --- Name ---
    const std::string& getName() const;
    void setName(const std::string& name);

    bool isVisible() const;
    void setVisible(bool visible);

    bool isDeletable() const;
    void setDeletable(bool deletable);

    // --- Editor Flags ---
    bool isSceneGraphVisible() const;
    void setSceneGraphVisible(bool visible);

    // --- Tags ---
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    const std::unordered_set<std::string>& getTags() const;

    // --- Save / Load ---
    bool isSavable() const;
    void setSavable(bool savable);

    virtual nlohmann::json toJson() const;
    virtual std::string getTypeName() const;

    std::vector<SceneNode*> getChildren() const;

protected:
    std::string name;

    bool visible = true;
    bool deletable = true;

    bool sceneGraphVisible = true;
    bool savable = true;

    std::unordered_set<std::string> tags = {};

    std::vector<std::unique_ptr<SceneNode>> children;
};

#endif //SPHLASH_SCENENODE_H
