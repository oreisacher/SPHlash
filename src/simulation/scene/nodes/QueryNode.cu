//
// Created by ruut on 7/24/26.
//

#include "simulation/scene/nodes/QueryNode.h"
#include <cuda_runtime_api.h>
#include <c++/16/fstream>
#include <cub/device/device_reduce.cuh>
#include "simulation/utils/SimulationMetrics.cuh"
#include "utils/Logger.h"

QueryNode::QueryNode(FluidSimulation* simulation)
    : BoundingBoxNode("QueryNode", glm::vec3(0.0, 0.0, 0.7)), simulation(simulation)
{
    QueryNode::init();
}

QueryNode::QueryNode(const nlohmann::json json) : BoundingBoxNode(json) {
    filter = json.value("filter", filter);

    QueryNode::init();
}

void QueryNode::init() {
    // Free old memory
    if (d_blockMetric)  cudaFree(d_blockMetric);
    if (d_result)       cudaFree(d_result);
    if (d_tempStorage)  cudaFree(d_tempStorage);

    history.clear();

    int particleCount = simulation->getParticleCount();

    if (particleCount == 0)
        return;

    // Allocate necessary blocks
    allocatedBlocks = (particleCount + 255) / 256;
    cudaMalloc(&d_blockMetric, allocatedBlocks * sizeof(Metrics));

    // Allocate reduction metric
    cudaMalloc(reinterpret_cast<void**>(&d_result), sizeof(Metrics));

    // Query cub reduce storage size
    cub::DeviceReduce::Reduce(
        nullptr,
        tempStorageBytes,
        d_blockMetric,
        d_result,
        allocatedBlocks,
        MetricsAdd(),
        Metrics{}
    );

    cudaMalloc(&d_tempStorage, tempStorageBytes);
}

void QueryNode::update(float deltaTime) {
    if (simulation->getState() != RUNNING)
        return;

    int particleCount = simulation->getParticleCount();
    ParticlesGPU particles = simulation->getParticlesGPU();

    metricsKernel<<<allocatedBlocks, 256>>>(
        particleCount,
        particles,
        filter,
        simulation->settings.p_0,
        getWorldBoundingBox().getAABBView(),
        d_blockMetric
    );

    cub::DeviceReduce::Reduce(
        d_tempStorage,
        tempStorageBytes,
        d_blockMetric,
        d_result,
        allocatedBlocks,
        MetricsAdd(),
        Metrics{}
    );

    Metrics result;
    cudaMemcpy(&result, d_result, sizeof(Metrics), cudaMemcpyDeviceToHost);

    if (result.count == 0) {
        history.push_back(Metrics{});
    } else {
        // Calc average
        result.position /= result.count;
        result.velocity /= result.count;
        result.pressureForce /= result.count;
        result.viscosityForce /= result.count;
        result.surfaceTensionForce /= result.count;
        result.density /= result.count;
        result.pressure /= result.count;

        result.densityError /= result.count;

        history.push_back(result);
    }
}

void QueryNode::reset() {
    history.clear();
}

void QueryNode::exportData(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Could not open destination file.");
        return;
    }

    file << "frame,"
            "count,"
            "pos_x,pos_y,pos_z,"
            "vel_x,vel_y,vel_z,"
            "pressure_force_x,pressure_force_y,pressure_force_z,"
            "viscosity_force_x,viscosity_force_y,viscosity_force_z,"
            "surface_tension_force_x,surface_tension_force_y,surface_tension_force_z,"
            "density,"
            "densityError,"
            "pressure\n";

    for (int i = 0 ; i < history.size(); i++)
        file << i << "," << history[i] << "\n";
}

Properties QueryNode::buildProperties() {
    Properties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Query");

    properties.addProperty(std::make_unique<EnumProperty>(
        "Filter",
        [this] () { return filter; },
        [this] (int index) { filter = index; },
        std::vector<std::string>{ "All Particles", "Top Surface Particles" }
    ));

    properties.addProperty(std::make_unique<ButtonProperty>(
        "Clear Data",
        [this] {
            history.clear();
        }
    ));

    properties.addProperty(std::make_unique<DialogProperty>(
        "Export Data",
        [this](const std::string& path) {
            exportData(path);
        },
        ".csv"
    ));

    return properties;
}

std::vector<Metrics>& QueryNode::getHistory() {
    return history;
}

nlohmann::json QueryNode::toJson() const {
    nlohmann::json j = BoundingBoxNode::toJson();

    j["filter"] = filter;

    return j;
}

std::string QueryNode::getTypeName() const {
    return "QueryNode";
}
