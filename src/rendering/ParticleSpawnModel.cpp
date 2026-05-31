//
// Created by ruut on 5/30/26.
//

#include "ParticleSpawnModel.h"

ParticleSpawnModel::ParticleSpawnModel(const std::string& path, ParticleSpawn* spawn)
    : Model(path), spawn(spawn)
{
    
}

ParticleSpawn* ParticleSpawnModel::getParticleSpawn() const {
    return spawn;
}
