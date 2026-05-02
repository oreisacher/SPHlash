//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_PRIMITIVES_H
#define SPHLASH_PRIMITIVES_H

#include "rendering/Mesh.h"

namespace Primitives {
    struct BoxGeometry {
        // Simple cube data
        //
        //      7 -------- 6
        //     /|         /|
        //    3 -------- 2 |
        //    | |        | |
        //    | 4 -------| 5
        //    |/         |/
        //    0 -------- 1

        static std::vector<Vertex> GetVertices() {
            return {
                { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 0
                { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 1
                { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 2
                { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 3
                { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 4
                { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 5
                { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 6
                { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }  // 7
            };
        }

        static std::vector<unsigned int> GetWireframeIndices() {
            return {
                0, 1, 1, 2, 2, 3, 3, 0,
                4, 5, 5, 6, 6, 7, 7, 4,
                0, 4, 1, 5, 2, 6, 3, 7
            };
        }
    };
}

#endif //SPHLASH_PRIMITIVES_H
