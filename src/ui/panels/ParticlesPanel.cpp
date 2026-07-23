//
// Created by ruut on 3/24/26.
//

#include "../../../include/ui/panels/ParticlesPanel.h"
#include "../../../include/assets/Shader.h"
#include "../../../include/simulation/FluidSimulation.h"
#include "../../../include/simulation/scene/nodes/ParticlesNode.h"
#include "imgui.h"
#include <format>

#include "ImGuiFileDialog.h"
#include "implot.h"
#include "../../../include/core/Input.h"
#include "../../../include/utils/Export.h"

ParticlesPanel::ParticlesPanel(FluidSimulation* simulation, ParticlesNode* particles)
    : UIPanel("Particles"), simulation(simulation)
{
    particlesShader = particles->getParticleShader();
}

void ParticlesPanel::preDraw(double deltaTime) {
    if (Input::Get()->isKeyPressed(Window::Key::C)) {
#ifdef METRIC_DENSITY
        averageDensity.clear();
        minAvgDensity = INFINITY;
        maxAvgDensity = -INFINITY;

        averageDensityError.clear();
        minDensityError = INFINITY;
        maxDensityError = -INFINITY;

        densityStandardDeviation.clear();
        minDensityStandardDeviation = INFINITY;
        maxDensityStandardDeviation = -INFINITY;
#endif

#ifdef METRIC_AVG_HEIGHT
        averageTopSurfaceHeigth.clear();
#endif
    }

    if (simulation->getState() != RUNNING)
        return;

    FrameMetrics metric = simulation->getFrameMetrics();

#ifdef METRIC_DENSITY
    // --- Average Density ---
    averageDensity.push_back(metric.averageDensity);

    if (metric.averageDensity < minAvgDensity) minAvgDensity = metric.averageDensity;
    if (metric.averageDensity > maxAvgDensity) maxAvgDensity = metric.averageDensity;

    // --- Average Density Error ---
    float avgDensityError = metric.averageDensityError * 100.0f;
    averageDensityError.push_back(avgDensityError);

    if (avgDensityError < minDensityError) minDensityError = avgDensityError;
    if (avgDensityError > maxDensityError) maxDensityError = avgDensityError;

    // --- Density Standard Deviation ---
    densityStandardDeviation.push_back(metric.densityStandardDeviation);

    if (metric.densityStandardDeviation < minDensityStandardDeviation) minDensityStandardDeviation = metric.densityStandardDeviation;
    if (metric.densityStandardDeviation > maxDensityStandardDeviation) maxDensityStandardDeviation = metric.densityStandardDeviation;
#endif

#ifdef METRIC_AVG_HEIGHT
    // --- Average Top Surface Height ---
    averageTopSurfaceHeigth.push_back(metric.avgTopSurfaceHeight);
#endif

    // --- Particle Data ---
    time += deltaTime;

    if (time >= 0.1f) {
        time = 0.0f;

        // Fetch new data from simulation
        particle = simulation->getParticle(selectedParticle);
    }
}

