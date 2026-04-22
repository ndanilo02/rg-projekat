#ifndef APP_GUI_CONTROLLER_HPP
#define APP_GUI_CONTROLLER_HPP

#include <engine/core/Engine.hpp>

/**
 * @class GUIController
 * @brief ImGui-based GUI controller for adjusting lighting and scene parameters.
 *
 * Toggle with F2. When enabled, shows sliders for directional and point light properties.
 */
class GUIController final : public engine::core::Controller {
public:
    std::string_view name() const override {
        return "app::GUIController";
    }

private:
    void initialize() override;

    void poll_events() override;

    void draw() override;
};
#endif//APP_GUI_CONTROLLER_HPP
