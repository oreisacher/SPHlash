//
// Created by ruut on 3/27/26.
//

#include "Application.h"
#include "Version.h"

#include "rendering/FluidRenderer.h"
#include "rendering/Window.h"
#include "ui/StatisticsPanel.h"
#include "ui/ParticlesPanel.h"
#include "ui/SimulationSettingsPanel.h"
#include "ui/SimulationPanel.h"
#include "ui/UIManager.h"
#include "utils/Camera.h"
#include "utils/InputManager.h"
#include "utils/Logger.h"
#include "utils/Time.h"

#define GLAD_GL_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#define WINDOW_SIZE_X 1200
#define WINDOW_SIZE_Y 800
#define SIM_UNITS_PER_PIXEL (1.0 / 600)

#include "tiny_obj_loader.h"
#include "rendering/ForceModel.h"
#include "rendering/Mesh.h"
#include "ui/ControlPanel.h"
#include "ui/InitPanel.h"
#include "ui/ScenePanel.h"
#include "ui/RenderingSettingsPanel.h"

Application::Application() {
    // Window creation
    Window::Init();
    window = new Window(WINDOW_SIZE_X, WINDOW_SIZE_Y, std::format("SPHlash {}", VERSION).c_str());
    Input.addInputWindow(window);

    // Load OpenGL
    if (!gladLoadGL(glfwGetProcAddress)) {
        LOG_FATAL("Failed to initialize GLAD");
        return;
    }

    // Simulation
    simulation = new FluidSimulation(scene);

    // Rendering
    renderer = new FluidRenderer(scene, simulation);

    surfaceMesher = new FluidSurfaceMesher(scene, simulation, renderer);

    editor = new Editor(scene, simulation);

    // Setup UI
    ui = new UIManager(window->getNativeWindow());
    ui->addPanel(new SimulationPanel            (renderer->getTextureId()));
    ui->addPanel(new ControlPanel               (simulation, surfaceMesher));
    ui->addPanel(new StatisticsPanel            (simulation));
    ui->addPanel(new InitPanel                  (editor, simulation, renderer, surfaceMesher));
    ui->addPanel(new SimulationSettingsPanel    (editor, simulation, renderer));
    ui->addPanel(new RenderingSettingsPanel     (renderer, surfaceMesher));
    ui->addPanel(new ParticlesPanel             (simulation, renderer));
    ui->addPanel(new ScenePanel                 (scene, editor, renderer));

    // Setup scene
    scene.setCamera(std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 1.0f)));
}

Application::~Application() {
    delete simulation;
    delete surfaceMesher;
    delete ui;
    delete window;
    delete editor;
}

void Application::run() const {
    while (!window->shouldClose()) {
        // Simulation
        simulation->update(Time.getDeltaTime());

        // Build fluid surface if necessary
        surfaceMesher->generateMesh();

        // Render simulation into texture
        renderer->render(scene);

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
            scene.getCamera()->setAspectRatio(e.resize.size.x / e.resize.size.y);
            break;
        case PanelEvent::MOUSEDOWN:
            if (e.mouse.button == 2)
                window->setCursorMode(Window::CursorMode::CURSOR_HIDDEN);
            break;
        case PanelEvent::MOUSEUP:
            if (e.mouse.button == 2) {
                window->setCursorMode(Window::CursorMode::CURSOR_NORMAL);

                // Reset camera to ignore first delta to avoid jump
                scene.getCamera()->reset();
            }
            break;
        case PanelEvent::MOUSEHOLD:
            if (e.mouse.button == 2) {
                // Update camera if holding
                scene.getCamera()->update(Time.getDeltaTime());
            }
            break;
        default: break;
        }
    }
}
