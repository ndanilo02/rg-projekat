#include <app/GUIController.hpp>
#include <app/MainController.hpp>
#include <cstdlib>
#include <engine/core/Engine.hpp>
#include <engine/graphics/Bloom.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <engine/graphics/PointShadow.hpp>
#include <memory>
#include <spdlog/spdlog.h>

void MainController::initialize() {
    engine::graphics::OpenGL::enable_depth_testing();
    auto config = engine::util::Configuration::config();
    int width = config["window"]["width"];
    int height = config["window"]["height"];

    // Optimize bloom blur amount for integrated GPU fillrate
    m_bloom.blur_amount = 3;
    m_bloom.initialize(width, height);

    // Set point shadow map resolution to 256 for smooth soft shadows and huge integrated GPU performance boost
    m_point_shadow.initialize(256);

    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    camera->MovementSpeed = 40.0f;   // Increase movement speed for fast traversal
    camera->MouseSensitivity = 0.35f;// Increase mouse rotation sensitivity for faster turning

    // Set starting position and angle facing the welder and engine block as requested
    camera->Position = glm::vec3(36.90f, 52.78f, -86.85f);
    camera->Yaw = 107.15f;
    camera->Pitch = -21.90f;
    camera->rotate_camera(0.0f, 0.0f);// update internal vectors (front, right, up)

    // Set initial light position on the welding point
    m_point_light.position = m_welding_point;
    m_point_light.enabled = false;// Disabled by default until event starts

    // Initialize the overhead garage light as a beautiful downward spotlight
    m_garage_light.position = m_lamp_transform.pos + glm::vec3(0.0f, m_lamp_light_y_offset, 0.0f);
    m_garage_light.direction = glm::vec3(0.0f, -1.0f, 0.0f);// Pointing straight down
    m_garage_light.cut_off = glm::cos(glm::radians(25.0f));
    m_garage_light.outer_cut_off = glm::cos(glm::radians(35.0f));
    m_garage_light.ambient = glm::vec3(0.15f, 0.15f, 0.12f);
    m_garage_light.diffuse = glm::vec3(2.5f, 2.5f, 2.2f);
    m_garage_light.specular = glm::vec3(2.5f, 2.5f, 2.2f);
    m_garage_light.constant = 1.0f;
    m_garage_light.linear = 0.009f;
    m_garage_light.quadratic = 0.0004f;
    m_garage_light.enabled = true;

    // Initialize the cube model to have beautiful grass-green material colors
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto cube_model = resources->model("cube");
    cube_model->set_material_colors(glm::vec3(0.12f, 0.32f, 0.12f), glm::vec3(0.02f, 0.05f, 0.02f));
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
            // Greatly increase brightness (HDR/Bloom friendly)
            m_point_light.diffuse = glm::vec3(1.2f, 3.6f, 6.0f);
            m_point_light.specular = glm::vec3(1.2f, 3.6f, 6.0f);
            m_garage_light.enabled = false;// Turn off the ceiling light!
            spdlog::info("[EventChain] Welding started! Ceiling light turned off.");
        }
    }
    // Reset event chain: press R
    if (platform->key(engine::platform::KEY_R).state() == engine::platform::Key::State::JustPressed) {
        m_event_chain.state = EventChain::State::Idle;
        m_event_chain.timer = 0.0f;
        m_point_light.enabled = false;
        m_point_light.diffuse = glm::vec3(1.0f, 0.7f, 0.4f);
        m_point_light.specular = glm::vec3(1.0f, 0.8f, 0.5f);
        m_garage_light.enabled = true;// Turn back on the ceiling light!
        m_particles.clear();
        spdlog::info("[EventChain] Reset to Idle. Ceiling light turned on.");
    }
}

void MainController::update() {
    update_camera();
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    update_event_chain(platform->dt());

    // Dynamically position the point light at the welding point
    m_point_light.position = m_welding_point;
    m_garage_light.position = m_lamp_transform.pos + glm::vec3(0.0f, m_lamp_light_y_offset, 0.0f);
}

