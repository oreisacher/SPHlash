//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_FORCEMODEL_H
#define SPHLASH_FORCEMODEL_H

#include "Model.h"
#include "simulation/FluidSimulation.h"

class ForceModel : public Model {
public:
    ForceModel(ForceField* forceField);
    ForceField* getForceField() const;

private:
    ForceField* forceField = nullptr;
};

#endif //SPHLASH_FORCEMODEL_H
