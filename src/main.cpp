#include <format>
#include <iostream>
#include <random>

#include "rendering/FluidRenderer.h"
#include "simulation/FluidSimulation.h"
#include "glm/gtc/type_ptr.hpp"
#include "rendering/Shader.h"
#include "rendering/Window.h"
#include "simulation/FluidSimulationGPU.h"
#include "ui/FPSPanel.h"
#include "ui/SettingsPanel.h"
#include "ui/UIManager.h"
#include "ui/SimulationPanel.h"
#include "utils/Time.h"

#define GLAD_GL_IMPLEMENTATION

#define WINDOW_SIZE_X 600
#define WINDOW_SIZE_Y 600
#define SIM_UNITS_PER_PIXEL (1.0 / WINDOW_SIZE_X)

int main() {
    // Window creation
    Window::Init();
    Window window(WINDOW_SIZE_X, WINDOW_SIZE_Y, "Fluid Simulation");

    FluidRenderer::Init();

    // Simulation
    // FluidSimulation simulation(1000, 0.1f, 0.1f, 0.5f, 0.5f);
    // simulation.updateBoundaries(window.getWidth() * SIM_UNITS_PER_PIXEL, window.getHeight() * SIM_UNITS_PER_PIXEL);

    FluidSimulationGPU simGPU(10000, 0.1f, 0.1f, 0.6f, 0.6f);

    // CUDA TESTING
    // int h_a[5] = { 1, 2, 3, 4, 5 };

    // int* d_a;
    // cudaMalloc(reinterpret_cast<void**>(&d_a), 5 * sizeof(int));
    // cudaMemcpy(d_a, h_a, 5 * sizeof(int), cudaMemcpyHostToDevice);
    // launchAdd(d_a);
    // cudaMemcpy(h_a, d_a, 5 * sizeof(int), cudaMemcpyDeviceToHost);
    // cudaFree(d_a);
    //
    // for (int a : h_a) {
    //     std::cout << a << std::endl;
    // }

    // Rendering
    FluidRenderer renderer(&simGPU);

    UIManager ui(window.getNativeWindow());

    // Viewport Panel
    auto vp = new SimulationPanel("Simulation", renderer.getTextureId());
    ui.addPanel(vp);
    ui.addPanel(new FPSPanel("FPS"));
    ui.addPanel(new SettingsPanel("Settings", &simGPU));

    // Setup Shader
    Shader shader({
        { "assets/shaders/default.vert", Shader::VERTEX },
        { "assets/shaders/default.frag", Shader::FRAGMENT },
    });

    shader.compile();
    shader.bind();

    shader.setMat4("projection", glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f));
    shader.setFloat("size", 0.007f);

    while (!window.shouldClose()) {
        // Simulation
        // simulation.update(Motor::Time.getDeltaTime());
        // simulation.update(0.016);  // Force 60 fps
        simGPU.update(0.008333333); // Force 120 fps

        // Render simulation into texture
        renderer.render();

        // Update & Draw UI
        ui.update(Time.getDeltaTime());
        ui.draw();

        // Check if viewport size changed
        if (vp->wasResized()) {
            vp->clearResizeFlag();
            auto size = vp->getSize();

            // Resize framebuffer
            renderer.setSize(size.x, size.y);

            // Update simulation bounds
            simGPU.updateBoundaries(
                size.x * SIM_UNITS_PER_PIXEL,
                size.y * SIM_UNITS_PER_PIXEL
            );

            shader.setMat4("projection", glm::ortho(
                0.0,
                size.x * SIM_UNITS_PER_PIXEL,
                0.0,
                size.y * SIM_UNITS_PER_PIXEL,
                -1.0,
                1.0
            ));
        }

        // if (auto click = vp->isMouseDown()) {
        //     auto mouse = vp->getMousePosition();
        //
        //     simGPU.updateMousePos(
        //         mouse.x * SIM_UNITS_PER_PIXEL,
        //         mouse.y * SIM_UNITS_PER_PIXEL
        //     );
        //     simGPU.updateMouseForceDirection(click == 1 ? 1.0f : -1.0f);
        // } else {
        //     simGPU.updateMousePos(INFINITY, INFINITY);
        // }
        //
        // if (vp->getScroll() != 0.0) {
        //     simGPU.updateMouseRadius(vp->getScroll());
        // }

        window.endFrame();
        Time.update();
    }
}
