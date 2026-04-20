//
// Created by ruut on 4/13/26.
//

#include "ModelsPanel.h"

#include <format>
#include <iostream>

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "glm/gtc/type_ptr.inl"

ModelsPanel::ModelsPanel()
    : UIPanel("Models") {
    renderer = FluidRenderer::getInstance();
}

void ModelsPanel::update(double deltaTime) {

}

void ModelsPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // --- Load Model ---
    if (ImGui::Button("Add Model", ImVec2(totalWidth, 0))) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadModel", "Choose Model", ".obj", config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadModel")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            renderer->addModel(filePathName.c_str());
        }

        ImGuiFileDialog::Instance()->Close();
    }

    // --- Scene ---
    ImGui::SeparatorText("Scene");

    std::vector<Model*> models = renderer->getModels();

    if (ImGui::BeginTable("Models", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

        for (int i = 0 ; i < models.size() ; i++) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::AlignTextToFramePadding();
            if (ImGui::Selectable(models[i]->getName().c_str(), selected == i, 0, ImVec2(0, 13))) {
                selected = i;
                selectedModel = models[i];
                selectedTransform = selectedModel->getTransform();
            }

            ImGui::TableNextColumn();
            ImGui::PushID(i);
            if (ImGui::Button(" - ", ImVec2(0, 19))) {
                if (selected == i) {
                    selected = -1;
                    selectedModel = nullptr;
                    selectedTransform = nullptr;
                }

                renderer->removeModel(i);
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    // --- Properties ---
    if (selectedModel) {
        ImGui::SeparatorText("Properties");

        ImGui::Text("Position (x, y, z):");
        if (ImGui::DragFloat3("##modelPos", glm::value_ptr(selectedTransform->position), 0.01f)) {
            selectedModel->updateTransform();
        }
        ImGui::Text("Rotation (x, y, z):");
        if (ImGui::DragFloat3("##modelRot", glm::value_ptr(selectedTransform->rotation), 0.01f)) {
            selectedModel->updateTransform();
        }
        ImGui::Text("Scale (x, y, z):");
        if (ImGui::DragFloat3("##modelScl", glm::value_ptr(selectedTransform->scale), 0.01f)) {
            selectedModel->updateTransform();
        }
    }

    ImGui::End();
}
