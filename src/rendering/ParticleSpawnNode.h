//
// Created by ruut on 5/30/26.
//

#ifndef SPHLASH_PARTICLESPAWNMODEL_H
#define SPHLASH_PARTICLESPAWNMODEL_H

#include "Model.h"
#include "simulation/FluidSimulation.h"

class ParticleSpawnNode : public Model {
public:
    ParticleSpawnNode(const std::string& path, CudaParticleSpawn* spawn);
    CudaParticleSpawn* getParticleSpawn() const;

private:
    CudaParticleSpawn* spawn;
};

#endif //SPHLASH_PARTICLESPAWNMODEL_H
