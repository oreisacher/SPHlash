//
// Created by ruut on 7/24/26.
//

#ifndef SPHLASH_QUERYNODE_H
#define SPHLASH_QUERYNODE_H

#include "BoundingBoxNode.h"
#include "simulation/SimulationNode.h"

struct FloatStat {
    float avg, min, max;

    __host__ __device__
    static FloatStat fromValue(float v) {
        return { v, v, v };
    }

    __host__ __device__
    static FloatStat combine(const FloatStat& a, const FloatStat& b) {
        return { a.avg + b.avg, fminf(a.min, b.min), fmaxf(a.max, b.max) };
    }

    void average(int count) {
        if (count > 0) avg /= static_cast<float>(count);
    }
};

inline std::ostream& operator<<(std::ostream& os, const FloatStat& s) {
    return os << s.avg << "," << s.min << "," << s.max;
}

struct VecStat {
    glm::vec3 avg, min, max;

    __host__ __device__
    static VecStat fromValue(const glm::vec3 v) {
        return { v, v, v };
    }

    __host__ __device__
    static VecStat combine(const VecStat& a, const VecStat& b) {
        return { a.avg + b.avg, glm::min(a.min, b.min), glm::max(a.max, b.max) };
    }

    __host__ __device__
    static VecStat combineBySpeed(const VecStat& a, const VecStat& b) {
        float lenAMin = glm::length(a.min), lenBMin = glm::length(b.min);
        float lenAMax = glm::length(a.max), lenBMax = glm::length(b.max);

        return {
            a.avg + b.avg,
            lenAMin <= lenBMin ? a.min : b.min,
            lenAMax >= lenBMax ? a.max : b.max,
        };
    }

    void average(int count) {
        if (count > 0) avg /= static_cast<float>(count);
    }
};

inline std::ostream& operator<<(std::ostream& os, const VecStat& s) {
    return os << s.avg.x << "," <<  s.avg.y << "," << s.avg.z << ","
              << s.min.x << "," <<  s.min.y << "," << s.min.z << ","
              << s.max.x << "," <<  s.max.y << "," << s.max.z;
}

struct Metrics {
    int count = 0;
    float deltaTime = 0;
    float gravity = 0;
    float depth = 0;

    VecStat position              = { };
    VecStat velocity              = { };
    VecStat pressureForce         = { };
    VecStat viscosityForce        = { };
    VecStat surfaceTensionForce   = { };

    FloatStat density             = { };
    FloatStat densityError        = { };
    FloatStat pressure            = { };

    __host__ __device__
    static Metrics identity() {
        VecStat v { glm::vec3(0), glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
        VecStat lenV { glm::vec3(0), glm::vec3(FLT_MAX), glm::vec3(0) };
        FloatStat f { 0.0f, FLT_MAX, -FLT_MAX };

        return { 0, 0, 0, 0, v, lenV, lenV, lenV, lenV, f, f, f };
    }

    void average() {
        position.average(count);
        velocity.average(count);
        pressureForce.average(count);
        viscosityForce.average(count);
        surfaceTensionForce.average(count);
        density.average(count);
        densityError.average(count);
        pressure.average(count);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Metrics& m) {
    os  << m.count << ","
        << m.deltaTime << ","
        << m.gravity << ","
        << m.depth << ","
        << m.position << ","
        << m.velocity << ","
        << m.pressureForce << ","
        << m.viscosityForce << ","
        << m.surfaceTensionForce << ","
        << m.density << ","
        << m.densityError << ","
        << m.pressure;

    return os;
}

struct GatherParticleStats {
    ParticlesGPU particles;
    float restDensity;
    AABBView area;
    int filter;

    __host__ __device__ __forceinline__
    Metrics operator()(int i) const {
        // Check if particle is inside quer node area
        if (particles.d_position[i].x < area.min.x || particles.d_position[i].x > area.max.x ||
            particles.d_position[i].y < area.min.y || particles.d_position[i].y > area.max.y ||
            particles.d_position[i].z < area.min.z || particles.d_position[i].z > area.max.z )
            return Metrics::identity();

        // Check if particle is filtered
        if (filter == 1 && particles.d_flags[i].y != 1.0)
            return Metrics::identity();

        float density = particles.d_properties[i].y;
        float pressure = particles.d_properties[i].z;

        return {
            1,
            0, 0, 0,
            VecStat::fromValue(particles.d_position[i]),
            VecStat::fromValue(particles.d_velocity[i]),
            VecStat::fromValue(particles.d_pressureForce[i]),
            VecStat::fromValue(particles.d_viscosityForce[i]),
            VecStat::fromValue(particles.d_surfaceTensionForce[i]),
            FloatStat::fromValue(density),
            FloatStat::fromValue(fabsf(density - restDensity) / restDensity),
            FloatStat::fromValue(pressure)
        };
    }
};

struct ParticleStatsOp {
    __host__ __device__ __forceinline__
    Metrics operator()(const Metrics& a, const Metrics& b) const {
        return {
            a.count + b.count,
            0, 0, 0,
            VecStat::combine(a.position, b.position),
            VecStat::combineBySpeed(a.velocity, b.velocity),
            VecStat::combineBySpeed(a.pressureForce, b.pressureForce),
            VecStat::combineBySpeed(a.viscosityForce, b.viscosityForce),
            VecStat::combineBySpeed(a.surfaceTensionForce, b.surfaceTensionForce),
            FloatStat::combine(a.density, b.density),
            FloatStat::combine(a.densityError, b.densityError),
            FloatStat::combine(a.pressure, b.pressure)
        };
    }
};

class QueryNode : public BoundingBoxNode {
public:
    QueryNode();
    explicit QueryNode(const nlohmann::json json);

    void onAddedToScene() override;

    void init() override;
    void update(float deltaTime) override;

    void reset();
    void exportData(const std::string& path) const;

    std::vector<Metrics>& getHistory();

    Properties buildProperties() override;

    nlohmann::json toJson() const override;
    std::string getTypeName() const override;

private:
    SimulationNode* simulation = nullptr;
    int filter = 0;

    std::vector<Metrics> history = {};

    Metrics* d_result       = nullptr;
    void* d_tempStorage     = nullptr;
    size_t tempStorageBytes = 0;
    int allocatedBlocks     = 0;

    std::string target = "";

    float targetDepth = -1.0f;
    float surfaceHeight = 0.0;

    float heightDiff = 0.0f;
    float threshold = 0.00002f;
    float timeBetweenPoints = 1;
    std::vector<float> vel;
    std::vector<float> pressures;
};

#endif //SPHLASH_QUERYNODE_H
