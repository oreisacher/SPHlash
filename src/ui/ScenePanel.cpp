//
// Created by ruut on 4/13/26.
//

#include "ScenePanel.h"
#include <format>
#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "PropertiesPanel.h"
#include "rendering/BoundingBoxNode.h"
#include "rendering/Editor.h"
#include "rendering/Scene.h"

ScenePanel::ScenePanel(Scene& scene, Editor* editor, FluidRenderer* renderer)
    : UIPanel("Scene"), editor(editor), renderer(renderer), scene(&scene) {
}

void ScenePanel::update(double deltaTime) { }

void ScenePanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // --- Scene ---
    ImGui::SeparatorText("Scene Graph");

    std::vector<SceneNode*> sceneNodes = scene->getSceneNodes();

    if (ImGui::BeginTable("##sceneNodes", 3, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

        for (int i = 0 ; i < sceneNodes.size() ; i++) {
            if (!sceneNodes[i]->isSceneGraphVisible())
                continue;

            ImGui::TableNextRow();
            ImGui::PushID(i);

            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();

            bool isSelected = (sceneNodes[i] == editor->getSelected());

            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.60f, 0.28f, 0.28f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.35f, 0.35f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.80f, 0.42f, 0.42f, 1.00f));
            }

            if (ImGui::Button(sceneNodes[i]->getName().c_str(), ImVec2(-FLT_MIN, 19))) {
                if (selected != i) {
                    selected = i;
                    editor->setSelected(sceneNodes[i]);
                } else {
                    selected = -1;
                    editor->setSelected(nullptr);
                }
            }

            if (isSelected)
                ImGui::PopStyleColor(3);

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                if (isSelected) {
                    selected = -1;
                    editor->setSelected(nullptr);
                }
            }

            ImGui::TableNextColumn();

            auto visibilityLabel = sceneNodes[i]->isVisible() ? "Hide" : "Show";
            if (ImGui::Button(visibilityLabel, ImVec2(0, 19))) {
                sceneNodes[i]->setVisible(!sceneNodes[i]->isVisible());
            }

            ImGui::TableNextColumn();
            if (sceneNodes[i]->isDeletable()) {
                if (ImGui::Button(" - ", ImVec2(0, 19))) {
                    if (isSelected) {
                        selected = -1;
                        editor->setSelected(nullptr);
                    }

                    editor->removeSceneNode(sceneNodes[i]);
                }
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::SeparatorText("");

    if (ImGuiFileDialog::Instance()->Display("LoadModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            editor->addBoundary(ImGuiFileDialog::Instance()->GetFilePathName());
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

    ImGui::End();
}