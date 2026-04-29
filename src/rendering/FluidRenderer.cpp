//
// Created by ruut on 3/9/26.
//

#define GLAD_GL_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL

#include "rendering/FluidRenderer.h"
#include <glad/gl.h>

#include "Mesh.h"
#include "../simulation/FluidSimulation.h"
#include "GLFW/glfw3.h"
#include "utils/Logger.h"
#include "utils/SphereCreator.h"
#include <vector>

#include "cuda/MarchingCubes.h"
#include "glm/gtx/transform.hpp"
#include "utils/Primitives.h"

FluidRenderer::FluidRenderer() {
    simulation = FluidSimulationGPU::getInstance();

    // Setup OpenGL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Setup framebuffer
    framebuffer = new Framebuffer(1, 1);
    framebuffer->addColorAttachment();
    framebuffer->addDepthBuffer();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Setup sphere
    setupSphereModel();

    // Setup particle
    setupParticles();

    // Setup Spawn Area box
    setupSpawnAreaModel();
    updateSpawnBoxModelMatrix();

    // Setup shaders
    setupShaders();
}

FluidRenderer::~FluidRenderer() {
    delete framebuffer;
    delete boxShader;

    for (const auto m : models) {
        delete m;
    }
}

FluidRenderer* FluidRenderer::getInstance() {
    static FluidRenderer instance;
    return &instance;
}

