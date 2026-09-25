#include <app/GUIController.hpp>
#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <imgui.h>

void GUIController::initialize() {
    set_enable(false);
}

void GUIController::poll_events() {
    const auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KeyId::KEY_F2).state() == engine::platform::Key::State::JustPressed) {
        set_enable(!is_enabled());
    }
}

void GUIController::draw() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto camera = graphics->camera();
    auto main_controller = engine::core::Controller::get<MainController>();

    graphics->begin_gui();

    ImGui::Begin("Lighting & Scene Info");

    // Camera info
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto &c = *camera;
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("Yaw: %.2f, Pitch: %.2f", c.Yaw, c.Pitch);
    }

    // Directional Light
    if (ImGui::CollapsingHeader("Directional Light (Moonlight)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &dir_light = main_controller->dir_light();
        ImGui::SliderFloat3("Direction", &dir_light.direction.x, -1.0f, 1.0f);
        ImGui::ColorEdit3("Ambient", &dir_light.ambient.x);
        ImGui::ColorEdit3("Diffuse", &dir_light.diffuse.x);
        ImGui::ColorEdit3("Specular", &dir_light.specular.x);
        ImGui::SliderFloat("Skybox Brightness", &main_controller->skybox_brightness(), 0.0f, 1.0f);
    }

    // Point Light
    if (ImGui::CollapsingHeader("Welder / Worklight (Point)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &point_light = main_controller->point_light();
        ImGui::Checkbox("Enabled", &point_light.enabled);
        ImGui::SliderFloat3("Position", &point_light.position.x, -100.0f, 100.0f);
        ImGui::ColorEdit3("Ambient##Point", &point_light.ambient.x);
        ImGui::ColorEdit3("Diffuse##Point", &point_light.diffuse.x);
        ImGui::ColorEdit3("Specular##Point", &point_light.specular.x);
        ImGui::SliderFloat("Constant", &point_light.constant, 0.0f, 2.0f);
        ImGui::SliderFloat("Linear", &point_light.linear, 0.0f, 0.05f);
        ImGui::SliderFloat("Quadratic", &point_light.quadratic, 0.0f, 0.01f);
    }

    // Garage Overhead Light (Spot Light)
    if (ImGui::CollapsingHeader("Garage Overhead Light (Spot Light)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &garage_light = main_controller->garage_light();
        ImGui::Checkbox("Enabled##Garage", &garage_light.enabled);
        ImGui::SliderFloat3("Position##Garage", &garage_light.position.x, -100.0f, 100.0f);
        ImGui::SliderFloat3("Direction##Garage", &garage_light.direction.x, -1.0f, 1.0f);

        float inner_deg = glm::degrees(glm::acos(garage_light.cut_off));
        float outer_deg = glm::degrees(glm::acos(garage_light.outer_cut_off));
        if (ImGui::SliderFloat("Inner Angle (Deg)", &inner_deg, 1.0f, 89.0f)) {
            garage_light.cut_off = glm::cos(glm::radians(inner_deg));
        }
        if (ImGui::SliderFloat("Outer Angle (Deg)", &outer_deg, 1.0f, 89.0f)) {
            garage_light.outer_cut_off = glm::cos(glm::radians(outer_deg));
        }

        ImGui::ColorEdit3("Ambient##Garage", &garage_light.ambient.x);
        ImGui::ColorEdit3("Diffuse##Garage", &garage_light.diffuse.x);
        ImGui::ColorEdit3("Specular##Garage", &garage_light.specular.x);
        ImGui::SliderFloat("Constant##Garage", &garage_light.constant, 0.0f, 2.0f);
        ImGui::SliderFloat("Linear##Garage", &garage_light.linear, 0.0f, 0.05f);
        ImGui::SliderFloat("Quadratic##Garage", &garage_light.quadratic, 0.0f, 0.01f);
    }

    // Event Chain Info
    if (ImGui::CollapsingHeader("Event Chain", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &event_chain = main_controller->event_chain();
        const char *state_str = "Idle";
        switch (event_chain.state) {
            case EventChain::State::Idle: state_str = "Idle (Press 'L' to start)"; break;
            case EventChain::State::LampOn: state_str = "Welding (waiting 3s...)"; break;
            case EventChain::State::ColorChanged: state_str = "Cooling / Sparks (waiting 2s...)"; break;
            case EventChain::State::ObjectSpawned: state_str = "Repaired (Press 'R' to reset)"; break;
        }
        ImGui::Text("State: %s", state_str);
        if (event_chain.state != EventChain::State::Idle && event_chain.state != EventChain::State::ObjectSpawned) {
            ImGui::Text("Timer: %.1f", event_chain.timer);
        }
    }

    // Bloom Settings
    if (ImGui::CollapsingHeader("Bloom (HDR)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Bloom", &main_controller->bloom_enabled());
        ImGui::SliderFloat("Exposure", &main_controller->exposure(), 0.1f, 5.0f);
    }

    // Shadows
    if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Shadows", &main_controller->shadows_enabled());
    }

    ImGui::End();

    graphics->end_gui();
}
