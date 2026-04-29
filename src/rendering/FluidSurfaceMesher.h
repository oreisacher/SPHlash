//
// Created by ruut on 4/27/26.
//

#ifndef SPHLASH_FLUIDSURFACEMESHER_H
#define SPHLASH_FLUIDSURFACEMESHER_H

#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulationGPU.h"

class FluidSurfaceMesher {
public:
    FluidSurfaceMesher();

    void generateMesh() const;

private:
    FluidRenderer* renderer;
    FluidSimulationGPU* simulation;

    Mesh* surfaceMesh;
};

#endif //SPHLASH_FLUIDSURFACEMESHER_H
