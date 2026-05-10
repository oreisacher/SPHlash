//
// Created by ruut on 4/13/26.
//

#include "ScenePanel.h"
#include <format>
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "ImGuizmo.h"
#include "misc/cpp/imgui_stdlib.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.inl"
#include "rendering/AreaModel.h"
#include "rendering/Editor.h"
#include "rendering/ForceModel.h"
#include "rendering/Scene.h"

ScenePanel::ScenePanel(Scene& scene, Editor* editor, FluidRenderer* renderer)
    : UIPanel("Scene"), editor(editor), renderer(renderer), scene(&scene)
{ }

void ScenePanel::update(double deltaTime) { }

void ScenePanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // --- Scene ---
    ImGui::SeparatorText("Scene Graph");

    std::vector<Model*> models = scene->getModels();

    if (ImGui::BeginTable("Models", 3, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

        for (int i = 0 ; i < models.size() ; i++) {
            if (!models[i]->isSceneGraphVisible())
                continue;

            ImGui::TableNextRow();
            ImGui::PushID(i);

            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();

            bool isSelected = (selected == i);

            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.60f, 0.28f, 0.28f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.35f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.80f, 0.42f, 0.42f, 1.00f));
            }

            if (ImGui::Button(models[i]->getName().c_str(), ImVec2(-FLT_MIN, 19))) {
                if (selected != i) {
                    selected = i;
                    selectedModel = models[i];
                } else {
                    selected = -1;
                    selectedModel = nullptr;
                }
            }

            if (isSelected) {
                ImGui::PopStyleColor(3);
            }

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                if (isSelected) {
                    selected = -1;
                    selectedModel = nullptr;
                }
            }

            ImGui::TableNextColumn();

            auto visibilityLabel = models[i]->isVisible() ? "Hide" : "Show";
            if (ImGui::Button(visibilityLabel, ImVec2(0, 19))) {
                models[i]->setVisible(!models[i]->isVisible());
            }

            ImGui::TableNextColumn();
            if (models[i]->isSceneGraphDeletable()) {
                if (ImGui::Button(" - ", ImVec2(0, 19))) {
                    if (isSelected) {
                        selected = -1;
                        selectedModel = nullptr;
                    }

                    // Remove force field
                    if (auto forceField = dynamic_cast<ForceModel*>(models[i])) {
                        editor->removeForceField(forceField);
                    }

                    scene->removeModel(i);
                }
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("");

    if (ImGuiFileDialog::Instance()->Display("LoadModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            editor->addBoundaryModel(std::make_unique<Model>(filePathName.c_str()));
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Add Boundary Model", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadModel", "Choose Model", ".obj", config);
    }

    if (ImGui::Button("Add Force Field", ImVec2(totalWidth, 0))) {
        editor->addForceField();
    }

    // --- Properties ---
    if (selectedModel) {
        static ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE mode = ImGuizmo::MODE::WORLD;

        if (ImGui::IsKeyPressed(ImGuiKey_F))
            operation = ImGuizmo::TRANSLATE;

        if (ImGui::IsKeyPressed(ImGuiKey_G))
            operation = ImGuizmo::ROTATE;

        if (ImGui::IsKeyPressed(ImGuiKey_H))
            operation = ImGuizmo::SCALE;

        if (ImGui::IsKeyPressed(ImGuiKey_J))
            mode = ImGuizmo::MODE::WORLD;

        if (ImGui::IsKeyPressed(ImGuiKey_K))
            mode = ImGuizmo::MODE::LOCAL;

        auto matrix = selectedModel->getTransform().modelMatrix;

        if (ImGuizmo::Manipulate(
            glm::value_ptr(scene->getCamera()->getViewMatrix()),
            glm::value_ptr(scene->getCamera()->getProjectionMatrix()),
            operation,
            mode,
            glm::value_ptr(matrix)
        )) {
            selectedModel->setTransform(matrix);
        }


        ImGui::SeparatorText("Properties");
        ImGui::BeginChild("Properties");
        ImGui::PushItemWidth(-FLT_MIN);

        auto name = selectedModel->getName();
        if (ImGui::InputText("Name:", &name, sizeof(name))) {
            selectedModel->setName(name);
        }

        Transform transform = selectedModel->getTransform();
        glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(transform.rotation));

        ImGui::Text("Position (x, y, z):");
        if (ImGui::DragFloat3("##modelPos", glm::value_ptr(transform.position), 0.01f)) {
            selectedModel->setTransform(transform);
        }

        if (!dynamic_cast<AreaModel*>(selectedModel)) {
            ImGui::Text("Rotation (x, y, z):");
            if (ImGui::DragFloat3("##modelRot", glm::value_ptr(eulerRotation), 0.01f)) {
                transform.rotation = glm::quat(glm::radians(eulerRotation));
                selectedModel->setTransform(transform);
            }
        }

        ImGui::Text("Scale (x, y, z):");
        if (ImGui::DragFloat3("##modelScl", glm::value_ptr(transform.scale), 0.01f)) {
            selectedModel->setTransform(transform);
        }

        if (auto forceField = dynamic_cast<ForceModel*>(selectedModel)) {
            ImGui::SeparatorText("Additional Properties");

            ImGui::Text("Force Strength:");
            if (ImGui::DragFloat("##ffStrength", &forceField->getForceField()->strength, 0.01f)) {
                forceField->notifyOnTransformChangedListeners();
            }
        }
        ImGui::EndChild();
    }

    if (ImGui::Button("Build Boundary Density Map", ImVec2(totalWidth, 0))) {
        editor->buildBoundaryDensityMap();
    }

    ImGui::End();
}
