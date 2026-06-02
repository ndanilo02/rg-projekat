#include <engine/graphics/OpenGL.hpp>
#include <engine/graphics/PointShadow.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace engine::graphics {

void PointShadow::initialize(int resolution) {
    if (m_initialized) {
        destroy();
    }
    m_resolution = resolution;

    // Create depth cubemap texture
    CHECKED_GL_CALL(glGenTextures, 1, &m_depth_cubemap);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_CUBE_MAP, m_depth_cubemap);

    for (unsigned int i = 0; i < 6; ++i) {
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                        resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Attach depth texture as FBO's depth buffer
    CHECKED_GL_CALL(glGenFramebuffers, 1, &m_depth_map_fbo);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_depth_map_fbo);
    CHECKED_GL_CALL(glFramebufferTexture, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depth_cubemap, 0);
    CHECKED_GL_CALL(glDrawBuffer, GL_NONE);
    CHECKED_GL_CALL(glReadBuffer, GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("PointShadow Framebuffer not complete!");
    }

    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
    m_initialized = true;
}

void PointShadow::destroy() {
    if (m_depth_map_fbo != 0) CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_depth_map_fbo);
    if (m_depth_cubemap != 0) CHECKED_GL_CALL(glDeleteTextures, 1, &m_depth_cubemap);
    m_initialized = false;
}

void PointShadow::bind_for_rendering() {
    CHECKED_GL_CALL(glViewport, 0, 0, m_resolution, m_resolution);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_depth_map_fbo);
    CHECKED_GL_CALL(glClear, GL_DEPTH_BUFFER_BIT);
}

void PointShadow::unbind() {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void PointShadow::set_matrices(const resources::Shader *depth_shader, const glm::vec3 &light_pos, float near_plane, float far_plane) {
    m_far_plane = far_plane;

    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float) m_resolution / (float) m_resolution, near_plane, far_plane);

    std::vector<glm::mat4> shadowTransforms;
    shadowTransforms.push_back(shadowProj * glm::lookAt(light_pos, light_pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(light_pos, light_pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(light_pos, light_pos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

    depth_shader->use();
    for (int i = 0; i < 6; ++i) {
        depth_shader->set_mat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
    }
    depth_shader->set_vec3("lightPos", light_pos);
    depth_shader->set_float("far_plane", far_plane);
}

void PointShadow::bind_depth_map(int texture_unit) {
    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0 + texture_unit);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_CUBE_MAP, m_depth_cubemap);
}

}// namespace engine::graphics
