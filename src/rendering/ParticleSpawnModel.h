//
// Created by ruut on 5/30/26.
//

#ifndef SPHLASH_PARTICLESPAWNMODEL_H
#define SPHLASH_PARTICLESPAWNMODEL_H

#include "Model.h"
#include "simulation/FluidSimulation.h"

class ParticleSpawnModel : public Model {
public:
    ParticleSpawnModel(const std::string& path, ParticleSpawn* spawn);
    ParticleSpawn* getParticleSpawn() const;

private:
    ParticleSpawn* spawn;
};

#endif //SPHLASH_PARTICLESPAWNMODEL_H
