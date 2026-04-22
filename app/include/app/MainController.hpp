#ifndef APP_MAIN_CONTROLLER_HPP
#define APP_MAIN_CONTROLLER_HPP

#include <engine/core/Engine.hpp>
#include <engine/graphics/Bloom.hpp>
#include <glm/glm.hpp>

/**
 * @struct DirLight
 * @brief Directional light parameters for the scene (moonlight).
 */
struct DirLight {
    glm::vec3 direction{-0.2f, -1.0f, -0.3f};
    glm::vec3 ambient{0.05f, 0.05f, 0.1f};
    glm::vec3 diffuse{0.2f, 0.2f, 0.4f};
    glm::vec3 specular{0.3f, 0.3f, 0.5f};
};

/**
 * @struct PointLight
 * @brief Point light parameters for the scene (lamp/candle on the desk).
 */
struct PointLight {
    glm::vec3 position{0.0f, 2.0f, 0.0f};
    glm::vec3 ambient{0.1f, 0.08f, 0.05f};
    glm::vec3 diffuse{1.0f, 0.7f, 0.4f};
    glm::vec3 specular{1.0f, 0.8f, 0.5f};
    float constant{1.0f};
    float linear{0.09f};
    float quadratic{0.032f};
    bool enabled{true};
};

/**
 * @struct EventChain
 * @brief State machine for the event chain requirement.
 *
 * Event chain: [Press L] ---> (immediately) Turn on lamp
 *              ---> (after 3s) Change light color to red
 *              ---> (after 2s) Spawn a new object on the scene
 */
struct EventChain {
    enum class State {
        Idle,
        LampOn,
        ColorChanged,
        ObjectSpawned,
    };

    State state{State::Idle};
    float timer{0.0f};
    bool object_visible{false};
};

/**
 * @class MainController
 * @brief Main controller for the Abandoned Workshop scene. Handles rendering, camera, lighting and events.
 */
class MainController final : public engine::core::Controller {
public:
    std::string_view name() const override {
        return "app::MainController";
    }

    DirLight &dir_light() {
        return m_dir_light;
    }

    PointLight &point_light() {
        return m_point_light;
    }

    EventChain &event_chain() {
        return m_event_chain;
    }

    bool &bloom_enabled() {
        return m_bloom_enabled;
    }

    float &exposure() {
        return m_exposure;
    }

private:
    void initialize() override;

    bool loop() override;

    void poll_events() override;

    void update() override;

    void begin_draw() override;

    void draw() override;

    void end_draw() override;

    void update_camera();

    void update_event_chain(float dt);

    void set_light_uniforms(const engine::resources::Shader *shader);

    void draw_scene();

    void draw_skybox();

    void draw_light_cube();

    engine::graphics::Bloom m_bloom;
    bool m_bloom_enabled{true};
    float m_exposure{1.0f};

    DirLight m_dir_light;
    PointLight m_point_light;
    EventChain m_event_chain;

    bool m_cursor_enabled{true};
};
#endif//APP_MAIN_CONTROLLER_HPP
