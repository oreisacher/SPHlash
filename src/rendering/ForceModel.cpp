//
// Created by ruut on 5/4/26.
//

#include "ForceModel.h"

// ForceModel::ForceModel(ForceField* forceField) : Model("assets/models/forceField.obj", false), forceField(forceField) {
//     name = "Force Field";
//
//     // Set initial size of ForceModel
//     transform.scale = { 0.25, 0.25, 0.25 };
//     updateTransform();
//
//     addOnTransformChangedListener([this, forceField](Model* model) {
//         forceField->position = { transform.position.x, transform.position.y, transform.position.z };
//         forceField->halfExtents = { 0.5f * transform.scale.x, 0.5f * transform.scale.y, 0.5f * transform.scale.z };
//
//         glm::vec3 right   = transform.rotation * glm::vec3(1, 0,  0);
//         forceField->right = { right.x, right.y, right.z };
//
//         glm::vec3 up      = transform.rotation * glm::vec3(0, 1,  0);
//         forceField->up    = { up.x, up.y, up.z };
//
//         glm::vec3 forward = transform.rotation * glm::vec3(0, 0, -1);
//         forceField->forward = { forward.x, forward.y, forward.z };
//     });
// }

ForceModel::ForceModel() : Model("assets/models/forceField.obj", false) {
    name = "Force Field";

    // Set initial size of ForceModel
    // transform.scale = { 0.25, 0.25, 0.25 };
    // updateTransform();
}

ForceField* ForceModel::getForceField() {
    return &forceField;
}
