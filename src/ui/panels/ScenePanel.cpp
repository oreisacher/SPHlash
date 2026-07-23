//
// Created by ruut on 4/13/26.
//

#include "../../../include/ui/panels/ScenePanel.h"
#include "ImGuiFileDialog.h"
#include "../../../include/ui/panels/PropertiesPanel.h"
#include "../../../include/core/Editor.h"
#include "../../../include/simulation/scene/Scene.h"
#include "imgui.h"

ScenePanel::ScenePanel(Editor* editor, Renderer* renderer)
    : UIPanel("Scene"), editor(editor), renderer(renderer)
{ }

void ScenePanel::preDraw(double deltaTime) { }

void ScenePanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // --- Scene ---
    ImGui::SeparatorText("Scene Graph");

    std::vector<SceneNode*> sceneNodes = editor->getScene()->getSceneNodes();

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
                if (isSelected) {
                    editor->setSelected(nullptr);
                } else {
                    editor->setSelected(sceneNodes[i]);
                }
            }

            if (isSelected)
                ImGui::PopStyleColor(3);

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                if (isSelected) {
                    editor->setSelected(nullptr);
                }
            }

            ImGui::TableNextColumn();

            if (sceneNodes[i]->isSceneGraphHideable()) {
                auto visibilityLabel = sceneNodes[i]->isVisible() ? "Hide" : "Show";
                if (ImGui::Button(visibilityLabel, ImVec2(0, 19))) {
                    sceneNodes[i]->setVisible(!sceneNodes[i]->isVisible());
                }
            }

            ImGui::TableNextColumn();
            if (sceneNodes[i]->isDeletable()) {
                if (ImGui::Button(" - ", ImVec2(0, 19))) {
                    if (isSelected) {
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

    // --- Particle Spawns ---
    if (ImGui::Button("Add Particle Spawn Cube", ImVec2(totalWidth, 0))) {
        editor->addParticleSpawn("");
    }

    if (ImGuiFileDialog::Instance()->Display("LoadParticleSpawnModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            editor->addParticleSpawn(ImGuiFileDialog::Instance()->GetFilePathName());
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Add Particle Spawn Model", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadParticleSpawnModel", "Choose Model", ".obj", config);
    }

    // --- Add (Load) Model ---
    if (ImGuiFileDialog::Instance()->Display("LoadModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            editor->addBoundary(ImGuiFileDialog::Instance()->GetFilePathName());
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGui::Button("Add Model", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadModel", "Choose Model", ".obj", config);
    }

    // --- Force Fields ---
    if (ImGui::Button("Add Force Field", ImVec2(totalWidth, 0))) {
        editor->addForceField();
    }

    // --- Load & Save File Dialogs ---
    if (ImGuiFileDialog::Instance()->Display("LoadScene")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            editor->getScene()->loadScene(filePathName);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("SaveScene")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            editor->getScene()->saveScene(filePathName);
        }

        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::End();
}

void ScenePanel::drawMenu() {
    if (ImGui::BeginMenu("Scene")) {
        if (ImGui::MenuItem("Load Scene")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("LoadScene", "Choose location", ".json", config);
        }

        if (ImGui::MenuItem("Save Scene")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("SaveScene", "Choose location", ".json", config);
        }

        if (ImGui::MenuItem("Reset Scene")) {
            editor->setSelected(nullptr);
            editor->getScene()->resetScene();
        }

        ImGui::EndMenu();
    }
}
