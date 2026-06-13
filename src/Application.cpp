//
// Created by ruut on 3/27/26.
//

#include "Application.h"
#include "Version.h"

#include "rendering/Renderer.h"
#include "rendering/Window.h"
#include "ui/StatisticsPanel.h"
#include "ui/ParticlesPanel.h"
#include "ui/SimulationSettingsPanel.h"
#include "ui/SimulationPanel.h"
#include "ui/UIManager.h"
#include "utils/CameraNode.h"
#include "utils/InputManager.h"
#include "utils/Logger.h"
#include "utils/Time.h"

#define GLAD_GL_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#define WINDOW_SIZE_X 1200
#define WINDOW_SIZE_Y 900
#define SIM_UNITS_PER_PIXEL (1.0 / 600)

#include "tiny_obj_loader.h"
#include "ui/ControlPanel.h"
#include "ui/PropertiesPanel.h"
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
    simulation = new FluidSimulation(&scene);

    // Rendering
    renderer = new Renderer(&scene);

    editor = new Editor(scene, simulation);

    // Setup UI
    ui = new UIManager(window->getNativeWindow());
    ui->addPanel(new SimulationPanel            (editor, renderer));
    ui->addPanel(new ControlPanel               (editor, simulation, renderer));
    ui->addPanel(new StatisticsPanel            (simulation));
    ui->addPanel(new SimulationSettingsPanel    (editor, simulation, renderer));
    ui->addPanel(new RenderingSettingsPanel     (scene.getParticlesNode()));
    ui->addPanel(new ParticlesPanel             (simulation, scene.getParticlesNode()));
    ui->addPanel(new PropertiesPanel            (editor));
    ui->addPanel(new ScenePanel                 (editor, renderer));

    // Setup scene
    scene.addSceneNode(std::make_unique<CameraNode>(window, glm::vec3(0.0f, 0.0f, 1.0f)));
    camera = scene.getCamera();
}

Application::~Application() {
    delete simulation;
    delete ui;
    delete window;
    delete editor;
}

void Application::run() const {
    float deltaTime = 0.0f;
    while (!window->shouldClose()) {
        deltaTime = Time.getDeltaTime();

        window->startFrame();

        // Simulation
        simulation->update(deltaTime);

        // Update SceneNodes
        scene.update(deltaTime);

        // Render simulation into texture
        renderer->render(scene);

        // Update & Draw UI
        ui->preDraw(deltaTime);
        ui->draw();
        ui->postDraw(deltaTime);

        window->endFrame();
        Time.update();
        Input.reset();
    }
}