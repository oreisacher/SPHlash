//
// Created by ruut on 3/27/26.
//

#include "Application.h"

#include "rendering/FluidRenderer.h"
#include "rendering/Window.h"
#include "ui/FPSPanel.h"
#include "ui/ParticlesPanel.h"
#include "ui/SettingsPanel.h"
#include "ui/SimulationPanel.h"
#include "ui/UIManager.h"
#include "utils/Camera.h"
#include "utils/InputManager.h"
#include "utils/Logger.h"
#include "utils/Time.h"

#define GLAD_GL_IMPLEMENTATION

#define WINDOW_SIZE_X 1000
#define WINDOW_SIZE_Y 600
#define SIM_UNITS_PER_PIXEL (1.0 / 600)
#define PARTICLES 10000

Application::Application() {
    // Window creation
    Window::Init();
    window = new Window(WINDOW_SIZE_X, WINDOW_SIZE_Y, "Fluid Simulation");
    Input.addInputWindow(window);

    // Load OpenGL
    if (!gladLoadGL(glfwGetProcAddress)) {
        LOG_FATAL("Failed to initialize GLAD");
        return;
    }

    // Simulation
    simulation = new FluidSimulationGPU(PARTICLES, { 0.3f, 0.6f }, { 0.3f, 0.6f });

    // Rendering
    renderer = new FluidRenderer(simulation);

    // Setup UI
    ui = new UIManager(window->getNativeWindow());
    ui->addPanel(new SimulationPanel("Simulation", renderer->getTextureId()));
    ui->addPanel(new FPSPanel("FPS"));
    ui->addPanel(new SettingsPanel("Settings", simulation, renderer));
    ui->addPanel(new ParticlesPanel("Particles", renderer->getParticleVBO(), PARTICLES, renderer->getParticleShader()));

    // Setup Camera
    camera = new Camera({ 0.0f, 0.0f, 1.0f });
    renderer->setCamera(camera);
}

Application::~Application() {
    delete simulation;
    delete renderer;
    delete ui;
    delete camera;
    delete window;
}

void Application::run() const {
    while (!window->shouldClose()) {
        // Simulation
        // simulation.update(Motor::Time.getDeltaTime());
        // simulation.update(0.016);  // Force 60 fps
        // simGPU.update(0.008333333); // Force 120 fps
        simulation->update(0.0005);
        simulation->update(0.0005);
        simulation->update(0.0005);
        simulation->update(0.0005);

        // Render simulation into texture
        renderer->render();

        // Update & Draw UI
        ui->update(Time.getDeltaTime());
        ui->draw();

        processEvents();

        window->endFrame();
        Time.update();
        Input.reset();
    }
}

void Application::processEvents() const {
    for (const auto& e : ui->getPanel<SimulationPanel>()->getEvents()) {
        switch (e.type) {
        case PanelEvent::RESIZE:
            // Update framebuffer
            renderer->setSize(e.resize.size.x, e.resize.size.y);

            // Update camera aspect ratio
            camera->setAspectRatio(e.resize.size.x / e.resize.size.y);
            break;
        case PanelEvent::MOUSEDOWN:
            if (e.mouse.button == 2)
                window->setCursorMode(Window::CursorMode::CURSOR_HIDDEN);
            break;
        case PanelEvent::MOUSEUP:
            if (e.mouse.button == 2) {
                window->setCursorMode(Window::CursorMode::CURSOR_NORMAL);

                // Reset camera to ignore first delta to avoid jump
                camera->reset();
            }
            break;
        case PanelEvent::MOUSEHOLD:
            if (e.mouse.button == 2) {
                // Update camera if holding
                camera->update(Time.getDeltaTime());
            }
            break;
        default: break;
        }
    }
}