void MainController::update_event_chain(float dt) {
    if (m_event_chain.state == EventChain::State::LampOn) {
        m_event_chain.timer += dt;
        if (m_event_chain.timer >= 3.0f) {
            // After 3 seconds: change light color to intense glowing red/orange (welder heated up)
            m_point_light.diffuse = glm::vec3(6.0f, 1.8f, 0.0f);
            m_point_light.specular = glm::vec3(6.0f, 1.8f, 0.0f);
            m_event_chain.state = EventChain::State::ColorChanged;
            m_event_chain.timer = 0.0f;
            spdlog::info("[EventChain] Welder heated up / glowing RED!");
        }
    } else if (m_event_chain.state == EventChain::State::ColorChanged) {
        m_event_chain.timer += dt;
        if (m_event_chain.timer >= 2.0f) {
            // After 2 more seconds: spawn object
            m_event_chain.state = EventChain::State::ObjectSpawned;
            m_event_chain.timer = 0.0f;

            // Turn off the welder's point light as the welding fire has ended
            m_point_light.enabled = false;
            m_garage_light.enabled = true;// Turn back on the ceiling light!
            spdlog::info("[EventChain] Engine successfully repaired! Ceiling light turned on.");
        }
    }
    // Spawn sparks during welding
    if (m_event_chain.state == EventChain::State::LampOn || m_event_chain.state == EventChain::State::ColorChanged) {
        for (int i = 0; i < 5; ++i) {
            SparkParticle p;
            p.pos = m_welding_point + glm::vec3(
                                              ((float) rand() / RAND_MAX - 0.5f) * 0.4f,
                                              ((float) rand() / RAND_MAX - 0.5f) * 0.4f,
                                              ((float) rand() / RAND_MAX - 0.5f) * 0.4f);
            float vx = ((float) rand() / RAND_MAX - 0.5f) * 4.0f;
            float vy = ((float) rand() / RAND_MAX) * 6.0f + 2.0f;
            float vz = ((float) rand() / RAND_MAX - 0.5f) * 4.0f;
            p.vel = glm::vec3(vx, vy, vz);
            p.color = glm::vec3(2.5f, 1.6f, 0.2f) * (((float) rand() / RAND_MAX) * 0.5f + 0.5f);
            p.life = ((float) rand() / RAND_MAX) * 0.4f + 0.2f;
            m_particles.push_back(p);
        }
    }

    // Update active particles
    for (auto it = m_particles.begin(); it != m_particles.end();) {
        it->pos += it->vel * dt;
        it->vel.y -= 15.0f * dt;
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_particles.erase(it);
        } else {
            ++it;
        }
    }
}

void MainController::begin_draw() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    int width = platform->window()->width();
    int height = platform->window()->height();

    if (width != m_last_width || height != m_last_height) {
        m_last_width = width;
        m_last_height = height;
        m_bloom.resize(width, height);
    }

    bool welder_shadow_active = m_shadows_enabled && m_point_light.enabled;
    bool garage_shadow_active = m_shadows_enabled && !m_point_light.enabled && m_garage_light.enabled;

    if (welder_shadow_active) {
        m_point_shadow.bind_for_rendering();
        auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
        auto depth_shader = resources->shader("point_shadow_depth");
        m_point_shadow.set_matrices(depth_shader, m_point_light.position, 1.0f, 25.0f);
        draw_scene_depth(depth_shader);
        m_point_shadow.unbind();

        engine::graphics::OpenGL::set_viewport(width, height);
    } else if (garage_shadow_active) {
        m_point_shadow.bind_for_rendering();
        auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
        auto depth_shader = resources->shader("point_shadow_depth");
        // Use 80.0f as far plane so the ceiling light shadows can reach the floor (height is 58.8f)
        m_point_shadow.set_matrices(depth_shader, m_garage_light.position, 1.0f, 80.0f);
        draw_scene_depth(depth_shader);
        m_point_shadow.unbind();

        engine::graphics::OpenGL::set_viewport(width, height);
    } else {
        engine::graphics::OpenGL::set_viewport(width, height);
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

    // Draw particles
    if (!m_particles.empty()) {
        auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
        auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
        auto light_cube_shader = resources->shader("light_cube");
        light_cube_shader->use();
        light_cube_shader->set_mat4("projection", graphics->projection_matrix());
        light_cube_shader->set_mat4("view", graphics->camera()->view_matrix());
        auto cube = resources->model("cube");
        for (const auto &p: m_particles) {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, p.pos);
            m = glm::scale(m, glm::vec3(0.8f));// visible particle size
            light_cube_shader->set_mat4("model", m);

            // Fade color over lifetime
            glm::vec3 col = p.color * (p.life / 0.6f);
            light_cube_shader->set_vec3("lightColor", col);
            cube->draw(light_cube_shader);
        }
    }

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

    // Spotlight (garage ceiling light)
    shader->set_vec3("spotLight.position", m_garage_light.position);
    shader->set_vec3("spotLight.direction", m_garage_light.direction);
    shader->set_float("spotLight.cutOff", m_garage_light.cut_off);
    shader->set_float("spotLight.outerCutOff", m_garage_light.outer_cut_off);
    if (m_garage_light.enabled) {
        shader->set_vec3("spotLight.ambient", m_garage_light.ambient);
        shader->set_vec3("spotLight.diffuse", m_garage_light.diffuse);
        shader->set_vec3("spotLight.specular", m_garage_light.specular);
    } else {
        shader->set_vec3("spotLight.ambient", glm::vec3(0.0f));
        shader->set_vec3("spotLight.diffuse", glm::vec3(0.0f));
        shader->set_vec3("spotLight.specular", glm::vec3(0.0f));
    }
    shader->set_float("spotLight.constant", m_garage_light.constant);
    shader->set_float("spotLight.linear", m_garage_light.linear);
    shader->set_float("spotLight.quadratic", m_garage_light.quadratic);

    shader->set_float("material.shininess", 32.0f);

    shader->set_int("depthMap", 10);
    m_point_shadow.bind_depth_map(10);

    float active_far_plane = m_point_light.enabled ? 25.0f : 80.0f;
    shader->set_float("far_plane", active_far_plane);
    shader->set_bool("shadows", m_shadows_enabled && (m_point_light.enabled || m_garage_light.enabled));
    shader->set_bool("shadowsFromWelder", m_point_light.enabled);
}

