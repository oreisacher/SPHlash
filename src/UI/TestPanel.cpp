//
// Created by ruut on 3/14/26.
//

#include "TestPanel.h"
#include "imgui.h"

TestPanel::TestPanel(const std::string& name) : UIPanel(name) { }

void TestPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str(), &visible);

    ImGui::Text("This is a text panel");

    ImGui::End();
}
