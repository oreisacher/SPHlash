//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
#define SPHLASH_BOUNDINGBOXPARTICLESPAWN_H

#include "BoundingBoxNode.h"
#include "simulation/IParticleSpawn.h"

class BoundingBoxParticleSpawnNode : public BoundingBoxNode, public IParticleSpawn {
public:
    BoundingBoxParticleSpawnNode(const std::string& name, const glm::vec3& color);

    CudaParticleSpawn getParticleSpawn() override;

    Properties buildProperties() override;

private:
    CudaParticleSpawn particleSpawn;
};

#endif //SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
