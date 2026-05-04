//
// Created by ruut on 4/13/26.
//

#include "BoundariesPanel.h"
#include <format>
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.inl"

BoundariesPanel::BoundariesPanel(Scene& scene, FluidRenderer* renderer)
    : UIPanel("Boundaries"), renderer(renderer), scene(&scene)
{ }

void BoundariesPanel::update(double deltaTime) { }

void BoundariesPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // --- Scene ---
    if (ImGui::CollapsingHeader("Scene")) {
        if (ImGuiFileDialog::Instance()->Display("LoadModel")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                scene->addModel(std::make_unique<Model>(filePathName.c_str()));
            }

            ImGuiFileDialog::Instance()->Close();
        }

        // --- Scene ---
        std::vector<Model*> models = scene->getModels();

        if (ImGui::BeginTable("Models", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

            for (int i = 0 ; i < models.size() ; i++) {
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
                    selected = i;
                    selectedModel = models[i];
                    selectedTransform = selectedModel->getTransformReference();
                }

                if (isSelected) {
                    ImGui::PopStyleColor(3);
                }

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    if (isSelected) {
                        selected = -1;
                        selectedModel = nullptr;
                        selectedTransform = nullptr;
                    }
                }

                ImGui::TableNextColumn();
                if (ImGui::Button(" - ", ImVec2(0, 19))) {
                    if (isSelected) {
                        selected = -1;
                        selectedModel = nullptr;
                        selectedTransform = nullptr;
                    }

                    scene->removeModel(i);
                }
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGui::Separator();

        if (ImGui::Button("Add Model", ImVec2(totalWidth, 0))) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("LoadModel", "Choose Model", ".obj", config);
        }

        // --- Properties ---
        if (selectedModel) {
            ImGui::Begin("Properties");
            ImGui::PushItemWidth(-FLT_MIN);

            glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(selectedTransform->rotation));

            ImGui::Text("Position (x, y, z):");
            if (ImGui::DragFloat3("##modelPos", glm::value_ptr(selectedTransform->position), 0.01f)) {
                selectedModel->updateTransform();
            }
            ImGui::Text("Rotation (x, y, z):");
            if (ImGui::DragFloat3("##modelRot", glm::value_ptr(eulerRotation), 0.01f)) {
                selectedTransform->rotation = glm::quat(glm::radians(eulerRotation));
                selectedModel->updateTransform();
            }
            ImGui::Text("Scale (x, y, z):");
            if (ImGui::DragFloat3("##modelScl", glm::value_ptr(selectedTransform->scale), 0.01f)) {
                selectedModel->updateTransform();
            }
            ImGui::End();
        }
    }

    // --- Setup Boundaries --
    if (ImGui::CollapsingHeader("Boundary Setup")) {
    }

    ImGui::End();
}
