//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_EDITOR_H
#define SPHLASH_EDITOR_H
#include "Scene.h"

class Editor {
public:
    Editor(Scene& scene);

private:
    Scene* scene;

    // --- Editor Objects ---
    Model* spawnAreaModel   = nullptr;
};

#endif //SPHLASH_EDITOR_H
