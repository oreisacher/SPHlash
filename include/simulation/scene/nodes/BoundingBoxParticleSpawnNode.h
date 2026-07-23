//
// Created by ruut on 6/1/26.
//

#ifndef SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
#define SPHLASH_BOUNDINGBOXPARTICLESPAWN_H

#include "BoundingBoxNode.h"
#include "../IParticleSpawn.h"

class BoundingBoxParticleSpawnNode : public BoundingBoxNode, public IParticleSpawn {
public:
    BoundingBoxParticleSpawnNode(const std::string& name);
    explicit BoundingBoxParticleSpawnNode(const nlohmann::json json);

    CudaParticleSpawn getParticleSpawn() override;

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    CudaParticleSpawn particleSpawn;
};

#endif //SPHLASH_BOUNDINGBOXPARTICLESPAWN_H