void FluidRenderer::update() {
    // Marching cubes
    // float3 cellSizes = make_float3(
    //     (simulation->settings.xDomain.y - simulation->settings.xDomain.x) / static_cast<float>(settings.marchingCubeResolution.x - 1),
    //     (simulation->settings.yDomain.y - simulation->settings.yDomain.x) / static_cast<float>(settings.marchingCubeResolution.y - 1),
    //     (simulation->settings.zDomain.y - simulation->settings.zDomain.x) / static_cast<float>(settings.marchingCubeResolution.z - 1)
    // );

    // simulation->computeScalarField(settings.marchingCubeResolution);
    //
    // int* d_vertexCount;
    // int* d_triangleCount;
    // cudaMalloc(&d_vertexCount, sizeof(int));
    // cudaMalloc(&d_triangleCount, sizeof(int));
    // cudaMemset(d_vertexCount, 0, sizeof(int));
    // cudaMemset(d_triangleCount, 0, sizeof(int));
    //
    // cudaGraphicsMapResources(1, &marchingCubeMesh->vboResource, nullptr);
    // cudaGraphicsMapResources(1, &marchingCubeMesh->eboResource, nullptr);
    //
    // Vertex* d_vertices;
    // unsigned int* d_indices;
    // size_t vboSize, eboSize;
    //
    // cudaGraphicsResourceGetMappedPointer((void**)&d_vertices, &vboSize, marchingCubeMesh->vboResource);
    // cudaGraphicsResourceGetMappedPointer((void**)&d_indices, &eboSize, marchingCubeMesh->eboResource);
    //
    // launchMarchingCubesKernel(
    //     simulation->voxelField.d_field,
    //     simulation->voxelField.d_normal,
    //     settings.marchingCubeResolution,
    //     { simulation->settings.xDomain.x, simulation->settings.yDomain.x, simulation->settings.zDomain.x },
    //     { simulation->settings.xDomain.y, simulation->settings.yDomain.y, simulation->settings.zDomain.y },
    //     d_vertexCount,
    //     d_triangleCount,
    //     d_vertices,
    //     d_indices,
    //     marchingCubeMesh->maxVertices,
    //     marchingCubeMesh->maxIndices
    // );
    //
    // // Unmap resources
    // cudaGraphicsUnmapResources(1, &marchingCubeMesh->vboResource, nullptr);
    // cudaGraphicsUnmapResources(1, &marchingCubeMesh->eboResource, nullptr);
    //
    // int vertexCount, triangleCount;
    // cudaMemcpy(&vertexCount, d_vertexCount, sizeof(int), cudaMemcpyDeviceToHost);
    // cudaMemcpy(&triangleCount, d_triangleCount, sizeof(int), cudaMemcpyDeviceToHost);
    //
    // marchingCubeMesh->vertexCount = vertexCount;
    // marchingCubeMesh->indexCount = triangleCount * 3;
    //
    // cudaFree(d_vertexCount);
    // cudaFree(d_triangleCount);

    // if (samples.first.empty()) {
    //     return;
    // }
    //
    // glm::vec3 domainMin(
    //     simulation->settings.xDomain.x,
    //     simulation->settings.yDomain.x,
    //     simulation->settings.zDomain.x
    // );
    //
    // int dx = 1;
    // int dy = settings.marchingCubeResolution.x;
    // int dz = settings.marchingCubeResolution.x * settings.marchingCubeResolution.y;
    //
    // std::vector<Vertex> vertices = {};
    // std::vector<unsigned int> indices = {};
    //
    // for (int z = 0; z < settings.marchingCubeResolution.z - 1; z++) {
    //     for (int y = 0; y < settings.marchingCubeResolution.y - 1; y++) {
    //         for (int x = 0; x < settings.marchingCubeResolution.x - 1; x++) {
    //             int id = x + y * settings.marchingCubeResolution.x + z * settings.marchingCubeResolution.x * settings.marchingCubeResolution.y;
    //
    //             float cube[8];
    //             cube[0] = samples.first[id                ];
    //             cube[1] = samples.first[id + dx           ];
    //             cube[2] = samples.first[id + dx + dy      ];
    //             cube[3] = samples.first[id      + dy      ];
    //             cube[4] = samples.first[id           + dz ];
    //             cube[5] = samples.first[id + dx      + dz ];
    //             cube[6] = samples.first[id + dx + dy + dz ];
    //             cube[7] = samples.first[id      + dy + dz ];
    //
    //             // Build index
    //             int cubeIndex = 0;
    //             if (cube[0] < 0) cubeIndex |= 1;
    //             if (cube[1] < 0) cubeIndex |= 2;
    //             if (cube[2] < 0) cubeIndex |= 4;
    //             if (cube[3] < 0) cubeIndex |= 8;
    //             if (cube[4] < 0) cubeIndex |= 16;
    //             if (cube[5] < 0) cubeIndex |= 32;
    //             if (cube[6] < 0) cubeIndex |= 64;
    //             if (cube[7] < 0) cubeIndex |= 128;
    //
    //             // Skip empty / full cells
    //             if (cubeIndex == 0 || cubeIndex == 255)
    //                 continue;
    //
    //             float3 normals[8];
    //             normals[0] = samples.second[id                ];
    //             normals[1] = samples.second[id + dx           ];
    //             normals[2] = samples.second[id + dx + dy      ];
    //             normals[3] = samples.second[id      + dy      ];
    //             normals[4] = samples.second[id           + dz ];
    //             normals[5] = samples.second[id + dx      + dz ];
    //             normals[6] = samples.second[id + dx + dy + dz ];
    //             normals[7] = samples.second[id      + dy + dz ];
    //
    //             glm::vec3 position[8];
    //             position[0] = glm::vec3((x + 0) * cellSizes.x, (y + 0) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    //             position[1] = glm::vec3((x + 1) * cellSizes.x, (y + 0) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    //             position[2] = glm::vec3((x + 1) * cellSizes.x, (y + 1) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    //             position[3] = glm::vec3((x + 0) * cellSizes.x, (y + 1) * cellSizes.y, (z + 0) * cellSizes.z) + domainMin;
    //             position[4] = glm::vec3((x + 0) * cellSizes.x, (y + 0) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;
    //             position[5] = glm::vec3((x + 1) * cellSizes.x, (y + 0) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;
    //             position[6] = glm::vec3((x + 1) * cellSizes.x, (y + 1) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;
    //             position[7] = glm::vec3((x + 0) * cellSizes.x, (y + 1) * cellSizes.y, (z + 1) * cellSizes.z) + domainMin;
    //
    //             // Add vertices
    //             int edgeVertexIndex[12] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    //             for (int i = 0 ; i < 12 ; i++) {
    //                 if ((edgeTable[cubeIndex] & (1 << i)) == 0)
    //                     continue;
    //
    //                 auto verts = edgeToVertices[i];
    //
    //                 auto lerpedVertex = interp(
    //                     position[verts.first],
    //                     position[verts.second],
    //                     cube[verts.first],
    //                     cube[verts.second]
    //                 );
    //
    //                 auto lerpedNormal = interp(
    //                     normals[verts.first],
    //                     normals[verts.second],
    //                     cube[verts.first],
    //                     cube[verts.second]
    //                 );
    //
    //                 edgeVertexIndex[i] = vertices.size();
    //                 vertices.push_back({{ lerpedVertex }, { lerpedNormal.x, lerpedNormal.y, lerpedNormal.z }});
    //             }
    //
    //             // Build faces
    //             for (int i = 0 ; triangleTable[cubeIndex][i] != -1 ; i+= 3) {
    //                 int index0 = triangleTable[cubeIndex][i    ];
    //                 int index1 = triangleTable[cubeIndex][i + 1];
    //                 int index2 = triangleTable[cubeIndex][i + 2];
    //
    //                 indices.push_back(edgeVertexIndex[index0]);
    //                 indices.push_back(edgeVertexIndex[index1]);
    //                 indices.push_back(edgeVertexIndex[index2]);
    //             }
    //         }
    //     }
    // }
    //
    // if (marchingCubeMesh) {
    //     marchingCubeMesh->updateMeshData(vertices, indices);
    // } else {
    //     marchingCubeMesh = new Mesh("SDF", vertices, indices);
    //
    //     addModel(new Model("SDF", { marchingCubeMesh }));
    // }
}

