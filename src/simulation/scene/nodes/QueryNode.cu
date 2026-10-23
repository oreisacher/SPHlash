//
// Created by ruut on 7/24/26.
//

#include "simulation/scene/nodes/QueryNode.cuh"
#include <cuda_runtime_api.h>
#include <cub/device/device_reduce.cuh>
#include "utils/Logger.h"
#include <fstream>
#include "simulation/scene/Scene.h"
#include <cub/cub.cuh>
#include <thrust/iterator/transform_iterator.h>

QueryNode::QueryNode()
    : BoundingBoxNode("QueryNode", glm::vec3(0.0, 0.0, 0.7))
{ }

QueryNode::QueryNode(const nlohmann::json json) : BoundingBoxNode(json) {
    filter = json.value("filter", filter);
}

void QueryNode::onAddedToScene() {
    simulation = scene->getNode<SimulationNode>();

    if (!simulation)
        return;

    QueryNode::init();
}

void QueryNode::init() {
    // Free old memory
    if (d_result)       cudaFree(d_result);
    if (d_tempStorage)  cudaFree(d_tempStorage);

    history.clear();

    int particleCount = simulation->getParticleCount();

    if (particleCount == 0)
        return;

    // Allocate reduction metric
    cudaMalloc(reinterpret_cast<void**>(&d_result), sizeof(Metrics));

    thrust::counting_iterator<int> counting(0);
    GatherParticleStats gather{};
    auto in = thrust::make_transform_iterator(counting, gather);

    cub::DeviceReduce::Reduce(
        nullptr,
        tempStorageBytes,
        in,
        d_result,
        particleCount,
        ParticleStatsOp{},
        Metrics::identity()
    );

    cudaMalloc(&d_tempStorage, tempStorageBytes);
}

void QueryNode::update(float deltaTime) {
    if (simulation->getState() != RUNNING)
        return;

    int particleCount = simulation->getParticleCount();
    ParticlesGPU particles = simulation->getParticlesGPU();

    thrust::counting_iterator<int> counting(0);
    GatherParticleStats gather{ particles, simulation->settings.p_0, getWorldBoundingBox().getAABBView(), filter };
    auto in = thrust::make_transform_iterator(counting, gather);

    cub::DeviceReduce::Reduce(
        d_tempStorage,
        tempStorageBytes,
        in,
        d_result,
        particleCount,
        ParticleStatsOp{},
        Metrics::identity()
    );

    Metrics result;
    cudaMemcpy(&result, d_result, sizeof(Metrics), cudaMemcpyDeviceToHost);
    result.average();

    result.deltaTime = deltaTime;

    history.push_back(result);

    if (streamOutput.is_open()) {
        streamOutput << history.size() - 1 << ","
                 << result << "\n";

        streamOutput.flush();
    }
}

void QueryNode::reset() {
    history.clear();
}

void QueryNode::exportData(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Could not open destination file.");
        return;
    }

    file << "frame,"
            "count,"
            "deltaTime,"

            "avg_pos_x,avg_pos_y,avg_pos_z,"
            "min_pos_x,min_pos_y,min_pos_z,"
            "max_pos_x,max_pos_y,max_pos_z,"

            "avg_vel_x,avg_vel_y,avg_vel_z,"
            "min_vel_x,min_vel_y,min_vel_z,"
            "max_vel_x,max_vel_y,max_vel_z,"

            "avg_pressure_force_x,avg_pressure_force_y,avg_pressure_force_z,"
            "min_pressure_force_x,min_pressure_force_y,min_pressure_force_z,"
            "max_pressure_force_x,max_pressure_force_y,max_pressure_force_z,"

            "avg_viscosity_force_x,avg_viscosity_force_y,avg_viscosity_force_z,"
            "min_viscosity_force_x,min_viscosity_force_y,min_viscosity_force_z,"
            "max_viscosity_force_x,max_viscosity_force_y,max_viscosity_force_z,"

            "avg_surface_tension_force_x,avg_surface_tension_force_y,avg_surface_tension_force_z,"
            "min_surface_tension_force_x,min_surface_tension_force_y,min_surface_tension_force_z,"
            "max_surface_tension_force_x,max_surface_tension_force_y,max_surface_tension_force_z,"

            "avg_density,"
            "min_density,"
            "max_density,"

            "avg_densityError,"
            "min_densityError,"
            "max_densityError,"

            "avg_pressure,"
            "min_pressure,"
            "max_pressure\n";

    for (int i = 0 ; i < history.size(); i++)
        file << i << "," << history[i] << "\n";


}

void QueryNode::startExporting(const std::string& path) {
    streamOutput.open(path);

    if (!streamOutput.is_open()) {
        LOG_ERROR("Could not open destination file.");
        return;
    }

    streamOutput << "frame,"
                    "count,"

                    "avg_pos_x,avg_pos_y,avg_pos_z,"
                    "min_pos_x,min_pos_y,min_pos_z,"
                    "max_pos_x,max_pos_y,max_pos_z,"

                    "avg_vel_x,avg_vel_y,avg_vel_z,"
                    "min_vel_x,min_vel_y,min_vel_z,"
                    "max_vel_x,max_vel_y,max_vel_z,"

                    "avg_pressure_force_x,avg_pressure_force_y,avg_pressure_force_z,"
                    "min_pressure_force_x,min_pressure_force_y,min_pressure_force_z,"
                    "max_pressure_force_x,max_pressure_force_y,max_pressure_force_z,"

                    "avg_viscosity_force_x,avg_viscosity_force_y,avg_viscosity_force_z,"
                    "min_viscosity_force_x,min_viscosity_force_y,min_viscosity_force_z,"
                    "max_viscosity_force_x,max_viscosity_force_y,max_viscosity_force_z,"

                    "avg_surface_tension_force_x,avg_surface_tension_force_y,avg_surface_tension_force_z,"
                    "min_surface_tension_force_x,min_surface_tension_force_y,min_surface_tension_force_z,"
                    "max_surface_tension_force_x,max_surface_tension_force_y,max_surface_tension_force_z,"

                    "avg_density,"
                    "min_density,"
                    "max_density,"

                    "avg_densityError,"
                    "min_densityError,"
                    "max_densityError,"

                    "avg_pressure,"
                    "min_pressure,"
                    "max_pressure\n";
}

void QueryNode::stopExporting() {
    streamOutput.close();
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

    properties.addProperty(std::make_unique<DialogProperty>(
        "Start stream export",
        [this](const std::string& path) {
            startExporting(path);
        },
        ".csv"
    ));

    properties.addProperty(std::make_unique<ButtonProperty>(
        "Stop stream export",
        [this] {
            stopExporting();
        }
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
