//
// Created by ruut on 6/11/26.
//

#ifndef SPHLASH_MODELPARTICLESPAWNNODE_H
#define SPHLASH_MODELPARTICLESPAWNNODE_H

#include "Model.h"
#include "ModelNode.h"
#include "simulation/IParticleSpawn.h"
#include "ui/SceneNode.h"

class ModelParticleSpawnNode : public ModelNode, public IParticleSpawn {
public:
    ModelParticleSpawnNode(const std::string& name, const std::shared_ptr<Model>& model);
    ModelParticleSpawnNode(const nlohmann::json json);
    ~ModelParticleSpawnNode() override;

    CudaParticleSpawn getParticleSpawn() override;

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    void init();

    CudaParticleSpawn particleSpawn;
};

#endif //SPHLASH_MODELPARTICLESPAWNNODE_H
