#ifndef APP_MAIN_CONTROLLER_HPP
#define APP_MAIN_CONTROLLER_HPP

#include <engine/core/Engine.hpp>
#include <engine/graphics/Bloom.hpp>
#include <engine/graphics/PointShadow.hpp>
#include <glm/glm.hpp>
#include <vector>

/**
 * @struct DirLight
 * @brief Directional light parameters for the scene (moonlight).
 */
struct DirLight {
    glm::vec3 direction{-0.2f, -1.0f, -0.3f};
    glm::vec3 ambient{0.1f, 0.1f, 0.15f};
    glm::vec3 diffuse{0.4f, 0.4f, 0.6f};
    glm::vec3 specular{0.5f, 0.5f, 0.7f};
};

struct ModelTransform {
    glm::vec3 pos{0.0f};
    glm::vec3 scale{1.0f};
    glm::vec3 rot{0.0f};// rotation degrees for X, Y, Z
};

struct SparkParticle {
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 color;
    float life;
};

/**
 * @struct PointLight
 * @brief Point light parameters for the scene (lamp/candle on the desk).
 */
struct PointLight {
    glm::vec3 position{0.0f, 2.0f, 0.0f};
    glm::vec3 ambient{0.2f, 0.16f, 0.1f};
    glm::vec3 diffuse{2.5f, 1.8f, 1.0f};
    glm::vec3 specular{2.5f, 2.0f, 1.2f};
    float constant{1.0f};
    float linear{0.009f};
    float quadratic{0.002f};
    bool enabled{false};
};

struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float cut_off{glm::cos(glm::radians(25.0f))};
    float outer_cut_off{glm::cos(glm::radians(35.0f))};

    glm::vec3 ambient{0.15f, 0.15f, 0.12f};
    glm::vec3 diffuse{2.5f, 2.5f, 2.2f};
    glm::vec3 specular{2.5f, 2.5f, 2.2f};

    float constant{1.0f};
    float linear{0.009f};
    float quadratic{0.0004f};
    bool enabled{true};
};

/**
 * @struct EventChain
 * @brief State machine for the event chain requirement.
 *
 * Event chain: [Press L] ---> (immediately) Turn on welder (point light, blue/cyan)
 *              ---> (after 3s) Welder heats up (light turns red/orange)
 *              ---> (after 2s) Engine successfully repaired (broken engine model swapped to repaired engine model)
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
};

/**
 * @class MainController
 * @brief Main controller for the Retro Garage Workshop scene. Handles rendering, camera, lighting and events.
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

    bool &shadows_enabled() {
        return m_shadows_enabled;
    }

    ModelTransform &room_transform() {
        return m_room_transform;
    }

    ModelTransform &grass_transform() {
        return m_grass_transform;
    }

    ModelTransform &engine_transform() {
        return m_engine_transform;
    }

    ModelTransform &repaired_engine_transform() {
        return m_repaired_engine_transform;
    }

    ModelTransform &welder_transform() {
        return m_welder_transform;
    }

    glm::vec3 &welding_point() {
        return m_welding_point;
    }

    SpotLight &garage_light() {
        return m_garage_light;
    }

    ModelTransform &lamp_transform() {
        return m_lamp_transform;
    }

    float &lamp_light_y_offset() {
        return m_lamp_light_y_offset;
    }

    glm::vec3 &lamp_bulb_scale() {
        return m_lamp_bulb_scale;
    }

    float &skybox_brightness() {
        return m_skybox_brightness;
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

    void draw_scene_depth(const engine::resources::Shader *depth_shader);

    void draw_skybox();

    void draw_light_cube();

    engine::graphics::Bloom m_bloom;
    bool m_bloom_enabled{true};
    float m_exposure{0.2f};

    engine::graphics::PointShadow m_point_shadow;
    bool m_shadows_enabled{true};

    DirLight m_dir_light;
    PointLight m_point_light;
    SpotLight m_garage_light;
    EventChain m_event_chain;

    ModelTransform m_room_transform{glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(5.0f), glm::vec3(0.0f)};
    ModelTransform m_grass_transform{glm::vec3(0.0f, -1.5f, 0.0f), glm::vec3(1000.0f, 0.01f, 1000.0f), glm::vec3(0.0f)};
    ModelTransform m_engine_transform{glm::vec3(37.5f, 22.4f, 14.6f), glm::vec3(0.14f), glm::vec3(0.0f, -44.0f, 0.0f)};
    ModelTransform m_repaired_engine_transform{glm::vec3(27.7f, 26.9f, 22.9f), glm::vec3(1.3f), glm::vec3(0.0f, -49.0f, 0.0f)};
    ModelTransform m_welder_transform{glm::vec3(16.55f, -0.95f, -4.1f), glm::vec3(22.0f), glm::vec3(0.0f, 47.0f, 0.0f)};
    ModelTransform m_lamp_transform{glm::vec3(9.700f, 58.800f, -21.700f), glm::vec3(10.000f), glm::vec3(0.000f)};
    float m_lamp_light_y_offset{-1.326f};
    glm::vec3 m_lamp_bulb_scale{12.500f, 0.510f, 0.910f};
    float m_skybox_brightness{0.3f};
    glm::vec3 m_welding_point{31.1f, 30.75f, 9.05f};

    std::vector<SparkParticle> m_particles;

    bool m_cursor_enabled{true};
    int m_last_width{0};
    int m_last_height{0};
};
#endif//APP_MAIN_CONTROLLER_HPP
