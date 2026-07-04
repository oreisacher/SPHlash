//
// Created by ruut on 3/27/26.
//

#include "Application.h"
#include "Version.h"
#include <format>
#include "InputManager.h"
#include "Time.h"
#include "ui/panels/ControlPanel.h"
#include "ui/panels/PropertiesPanel.h"
#include "ui/panels/ScenePanel.h"
#include "ui/panels/RenderingSettingsPanel.h"
#include "ui/panels/SimulationPanel.h"
#include "ui/panels/ParticlesPanel.h"
#include "ui/panels/SimulationSettingsPanel.h"
#include "ui/panels/StatisticsPanel.h"
#include "utils/Logger.h"

#define GLAD_GL_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#define WINDOW_SIZE_X 1200
#define WINDOW_SIZE_Y 900
#define SIM_UNITS_PER_PIXEL (1.0 / 600)

Application::Application() {
    // Window creation
    Window::Init();
    _window = new Window(WINDOW_SIZE_X, WINDOW_SIZE_Y, std::format("SPHlash {}", VERSION).c_str());
    Input.addInputWindow(_window);

    // Load OpenGL
    if (!gladLoadGL(glfwGetProcAddress)) {
        LOG_FATAL("Failed to initialize GLAD");
        return;
    }

    _scene = new Scene(_window);

    // Simulation
    _simulation = new FluidSimulation(_scene);

    // Rendering
    _renderer = new Renderer(_scene);

    _editor = new Editor(_scene, _simulation);

    // Setup UI
    _ui = new UIManager(_window->getNativeWindow());
    _ui->addPanel(new SimulationPanel            (_editor, _renderer));
    _ui->addPanel(new ControlPanel               (_editor, _simulation, _renderer));
    _ui->addPanel(new StatisticsPanel            (_simulation));
    _ui->addPanel(new SimulationSettingsPanel    (_editor, _simulation, _renderer));
    _ui->addPanel(new RenderingSettingsPanel     (_scene->getParticlesNode()));
    _ui->addPanel(new ParticlesPanel             (_simulation, _scene->getParticlesNode()));
    _ui->addPanel(new PropertiesPanel            (_editor));
    _ui->addPanel(new ScenePanel                 (_editor, _renderer));

    // Setup scene
    _camera = _scene->getCamera();
}

Application::~Application() {
    delete _simulation;
    delete _ui;
    delete _window;
    delete _editor;
}

void Application::run() const {
    float deltaTime = 0.0f;
    while (!_window->shouldClose()) {
        deltaTime = Time.getDeltaTime();

        _window->startFrame();

        // Simulation
        _simulation->update(deltaTime);

        // Update SceneNodes
        _scene->update(deltaTime);

        // Render simulation into texture
        _renderer->render(_scene);

        // Update & Draw UI
        _ui->preDraw(deltaTime);
        _ui->draw();
        _ui->postDraw(deltaTime);

        _window->endFrame();
        Time.update();
        Input.reset();
    }
}