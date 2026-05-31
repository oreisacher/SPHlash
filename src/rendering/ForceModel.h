//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_FORCEMODEL_H
#define SPHLASH_FORCEMODEL_H

#include "Model.h"
#include "simulation/FluidSimulation.h"

class ForceModel : public Model {
public:
    ForceModel();
    ForceField* getForceField();

private:
    ForceField forceField;
};

#endif //SPHLASH_FORCEMODEL_H
