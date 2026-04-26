#include <app/GUIController.hpp>
#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <engine/graphics/Bloom.hpp>
#include <engine/graphics/PointShadow.hpp>
#include <memory>
#include <spdlog/spdlog.h>

void MainController::initialize() {
    engine::graphics::OpenGL::enable_depth_testing();
    auto config = engine::util::Configuration::config();
    int width = config["window"]["width"];
    int height = config["window"]["height"];
    m_bloom.initialize(width, height);
    m_point_shadow.initialize(1024);
}

bool MainController::loop() {
    const auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KeyId::KEY_ESCAPE).state() == engine::platform::Key::State::JustPressed) {
        return false;
    }
    return true;
}

void MainController::poll_events() {
    const auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KEY_F1).state() == engine::platform::Key::State::JustPressed) {
        m_cursor_enabled = !m_cursor_enabled;
        platform->set_enable_cursor(m_cursor_enabled);
    }
    // Event chain trigger: press L to start
    if (platform->key(engine::platform::KEY_L).state() == engine::platform::Key::State::JustPressed) {
        if (m_event_chain.state == EventChain::State::Idle) {
            m_event_chain.state = EventChain::State::LampOn;
            m_event_chain.timer = 0.0f;
            m_point_light.enabled = true;
            m_point_light.diffuse = glm::vec3(1.0f, 0.7f, 0.4f);
            m_point_light.specular = glm::vec3(1.0f, 0.8f, 0.5f);
            spdlog::info("[EventChain] Lamp turned ON!");
        }
    }
    // Reset event chain: press R
    if (platform->key(engine::platform::KEY_R).state() == engine::platform::Key::State::JustPressed) {
        m_event_chain.state = EventChain::State::Idle;
        m_event_chain.timer = 0.0f;
        m_event_chain.object_visible = false;
        m_point_light.enabled = false;
        m_point_light.diffuse = glm::vec3(1.0f, 0.7f, 0.4f);
        m_point_light.specular = glm::vec3(1.0f, 0.8f, 0.5f);
        spdlog::info("[EventChain] Reset to Idle.");
    }
}

void MainController::update() {
    update_camera();
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    update_event_chain(platform->dt());
}

void MainController::update_event_chain(float dt) {
    if (m_event_chain.state == EventChain::State::LampOn) {
        m_event_chain.timer += dt;
        if (m_event_chain.timer >= 3.0f) {
            // After 3 seconds: change light color to red
            m_point_light.diffuse = glm::vec3(1.0f, 0.1f, 0.1f);
            m_point_light.specular = glm::vec3(1.0f, 0.2f, 0.2f);
            m_event_chain.state = EventChain::State::ColorChanged;
            m_event_chain.timer = 0.0f;
            spdlog::info("[EventChain] Light color changed to RED!");
        }
    } else if (m_event_chain.state == EventChain::State::ColorChanged) {
        m_event_chain.timer += dt;
        if (m_event_chain.timer >= 2.0f) {
            // After 2 more seconds: spawn object
            m_event_chain.object_visible = true;
            m_event_chain.state = EventChain::State::ObjectSpawned;
            m_event_chain.timer = 0.0f;
            spdlog::info("[EventChain] Object spawned on scene!");
        }
    }
}

void MainController::begin_draw() {
    if (m_shadows_enabled && m_point_light.enabled) {
        m_point_shadow.bind_for_rendering();
        auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
        auto depth_shader = resources->shader("point_shadow_depth");
        m_point_shadow.set_matrices(depth_shader, m_point_light.position, 1.0f, m_far_plane);
        draw_scene_depth(depth_shader);
        m_point_shadow.unbind();
        
        auto config = engine::util::Configuration::config();
        engine::graphics::OpenGL::set_viewport(config["window"]["width"].get<int>(), config["window"]["height"].get<int>());
    }

    if (m_bloom_enabled) {
        m_bloom.bind_for_rendering();
    } else {
        engine::graphics::OpenGL::clear_buffers();
    }
}

void MainController::draw() {
    draw_scene();
    draw_light_cube();
    draw_skybox();
    
    if (m_bloom_enabled) {
        auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
        auto blur_shader = resources->shader("bloom_blur");
        auto final_shader = resources->shader("bloom_final");
        m_bloom.exposure = m_exposure;
        m_bloom.render_bloom(blur_shader, final_shader);
    }
}

void MainController::end_draw() {
    engine::core::Controller::get<engine::platform::PlatformController>()->swap_buffers();
}

