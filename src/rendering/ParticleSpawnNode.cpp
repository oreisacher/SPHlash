//
// Created by ruut on 5/30/26.
//

#include "ParticleSpawnNode.h"

ParticleSpawnNode::ParticleSpawnNode(const std::string& path, CudaParticleSpawn* spawn)
    : Model(path), spawn(spawn)
{
    
}

CudaParticleSpawn* ParticleSpawnNode::getParticleSpawn() const {
    return spawn;
}