void ParticlesPanel::draw() {
    if (!visible)
        return;

    ImGui::Begin(name.c_str());

    int particleCount = simulation->getParticleCount();

    if (particleCount == 0) {
        auto windowSize = ImGui::GetWindowSize();
        auto textSize   = ImGui::CalcTextSize("No particles.");

        ImGui::SetCursorPos({
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::Text("No particles.");
    } else {
        ImGui::PushItemWidth(-FLT_MIN);
        float totalWidth = ImGui::GetContentRegionAvail().x;

        ImGui::Text("Total Particles:");
        ImGui::Button(std::format("{}##tp", simulation->getParticleCount()).c_str(), ImVec2(-FLT_MIN, 0));

#ifdef METRIC_DENSITY
        ImGui::SeparatorText("");

        // --- Average Density ---
        if (ImGui::CollapsingHeader("Average Density")) {
            if (ImPlot::BeginPlot("##Average Density", ImVec2(-1, 200), ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxes("Frame", "Avg Density", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

                ImPlot::PlotLine("Average Density", averageDensity.data(), static_cast<int>(averageDensity.size()));

                ImPlotSpec spec;
                spec.Flags = ImPlotInfLinesFlags_Horizontal;
                ImPlot::PlotInfLines("Rest Density", &simulation->settings.p_0, 1, spec);

                ImPlot::EndPlot();
            }

            ImGui::Spacing();
            ImGui::Text("Min: %f", minAvgDensity);
            ImGui::SameLine(150);
            ImGui::Text("Max: %f", maxAvgDensity);

            if (ImGui::Button("Export CSV##avgDensity", ImVec2(totalWidth, 0))) {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("avgDensityExport", "Choose location", ".csv", config);
            }

            if (ImGuiFileDialog::Instance()->Display("avgDensityExport")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    Export::exportCSV(filePathName, averageDensity);
                }

                ImGuiFileDialog::Instance()->Close();
            }
        }

        // --- Average Density Error ---
        if (ImGui::CollapsingHeader("Average Density Error")) {
            if (ImPlot::BeginPlot("##Average Density Error", ImVec2(-1, 200), ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxes("Frame", "Error (%)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, static_cast<double>(maxDensityError) * 1.1, ImGuiCond_Always);
                ImPlot::SetupAxisFormat(ImAxis_Y1, "%.1f");

                ImPlot::PlotLine("Density Error", averageDensityError.data(), static_cast<int>(averageDensityError.size()));
                ImPlot::EndPlot();
            }

            ImGui::Spacing();
            ImGui::Text("Min: %.3f%%", minDensityError);
            ImGui::SameLine(150);
            ImGui::Text("Max: %.3f%%", maxDensityError);

            if (ImGui::Button("Export CSV##avgDensityError", ImVec2(totalWidth, 0))) {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("avgDensityErrorExport", "Choose location", ".csv", config);
            }

            if (ImGuiFileDialog::Instance()->Display("avgDensityErrorExport")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    Export::exportCSV(filePathName, averageDensityError);
                }

                ImGuiFileDialog::Instance()->Close();
            }
        }

        // --- Density Standard Deviation ---
        if (ImGui::CollapsingHeader("Density Standard Deviation")) {
            if (ImPlot::BeginPlot("##Density Standard Deviation", ImVec2(-1, 200), ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxes("Frame", "Standard Deviation", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

                ImPlot::PlotLine("Standard Deviation", densityStandardDeviation.data(), static_cast<int>(densityStandardDeviation.size()));
                ImPlot::EndPlot();
            }

            ImGui::Spacing();
            ImGui::Text("Min: %f", minDensityStandardDeviation);
            ImGui::SameLine(150);
            ImGui::Text("Max: %f", maxDensityStandardDeviation);

            if (ImGui::Button("Export CSV##densityStandardDeviationExport", ImVec2(totalWidth, 0))) {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("densityStandardDeviationExport", "Choose location", ".csv", config);
            }

            if (ImGuiFileDialog::Instance()->Display("densityStandardDeviationExport")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    Export::exportCSV(filePathName, densityStandardDeviation);
                }

                ImGuiFileDialog::Instance()->Close();
            }
        }
#endif

#ifdef METRIC_AVG_HEIGHT
        // --- Average Top Surface Height ---
        if (ImGui::CollapsingHeader("Average Top Surface Height")) {
            if (ImPlot::BeginPlot("##Average Top Surface Height", ImVec2(-1, 200), ImPlotFlags_NoLegend)) {
                ImPlot::SetupAxes("Frame", "Average Height", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

                ImPlot::PlotLine("Average Height", averageTopSurfaceHeigth.data(), static_cast<int>(averageTopSurfaceHeigth.size()));
                ImPlot::EndPlot();
            }

            if (ImGui::Button("Export CSV##averageTopSurfaceHeight", ImVec2(totalWidth, 0))) {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("averageTopSurfaceHeightExport", "Choose location", ".csv", config);
            }

            if (ImGuiFileDialog::Instance()->Display("averageTopSurfaceHeightExport")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    Export::exportCSV(filePathName, densityStandardDeviation);
                }

                ImGuiFileDialog::Instance()->Close();
            }
        }
#endif

        ImGui::SeparatorText("");

        ImGui::Checkbox("Show particle info", &showParticles);

        if (showParticles) {
            ImGui::Spacing();
            ImGui::Text("Particle ID:");
            ImGui::DragInt("##id", &selectedParticle, 1, 0, particleCount - 1, "%d", ImGuiSliderFlags_AlwaysClamp);

            particlesShader->bind();
            particlesShader->setInt("selectedParticleID", selectedParticle);

            if (particle.properties.w != -1.0) {
                float totalWidth = ImGui::GetContentRegionAvail().x;
                float spacing = ImGui::GetStyle().ItemSpacing.x;
                float itemWidth = (totalWidth - 2.0f * spacing) / 3.0f;

                ImGui::Separator();

                ImGui::Text("Position:");
                ImGui::Button(std::format("{:.3f}##posx", particle.position.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##posy", particle.position.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##posz", particle.position.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Text("Velocity:");
                ImGui::Button(std::format("{:.3f}##velx", particle.velocity.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##vely", particle.velocity.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##velz", particle.velocity.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Separator();

                ImGui::Text("Mass:");
                ImGui::Button(std::format("{:.3f}##mass", particle.properties.x).c_str(), ImVec2(-FLT_MIN, 0));

                ImGui::Text("Density:");
                ImGui::Button(std::format("{:.3f}##dens", particle.properties.y).c_str(), ImVec2(-FLT_MIN, 0));

                ImGui::Text("Pressure:");
                ImGui::Button(std::format("{:.3f}##pres", particle.properties.z).c_str(), ImVec2(-FLT_MIN, 0));

                ImGui::Separator();

                ImGui::Text("Pressure Force:");
                ImGui::Button(std::format("{:.3f}##pfx", particle.pressureForce.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##pfy", particle.pressureForce.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##pfz", particle.pressureForce.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Text("Viscosity Force:");
                ImGui::Button(std::format("{:.3f}##vfx", particle.viscosityForce.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##vfy", particle.viscosityForce.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##vfz", particle.viscosityForce.z).c_str(), ImVec2(itemWidth, 0));

                ImGui::Text("Surface Tension Force:");
                ImGui::Button(std::format("{:.3f}##stfx", particle.surfaceTensionForce.x).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##stfy", particle.surfaceTensionForce.y).c_str(), ImVec2(itemWidth, 0)); ImGui::SameLine();
                ImGui::Button(std::format("{:.3f}##stfz", particle.surfaceTensionForce.z).c_str(), ImVec2(itemWidth, 0));
            } else {
                selectedParticle = 0;
            }
        } else {
            particlesShader->bind();
            particlesShader->setInt("selectedParticleID", -1);
        }
    }

    ImGui::End();
}