void FluidRenderer::render(const Scene& scene) const {
    // // Update shader
    // particleShader->bind();
    // particleShader->setMat4("projection", camera->getProjectionMatrix());
    // particleShader->setMat4("view", camera->getViewMatrix());

    // Render into framebuffer
    framebuffer->bind();
    glViewport(0, 0, framebuffer->getSize().x, framebuffer->getSize().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // // Draw particles
    // if (simulation->particleCount != 0) {
    //     glBindVertexArray(vao);
    //     glDrawElementsInstanced(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0, simulation->particleCount);
    // }
    //
    // boxShader->bind();
    // boxShader->setMat4("view", camera->getViewMatrix());
    // boxShader->setMat4("projection", camera->getProjectionMatrix());
    // glBindVertexArray(boxVAO);
    //
    // // Draw domain box
    // boxShader->setMat4("model", domainModel);
    // boxShader->setVec3("color", glm::vec3(0.2f, 0.6f, 1.0f));
    // glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    //
    // // Draw spawn box
    // boxShader->setMat4("model", spawnBoxModel);
    // boxShader->setVec3("color", glm::vec3(1.0f, 0.3f, 0.3f));
    // glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    //
    // modelShader->bind();
    // modelShader->setMat4("projection", camera->getProjectionMatrix());
    // modelShader->setMat4("view", camera->getViewMatrix());
    //
    // // Draw surface model
    // if (surfaceModel && surfaceModel->isVisible())
    //     surfaceModel->draw(modelShader);

    // Camera transform
    auto camera = scene.getCamera();
    auto view = camera->getViewMatrix();
    auto projection = camera->getProjectionMatrix();

    for (const auto model : scene.getModels()) {
        if (!model->isVisible())
            continue;

        model->draw(view, projection);
    }
}

void FluidRenderer::setCamera(Camera* cam) {
    camera = cam;
}

Shader* FluidRenderer::getParticleShader() const {
    return particleShader;
}

Shader* FluidRenderer::getBoundaryShader() const {
    return boxShader;
}

void FluidRenderer::updateParticleSize() const {
    particleShader->bind();
    particleShader->setFloat("size", settings.particleSize);
}

void FluidRenderer::updateSpawnBoxModelMatrix() {
    glm::vec3 max(
        simulation->initSettings.xBoundary.y,
        simulation->initSettings.yBoundary.y,
        simulation->initSettings.zBoundary.y
    );

    glm::vec3 min(
        simulation->initSettings.xBoundary.x,
        simulation->initSettings.yBoundary.x,
        simulation->initSettings.zBoundary.x
    );

    // spawnAreaModel->setTransform(calculateModelMatrix(min, max));
}

void FluidRenderer::updateDomainModelMatrix() {
    glm::vec3 max(
        simulation->settings.xDomain.y,
        simulation->settings.yDomain.y,
        simulation->settings.zDomain.y
    );

    glm::vec3 min(
        simulation->settings.xDomain.x,
        simulation->settings.yDomain.x,
        simulation->settings.zDomain.x
    );

    // domainModel = calculateModelMatrix(min , max);
}

GLuint FluidRenderer::getTextureId() const {
    return framebuffer->getColorAttachment(0);
}

void FluidRenderer::setSize(int width, int height) const {
    auto size = framebuffer->getSize();
    if (size.x != width || size.y != height)
        framebuffer->resize(width, height);
}

GLuint FluidRenderer::getParticleVBO() const { return particleVBO; }

void FluidRenderer::setupShaders() {
    // Particle Shader
    particleShader = new Shader({
        { "assets/shaders/default.vert", Shader::VERTEX },
        { "assets/shaders/default.frag", Shader::FRAGMENT },
    });

    particleShader->compile();
    particleShader->bind();

    // Set default settings
    particleShader->setInt("mode", settings.mode);
    particleShader->setFloat("size", settings.particleSize);
    particleShader->setVec3("particleColor", settings.solidColorParticles);

    // Boundary Box Shader
    boxShader = new Shader({
        { "assets/shaders/box.vert", Shader::VERTEX },
        { "assets/shaders/box.frag", Shader::FRAGMENT },
    });

    boxShader->compile();

    modelShader = new Shader({
        { "assets/shaders/model.vert", Shader::VERTEX },
        { "assets/shaders/model.frag", Shader::FRAGMENT },
    });

    modelShader->compile();
}

void FluidRenderer::setupSphereModel() {
    // Generate mesh data for sphere
    std::vector<SphereVertex> vertices;
    std::vector<unsigned int> indices;
    createSphere(vertices, indices, 4, 2);
    sphereIndexCount = indices.size();

    // Vertex buffer
    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SphereVertex), vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glGenBuffers(1, &sphereEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)offsetof(SphereVertex, normal));
    glEnableVertexAttribArray(1);
}

