//
// Created by ruut on 7/27/26.
//

#include "ui/panels/QueryDataPanel.h"
#include "imgui.h"
#include "implot.h"
#include "core/Editor.h"

QueryDataPanel::QueryDataPanel(Editor* editor) : UIPanel("Query Data"), _editor(editor) { }

void QueryDataPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str(), &visible);
    ImGui::PushItemWidth(-FLT_MIN);

    auto queryNodes = _editor->getScene()->getQueryNodes();

    if (queryNodes.size() == 0) {
        auto windowSize = ImGui::GetWindowSize();
        auto textSize   = ImGui::CalcTextSize("No query nodes in scene.");

        ImGui::SetCursorPos({
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::Text("No query nodes in scene.");
        ImGui::End();
        return;
    }

    if (_selectedNode >= queryNodes.size()) {
        _selectedNode = 0;
    }

    ImGui::Text("Query Node:");
    if (ImGui::BeginCombo("##Query Node", queryNodes[_selectedNode]->getName().c_str())) {
        for (int i = 0 ; i < queryNodes.size(); i++) {
            ImGui::PushID(i);
            if (ImGui::Selectable(queryNodes[i]->getName().c_str(), _selectedNode == i))
                _selectedNode = i;

            if (_selectedNode == i)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    ImGui::SeparatorText("");

    auto& history = queryNodes[_selectedNode]->getHistory();

    // Query Particle Count
    if (ImGui::CollapsingHeader("Query Particle Count")) {
        plotFloatMetric("##QueryParticleCount", "Query Particles", history, [] (const Metrics& m) { return (float)m.count; });
    }

    // Average Position
    if (ImGui::CollapsingHeader("Position")) {
        plotVectorMetric("##AvgPosition", "Average Position", history, [] (const Metrics& m) { return m.position.avg; });
        plotVectorMetric("##MinPosition", "Min Position", history, [] (const Metrics& m) { return m.position.min; });
        plotVectorMetric("##MaxPosition", "Max Position", history, [] (const Metrics& m) { return m.position.max; });
    }

    // Average Velocity
    if (ImGui::CollapsingHeader("Velocity")) {
        plotVectorMetric("##AvgVelocity", "Average Velocity", history, [] (const Metrics& m) { return m.velocity.avg; });
        plotVectorMetric("##MinVelocity", "Min Velocity", history, [] (const Metrics& m) { return m.velocity.min; });
        plotVectorMetric("##MaxVelocity", "Max Velocity", history, [] (const Metrics& m) { return m.velocity.max; });
    }

    // Density
    if (ImGui::CollapsingHeader("Density")) {
        plotFloatMetric("##AvgDensity", "Average Density", history, [] (const Metrics& m) { return m.density.avg; });
        plotFloatMetric("##MinDensity", "Min Density", history, [] (const Metrics& m) { return m.density.min; });
        plotFloatMetric("##MaxDensity", "Max Density", history, [] (const Metrics& m) { return m.density.max; });
    }

    if (ImGui::CollapsingHeader("Density Error")) {
        plotFloatMetric("##AvgDensityError", "Average Error (%)", history, [] (const Metrics& m) { return m.densityError.avg; });
        plotFloatMetric("##MinDensityError", "Min Error (%)", history, [] (const Metrics& m) { return m.densityError.min; });
        plotFloatMetric("##MaxDensityError", "Max Error (%)", history, [] (const Metrics& m) { return m.densityError.max; });
    }

    // Pressure
    if (ImGui::CollapsingHeader("Pressure")) {
        plotFloatMetric("##AvgPressure", "Average Pressure", history, [] (const Metrics& m) { return m.pressure.avg; });
        plotFloatMetric("##MinPressure", "Min Pressure", history, [] (const Metrics& m) { return m.pressure.min; });
        plotFloatMetric("##MaxPressure", "Max Pressure", history, [] (const Metrics& m) { return m.pressure.max; });
    }

    // Average Pressure Force
    if (ImGui::CollapsingHeader("Pressure Force")) {
        plotVectorMetric("##AvgPressure Force", "Average Pressure Force", history, [] (const Metrics& m) { return m.pressureForce.avg; });
        plotVectorMetric("##MinPressure Force", "Min Pressure Force", history, [] (const Metrics& m) { return m.pressureForce.min; });
        plotVectorMetric("##MaxPressure Force", "Max Pressure Force", history, [] (const Metrics& m) { return m.pressureForce.max; });
    }

    // Average Viscosity Force
    if (ImGui::CollapsingHeader("Viscosity Force")) {
        plotVectorMetric("##AvgViscosity Force", "Average Viscosity Force", history, [] (const Metrics& m) { return m.viscosityForce.avg; });
        plotVectorMetric("##MinViscosity Force", "Min Viscosity Force", history, [] (const Metrics& m) { return m.viscosityForce.min; });
        plotVectorMetric("##MaxViscosity Force", "Max Viscosity Force", history, [] (const Metrics& m) { return m.viscosityForce.max; });
    }

    // Average Surface Tension Force
    if (ImGui::CollapsingHeader("Surface Tension Force")) {
        plotVectorMetric("##AvgSurface Tension Force", "Average Surface Tension Force", history, [] (const Metrics& m) { return m.surfaceTensionForce.avg; });
        plotVectorMetric("##MinSurface Tension Force", "Min Surface Tension Force", history, [] (const Metrics& m) { return m.surfaceTensionForce.min; });
        plotVectorMetric("##MaxSurface Tension Force", "Max Surface Tension Force", history, [] (const Metrics& m) { return m.surfaceTensionForce.max; });
    }

    ImGui::End();
}

void QueryDataPanel::plotVectorMetric(const char* id, const char* label, const std::vector<Metrics>& history, VectorMetricGetter getter) {
    if (ImPlot::BeginPlot(id, ImVec2(-1, 200))) {
        ImPlot::SetupAxes(
            "Frame",
            label,
            ImPlotAxisFlags_AutoFit,
            ImPlotAxisFlags_AutoFit
        );

        MetricData mData {
            .history = &history,
            .getterVector = getter
        };

        ImPlot::PlotLineG(
            "X",
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getterVector(metric).x
                );
            },
            &mData,
            history.size()
        );

        ImPlot::PlotLineG(
            "Y",
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getterVector(metric).y
                );
            },
            &mData,
            history.size()
        );

        ImPlot::PlotLineG(
            "Z",
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getterVector(metric).z
                );
            },
            &mData,
            history.size()
        );

        if (ImPlot::IsPlotHovered()) {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();

            double x = std::round(mouse.x);

            ImPlot::DragLineX(
                0,
                &x,
                ImVec4(1, 1, 1, 0.5f),
                1.0f
            );

            int index = static_cast<int>(x);

            if (index >= 0 && index < static_cast<int>(history.size())) {
                glm::vec3 value = getter(history[index]);

                ImPlot::Annotation(
                    x,
                    value.x,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "X: %.3f",
                    value.x
                );

                ImPlot::Annotation(
                    x,
                    value.y,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "Y: %.3f",
                    value.y
                );

                ImPlot::Annotation(
                    x,
                    value.z,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "Z: %.3f",
                    value.z
                );
            }
        }

        ImPlot::EndPlot();
    }
}

