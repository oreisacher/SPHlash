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

#include "core/Input.h"

QueryNode::QueryNode()
    : BoundingBoxNode("QueryNode", glm::vec3(0.0, 0.0, 0.7))
{ }

QueryNode::QueryNode(const nlohmann::json json) : BoundingBoxNode(json) {
    filter = json.value("filter", filter);
    target = json.value("target", target);
    heightDiff = json.value("heightDiff", heightDiff);
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

    if (name == "Depth" || name == "DepthHighRes") {
        for (auto qn : scene->getQueryNodes()) {
            if (qn->getName() == "Top") {
                auto h = qn->getHistory();

                if (h.empty())
                    break;

                // DO TARGET STUFF
                setPosition({
                    position.x,
                    h.back().position.avg.y - heightDiff,
                    position.z
                });
            }
        }
    }

    if (name == "DepthHighRes") {
        c++;

        if (c < 200)
            return;
    }

    if (name == "Depth" && target != "") {
        for (auto qn : scene->getQueryNodes()) {
            if (qn->getName() == "All") {
                auto h = qn->getHistory();

                if (h.empty())
                    break;

                vel.push_back(h.back().velocity.avg.y);
            }

            if (qn->getName() == "DepthHighRes") {
                auto h = qn->getHistory();

                if (h.empty())
                    break;

                pressures.push_back(h.back().pressure.avg);
            }
        }
    }

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
    result.gravity = simulation->settings.gravity;

    int count = std::min<int>(1000, pressures.size());

    // AUTOMATIC MEASURING STUFF
    if (getName() == "Depth") {
        const auto begin = pressures.end() - count;
        const float sum = std::accumulate(begin, pressures.end(), 0.0f);
        float a = sum / static_cast<float>(count);
        std::cout << a << std::endl;

        if (vel.size() < 100)
            return;

        float minHeight = std::numeric_limits<float>::infinity();
        float maxHeight = -std::numeric_limits<float>::infinity();

        int count = 0;
        for (auto it = vel.rbegin() ; it != vel.rend() && count < 100; ++it, ++count) {
            minHeight = std::min(minHeight, *it);
            maxHeight = std::max(maxHeight, *it);
        }

        float avg = maxHeight - minHeight;

        if (Input::Get()->isKeyDown(Window::Key::F8))
            avg = 0.0f;

        if (fabsf(avg) > threshold)
            return;
    }

    if (name == "Depth") {
        int count = std::min<int>(1000, pressures.size());

        const auto begin = pressures.end() - count;
        const float sum = std::accumulate(begin, pressures.end(), 0.0f);
        result.pressure.avg = sum / static_cast<float>(count);

        history.push_back(result);
        std::cout << "Recorded " << history.size() << std::endl;

        simulation->settings.gravity -= 0.5;

        if (simulation->settings.gravity < -21) {
            simulation->pause();
            return;
        }

        simulation->reset();

        for (const auto& node : scene->getQueryNodes())
            node->reset();

        simulation->start();
        vel.clear();
        pressures.clear();
    } else {
        history.push_back(result);
    }
}

void QueryNode::reset() {
    if (name == "Depth")
        return;

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
            "gravity,"

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

Properties QueryNode::buildProperties() {
    Properties properties = BoundingBoxNode::buildProperties();

    properties.addSection("Query");

    properties.addProperty(std::make_unique<StringProperty>(
       "Target",
       [this] () { return target; },
       [this] (std::string t) { target = t; }
    ));

    properties.addProperty(std::make_unique<FloatProperty>(
       "Threshold",
       [this] () { return threshold; },
       [this] (float t) { threshold = t; }
    ));

    properties.addProperty(std::make_unique<FloatProperty>(
        "Height Diff",
        [this] () { return heightDiff; },
        [this] (float t) { heightDiff = t; }
    ));

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
    j["target"] = target;
    j["heightDiff"] = heightDiff;

    return j;
}

std::string QueryNode::getTypeName() const {
    return "QueryNode";
}