static glm::mat4 get_model_matrix(const ModelTransform &t) {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, t.pos);
    if (t.rot.x != 0.0f) m = glm::rotate(m, glm::radians(t.rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    if (t.rot.y != 0.0f) m = glm::rotate(m, glm::radians(t.rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    if (t.rot.z != 0.0f) m = glm::rotate(m, glm::radians(t.rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    m = glm::scale(m, t.scale);
    return m;
}

void MainController::draw_scene() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto shader = resources->shader("model");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());
    set_light_uniforms(shader);

    // 1. Draw Room (Garage)
    auto room = resources->model("room");
    glm::mat4 room_matrix = get_model_matrix(m_room_transform);
    shader->set_mat4("model", room_matrix);
    room->draw(shader);

    // 1b. Draw Grass Ground (giant lawn)
    auto grass = resources->model("cube");
    glm::mat4 grass_matrix = get_model_matrix(m_grass_transform);
    shader->set_mat4("model", grass_matrix);
    grass->draw(shader);

    // 1c. Draw Ceiling Lamp
    auto lamp = resources->model("lamp");
    glm::mat4 lamp_matrix = get_model_matrix(m_lamp_transform);
    shader->set_mat4("model", lamp_matrix);
    lamp->draw(shader);

    // 2. Draw Welder (Mechanic/Tool)
    auto welder = resources->model("welder");
    glm::mat4 welder_matrix = get_model_matrix(m_welder_transform);
    shader->set_mat4("model", welder_matrix);
    welder->draw(shader);

    // 3. Draw Engine (Broken or Repaired depending on state)
    if (m_event_chain.state == EventChain::State::ObjectSpawned) {
        glm::mat4 engine_matrix = get_model_matrix(m_repaired_engine_transform);
        shader->set_mat4("model", engine_matrix);
        auto repaired = resources->model("repaired_engine");
        repaired->draw(shader);
    } else {
        glm::mat4 engine_matrix = get_model_matrix(m_engine_transform);
        shader->set_mat4("model", engine_matrix);
        auto broken = resources->model("broken_engine");
        broken->draw(shader);
    }
}

void MainController::draw_scene_depth(const engine::resources::Shader *depth_shader) {
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();

    // OPTIMIZATION: On integrated GPUs (like AMD Radeon Graphics), rendering high-poly models
    // like the 39MB welder 6 times per frame in the shadow pass is extremely heavy.
    // We only render the engine model, which casts the main shadows onto the floor/walls.
    // This maintains excellent performance (>60 FPS) while keeping point shadows fully active.

    // Render the engine block in shadow pass
    if (m_event_chain.state == EventChain::State::ObjectSpawned) {
        glm::mat4 engine_matrix = get_model_matrix(m_repaired_engine_transform);
        depth_shader->set_mat4("model", engine_matrix);
        auto repaired = resources->model("repaired_engine");
        repaired->draw(depth_shader);
    } else {
        glm::mat4 engine_matrix = get_model_matrix(m_engine_transform);
        depth_shader->set_mat4("model", engine_matrix);
        auto broken = resources->model("broken_engine");
        broken->draw(depth_shader);
    }

    // When the ceiling spotlight is active, the welder character is in the room and should cast shadows!
    if (!m_point_light.enabled) {
        auto welder = resources->model("welder");
        glm::mat4 welder_matrix = get_model_matrix(m_welder_transform);
        depth_shader->set_mat4("model", welder_matrix);
        welder->draw(depth_shader);
    }
}

void MainController::draw_light_cube() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto shader = resources->shader("light_cube");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());

    auto cube = resources->model("cube");

    // 1. Welder light cube (only when welding is active)
    if (m_point_light.enabled) {
        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, m_point_light.position);
        model_matrix = glm::scale(model_matrix, glm::vec3(0.1f));
        shader->set_mat4("model", model_matrix);
        shader->set_vec3("lightColor", m_point_light.diffuse);
        cube->draw(shader);
    }

    // 2. Garage ceiling bulb (always when active to make the lamp look like it's ON and glowing!)
    if (m_garage_light.enabled) {
        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, m_garage_light.position);
        model_matrix = glm::scale(model_matrix, m_lamp_bulb_scale);
        shader->set_mat4("model", model_matrix);

        // Multiply by 4.0 to make it bloom brilliantly!
        shader->set_vec3("lightColor", m_garage_light.diffuse * 4.0f);
        cube->draw(shader);
    }
}

void MainController::draw_skybox() {
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto shader = resources->shader("skybox");
    auto skybox_cube = resources->skybox("night");
    shader->use();
    shader->set_float("brightness", m_skybox_brightness);
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
