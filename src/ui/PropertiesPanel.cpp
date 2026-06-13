//
// Created by ruut on 5/31/26.
//

#include "PropertiesPanel.h"
#include "imgui.h"

PropertiesPanel::PropertiesPanel(Editor* editor) : UIPanel("Properties"), editor(editor) { }

void PropertiesPanel::preDraw(double deltaTime) {
    if (editor->getSelected() != selected) {
        selected = editor->getSelected();

        if (selected)
            properties = selected->buildProperties();
    }
}

void PropertiesPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());
    ImGui::PushItemWidth(-FLT_MIN);

    if (selected) {
        int sectionIndex = 0;
        for (int p = 0 ; p < properties.getPropertyCount() ; p++) {
            while (sectionIndex < properties.getSectionCount() && properties.getSection(sectionIndex).index == p) {
                ImGui::SeparatorText(properties.getSection(sectionIndex).name.c_str());
                sectionIndex++;
            }

            properties.getProperty(p)->draw();
        }
    } else {
        auto windowSize = ImGui::GetWindowSize();
        auto textSize   = ImGui::CalcTextSize("No object selected.");

        ImGui::SetCursorPos({
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}
