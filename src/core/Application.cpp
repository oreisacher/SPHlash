//
// Created by ruut on 3/27/26.
//

#include "core/Application.h"
#include "Version.h"
#include <format>
#include "core/Input.h"
#include "core/Time.h"
#include "ui/panels/ControlPanel.h"
#include "ui/panels/PropertiesPanel.h"
#include "ui/panels/ScenePanel.h"
#include "ui/panels/RenderingSettingsPanel.h"
#include "ui/panels/SimulationPanel.h"
#include "ui/panels/ParticlesPanel.h"
#include "ui/panels/QueryDataPanel.h"
#include "ui/panels/RecordingPanel.h"
#include "ui/panels/RenderSettingsPanel.h"
#include "ui/panels/SimulationSettingsPanel.h"
#include "ui/panels/StatisticsPanel.h"
#include "utils/Logger.h"

#define GLAD_GL_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION

#define WINDOW_SIZE_X 1200
#define WINDOW_SIZE_Y 900
#define SIM_UNITS_PER_PIXEL (1.0 / 600)

Application::Application() {
    Logger::activateLevel(Logger::Info);

    // Window creation
    Window::Init();
    _window = new Window(WINDOW_SIZE_X, WINDOW_SIZE_Y, std::format("SPHlash {}", VERSION).c_str());
    Input::Get()->addInputWindow(_window);

    // Load OpenGL
    if (!gladLoadGL(glfwGetProcAddress)) {
        LOG_FATAL("Failed to initialize GLAD");
        return;
    }

    _scene = new Scene(_window);

    // Simulation
    _scene->addSceneNode(std::make_unique<SimulationNode>());

    // Rendering
    _renderer = new Renderer(_scene);

    _editor = new Editor(_scene);

    // Setup UI
    _ui = new UIManager(_window->getNativeWindow());
    _ui->addPanel(new SimulationPanel            (_editor, _renderer));
    _ui->addPanel(new ControlPanel               (_editor, _renderer));
    _ui->addPanel(new StatisticsPanel            (_editor));
    _ui->addPanel(new QueryDataPanel             (_editor));
    _ui->addPanel(new SimulationSettingsPanel    (_editor, _renderer));
    _ui->addPanel(new RenderingSettingsPanel     (_editor));
    _ui->addPanel(new ParticlesPanel             (_editor));
    _ui->addPanel(new PropertiesPanel            (_editor));
    _ui->addPanel(new ScenePanel                 (_editor, _renderer));
    _ui->addPanel(new RecordingPanel             (_editor, _renderer));
    _ui->addPanel(new RenderSettingsPanel        (_editor, _renderer));
}

Application::~Application() {
    delete _ui;
    delete _window;
    delete _editor;
}

void Application::run() const {
    float deltaTime = 0.0f;
    while (!_window->shouldClose()) {
        deltaTime = Time.getDeltaTime();

        _window->startFrame();

        _scene->update(deltaTime);
        _renderer->render(_scene);

        // Update & Draw UI
        _ui->preDraw(deltaTime);
        _ui->draw();
        _ui->postDraw(deltaTime);

        _window->endFrame();
        Time.update();
        Input::Get()->update();
    }
}