void MainController::set_light_uniforms(const engine::resources::Shader *shader) {
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    shader->set_vec3("viewPos", camera->Position);

    // Directional light (moonlight)
    shader->set_vec3("dirLight.direction", m_dir_light.direction);
    shader->set_vec3("dirLight.ambient", m_dir_light.ambient);
    shader->set_vec3("dirLight.diffuse", m_dir_light.diffuse);
    shader->set_vec3("dirLight.specular", m_dir_light.specular);

    // Point light (lamp)
    shader->set_vec3("pointLight.position", m_point_light.position);
    if (m_point_light.enabled) {
        shader->set_vec3("pointLight.ambient", m_point_light.ambient);
        shader->set_vec3("pointLight.diffuse", m_point_light.diffuse);
        shader->set_vec3("pointLight.specular", m_point_light.specular);
    } else {
        shader->set_vec3("pointLight.ambient", glm::vec3(0.0f));
        shader->set_vec3("pointLight.diffuse", glm::vec3(0.0f));
        shader->set_vec3("pointLight.specular", glm::vec3(0.0f));
    }
    shader->set_float("pointLight.constant", m_point_light.constant);
    shader->set_float("pointLight.linear", m_point_light.linear);
    shader->set_float("pointLight.quadratic", m_point_light.quadratic);

    shader->set_float("material.shininess", 32.0f);

    shader->set_int("depthMap", 2);
    m_point_shadow.bind_depth_map(2);
    shader->set_float("far_plane", m_far_plane);
    shader->set_bool("shadows", m_shadows_enabled && m_point_light.enabled);
}

void MainController::draw_scene() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto shader = resources->shader("model");
    auto model = resources->model("workshop");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, 0.0f, 0.0f));
    model_matrix = glm::scale(model_matrix, glm::vec3(1.0f));
    shader->set_mat4("model", model_matrix);

    set_light_uniforms(shader);
    model->draw(shader);

    // Draw event chain object if visible
    if (m_event_chain.object_visible) {
        auto event_model = resources->model("event_object");
        glm::mat4 event_matrix = glm::mat4(1.0f);
        event_matrix = glm::translate(event_matrix, glm::vec3(2.0f, 0.5f, 0.0f));
        event_matrix = glm::scale(event_matrix, glm::vec3(0.5f));
        shader->set_mat4("model", event_matrix);
        event_model->draw(shader);
    }
}

void MainController::draw_scene_depth(const engine::resources::Shader *depth_shader) {
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto model = resources->model("workshop");

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, 0.0f, 0.0f));
    model_matrix = glm::scale(model_matrix, glm::vec3(1.0f));
    depth_shader->set_mat4("model", model_matrix);
    model->draw(depth_shader);

    if (m_event_chain.object_visible) {
        auto event_model = resources->model("event_object");
        glm::mat4 event_matrix = glm::mat4(1.0f);
        event_matrix = glm::translate(event_matrix, glm::vec3(2.0f, 0.5f, 0.0f));
        event_matrix = glm::scale(event_matrix, glm::vec3(0.5f));
        depth_shader->set_mat4("model", event_matrix);
        event_model->draw(depth_shader);
    }
}

void MainController::draw_light_cube() {
    if (!m_point_light.enabled) {
        return;
    }
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto shader = resources->shader("light_cube");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, m_point_light.position);
    model_matrix = glm::scale(model_matrix, glm::vec3(0.1f));
    shader->set_mat4("model", model_matrix);
    shader->set_vec3("lightColor", m_point_light.diffuse);

    auto cube = resources->model("cube");
    cube->draw(shader);
}

void MainController::draw_skybox() {
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto shader = resources->shader("skybox");
    auto skybox_cube = resources->skybox("skybox");
    engine::core::Controller::get<engine::graphics::GraphicsController>()->draw_skybox(shader, skybox_cube);
}

void MainController::update_camera() {
    auto gui = engine::core::Controller::get<GUIController>();
    if (gui->is_enabled()) {
        return;
    }
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    float dt = platform->dt();
    if (platform->key(engine::platform::KEY_W).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::FORWARD, dt);
    }
    if (platform->key(engine::platform::KEY_S).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::BACKWARD, dt);
    }
    if (platform->key(engine::platform::KEY_A).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::LEFT, dt);
    }
    if (platform->key(engine::platform::KEY_D).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::RIGHT, dt);
    }
    auto mouse = platform->mouse();
    camera->rotate_camera(mouse.dx, mouse.dy);
    camera->zoom(mouse.scroll);
}