void FluidRenderer::setupParticles() {
    glGenBuffers(1, &particleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, velocity));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(RendererParticle), (void*)offsetof(RendererParticle, properties));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, flags));
    // glEnableVertexAttribArray(5);
    // glVertexAttribDivisor(5, 1)
    //
    // glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleGPU), (void*)offsetof(ParticleGPU, debugging));
    // glEnableVertexAttribArray(6);
    // glVertexAttribDivisor(6, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // simulation->setRendererVBO(particleVBO);
}

void FluidRenderer::setupSpawnAreaModel() {
    // spawnAreaModel = new Model(
    //     "Particle Spawn Area",
    //     { new Mesh(
    //         "PSA",
    //         Primitives::BoxGeometry::GetVertices(),
    //         Primitives::BoxGeometry::GetWireframeIndices(),
    //         GL_LINES)
    //     }
    // );
    //
    // Material boxMaterial("Wireframe", Shader::createBox());
    // boxMaterial.setVec3("color", glm::vec3(1.0f, 0.3f, 0.3f));
    // spawnAreaModel->addMaterial(boxMaterial);
    //
    // editorModels.push_back(spawnAreaModel);
}

glm::mat4 FluidRenderer::calculateModelMatrix(glm::vec3 min, glm::vec3 max) {
    const glm::vec3 center = (min + max) * 0.5f;
    const glm::vec3 scale = (max - min) * 0.5f;

    return glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), scale);
}

void FluidRenderer::updateParticleData() const {
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RendererParticle) * simulation->particleCount, nullptr, GL_DYNAMIC_DRAW);
    simulation->setRendererVBO(particleVBO);
}

void FluidRenderer::updateRenderingSettings() const {
    particleShader->bind();

    particleShader->setInt("mode", settings.mode);
    particleShader->setFloat("size", settings.particleSize);

    particleShader->setVec3("particleColor", settings.solidColorParticles);

    particleShader->setVec3("minVelColor", settings.minVelColor);
    particleShader->setVec3("maxVelColor", settings.maxVelColor);
    particleShader->setFloat("minVel", settings.velocityMin);
    particleShader->setFloat("maxVel", settings.velocityMax);

    particleShader->setVec3("minPresColor", settings.minPresColor);
    particleShader->setVec3("maxPresColor", settings.maxPresColor);
    particleShader->setFloat("minPres", settings.pressureMin);
    particleShader->setFloat("maxPres", settings.pressureMax);

    particleShader->setVec3("solidColor", settings.solidColor);
}

void FluidRenderer::addModel(const std::string& path) {
    models.push_back(new Model(path));
}

void FluidRenderer::addModel(Model* model) {
    models.push_back(model);
}

void FluidRenderer::removeModel(int index) {
    if (index < 0 || index >= models.size())
        return;

    delete models[index];
    models.erase(models.begin() + index);
}

std::vector<Model*> FluidRenderer::getModels() const {
    return models;
}

void FluidRenderer::setSurfaceModel(Model* model) {
    surfaceModel = model;
}
