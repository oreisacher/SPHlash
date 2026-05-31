//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_SCENENODE_H
#define SPHLASH_SCENENODE_H

#include "ISelectable.h"

class SceneNode : public ISelectable {
public:
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

protected:
    std::string name;

    bool visible = true;
    bool deletable = true;

    bool sceneGraphVisible = true;
};

#endif //SPHLASH_SCENENODE_H
