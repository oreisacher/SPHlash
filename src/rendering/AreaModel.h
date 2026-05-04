//
// Created by ruut on 5/4/26.
//

#ifndef SPHLASH_AREAMODEL_H
#define SPHLASH_AREAMODEL_H

#include "Model.h"
#include "utils/AABB.h"

class AreaModel : public Model{
public:
    AreaModel(const std::string& name, glm::vec3 color, AABB& area);

private:
    AABB* area;
};

#endif //SPHLASH_AREAMODEL_H