void QueryDataPanel::plotFloatMetric(const char* id, const char* label, const std::vector<Metrics>& history, FloatMetricGetter getter) {
    if (ImPlot::BeginPlot(id, ImVec2(-1, 200), ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes(
            "Frame",
            label,
            ImPlotAxisFlags_AutoFit,
            ImPlotAxisFlags_AutoFit
        );

        MetricData mData {
            .history = &history,
            .getter = getter
        };

        ImPlot::PlotLineG(
            label,
            [](int idx, void* data) -> ImPlotPoint {
                auto* mData = static_cast<MetricData*>(data);
                const Metrics& metric = (*mData->history)[idx];

                return ImPlotPoint(
                    idx,
                    mData->getter(metric)
                );
            },
            &mData,
            history.size()
        );

        if (ImPlot::IsPlotHovered()) {
            ImPlotPoint mouse = ImPlot::GetPlotMousePos();

            // Vertical cursor line
            double x = mouse.x;
            ImPlot::DragLineX(
                0,
                &x,
                ImVec4(1, 1, 1, 0.5f),
                1.0f
            );

            // Convert x to index
            int index = static_cast<int>(std::round(x));

            if (index >= 0 && index < history.size()) {
                float value = getter(history[index]);

                ImPlot::Annotation(
                    x,
                    value,
                    ImVec4(1,1,1,1),
                    ImVec2(10,-10),
                    true,
                    "%.3f",
                    value
                );
            }
        }

        ImPlot::EndPlot();
    }
}
