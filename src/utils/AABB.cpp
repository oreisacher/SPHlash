//
// Created by ruut on 5/2/26.
//

#include "AABB.h"
#include <glm/glm.hpp>

void AABB::setCenter(glm::vec3 center) {
    this->center = center;
}

void AABB::setSize(glm::vec3 size) {
    this->size = size;
}

glm::vec3 AABB::min() const {
    return center - 0.5f * size;
}

glm::vec3 AABB::max() const {
    return center + 0.5f * size;
}

float AABB::volume() const {
    return size.x * size.y * size.z;
}

bool AABB::contains(const glm::vec3& point) const {
    glm::vec3 min = center - size * 0.5f;
    glm::vec3 max = center + size * 0.5f;

    return
        point.x >= min.x && point.x <= max.x &&
        point.y >= min.y && point.y <= max.y &&
        point.z >= min.z && point.z <= max.z;
}

AABB AABB::tranform(glm::mat4 matrix) {
    glm::vec3 newCenter = glm::vec3(matrix * glm::vec4(center, 1.0));

    glm::mat3 R = glm::mat3(matrix);

    glm::vec3 newSize = glm::vec3(
        glm::dot(glm::abs(R[0]), size),
        glm::dot(glm::abs(R[1]), size),
        glm::dot(glm::abs(R[2]), size)
    );

    return {
        newCenter,
        newSize
    };
}

CudaAABB AABB::toCudaAABB() const {
    const glm::vec3 min = center - size * 0.5f;
    const glm::vec3 max = center + size * 0.5f;

    return {
        { min.x, min.y, min.z },
        { max.x, max.y, max.z }
    };
}
