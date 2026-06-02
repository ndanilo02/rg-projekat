#include <engine/graphics/Bloom.hpp>
#include <engine/graphics/OpenGL.hpp>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace engine::graphics {

void Bloom::initialize(int width, int height) {
    if (m_initialized) {
        destroy();
    }
    init_framebuffers(width, height);
    m_initialized = true;
}

void Bloom::destroy() {
    if (m_hdr_fbo != 0) {
        CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_hdr_fbo);
        m_hdr_fbo = 0;
    }
    if (m_rbo_depth != 0) {
        CHECKED_GL_CALL(glDeleteRenderbuffers, 1, &m_rbo_depth);
        m_rbo_depth = 0;
    }
    if (m_color_buffers[0] != 0) {
        CHECKED_GL_CALL(glDeleteTextures, 2, m_color_buffers);
        m_color_buffers[0] = 0;
        m_color_buffers[1] = 0;
    }
    if (m_pingpong_fbo[0] != 0) {
        CHECKED_GL_CALL(glDeleteFramebuffers, 2, m_pingpong_fbo);
        m_pingpong_fbo[0] = 0;
        m_pingpong_fbo[1] = 0;
    }
    if (m_pingpong_color_buffers[0] != 0) {
        CHECKED_GL_CALL(glDeleteTextures, 2, m_pingpong_color_buffers);
        m_pingpong_color_buffers[0] = 0;
        m_pingpong_color_buffers[1] = 0;
    }
    if (m_quad_vao != 0) {
        CHECKED_GL_CALL(glDeleteVertexArrays, 1, &m_quad_vao);
        m_quad_vao = 0;
    }
    if (m_quad_vbo != 0) {
        CHECKED_GL_CALL(glDeleteBuffers, 1, &m_quad_vbo);
        m_quad_vbo = 0;
    }

    m_initialized = false;
}

void Bloom::resize(int width, int height) {
    if (m_width != width || m_height != height) {
        initialize(width, height);
    }
}

void Bloom::init_framebuffers(int width, int height) {
    m_width = width;
    m_height = height;

    // 1. Configure HDR framebuffer
    CHECKED_GL_CALL(glGenFramebuffers, 1, &m_hdr_fbo);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_hdr_fbo);

    // Create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    CHECKED_GL_CALL(glGenTextures, 2, m_color_buffers);
    for (unsigned int i = 0; i < 2; i++) {
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_color_buffers[i]);
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);// we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_color_buffers[i], 0);
    }

    // Create and attach depth buffer (renderbuffer)
    CHECKED_GL_CALL(glGenRenderbuffers, 1, &m_rbo_depth);
    CHECKED_GL_CALL(glBindRenderbuffer, GL_RENDERBUFFER, m_rbo_depth);
    CHECKED_GL_CALL(glRenderbufferStorage, GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    CHECKED_GL_CALL(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo_depth);

    // Tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    CHECKED_GL_CALL(glDrawBuffers, 2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        spdlog::error("Framebuffer not complete!");
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    // 2. Ping-pong framebuffers for blurring
    CHECKED_GL_CALL(glGenFramebuffers, 2, m_pingpong_fbo);
    CHECKED_GL_CALL(glGenTextures, 2, m_pingpong_color_buffers);
    for (unsigned int i = 0; i < 2; i++) {
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_pingpong_color_buffers[i]);
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);// we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpong_color_buffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            spdlog::error("Pingpong Framebuffer not complete!");
    }
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void Bloom::bind_for_rendering() {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_hdr_fbo);
    CHECKED_GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Bloom::render_bloom(const resources::Shader *blur_shader, const resources::Shader *final_shader) {
    bool horizontal = true, first_iteration = true;
    blur_shader->use();

    // Disable depth testing since we are rendering screen-space quads
    OpenGL::disable_depth_testing();

    // 1. Blur the bright texture
    for (unsigned int i = 0; i < blur_amount; i++) {
        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[horizontal]);
        blur_shader->set_int("horizontal", horizontal);

        // Bind texture of other framebuffer (or scene if first iteration)
        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, first_iteration ? m_color_buffers[1] : m_pingpong_color_buffers[!horizontal]);

        render_quad();

        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    // 2. Combine and apply tone mapping
    CHECKED_GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    final_shader->use();

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_color_buffers[0]);// Scene color

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE1);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_pingpong_color_buffers[!horizontal]);// Blurred bright color

    final_shader->set_int("scene", 0);
    final_shader->set_int("bloomBlur", 1);
    final_shader->set_bool("bloom", true);
    final_shader->set_float("exposure", exposure);

    render_quad();

    // Re-enable depth testing for the next frame
    OpenGL::enable_depth_testing();
}

void Bloom::render_quad() {
    if (m_quad_vao == 0) {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                -1.0f,
                -1.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                -1.0f,
                0.0f,
                1.0f,
                0.0f,
        };
        // setup plane VAO
        CHECKED_GL_CALL(glGenVertexArrays, 1, &m_quad_vao);
        CHECKED_GL_CALL(glGenBuffers, 1, &m_quad_vbo);
        CHECKED_GL_CALL(glBindVertexArray, m_quad_vao);
        CHECKED_GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, m_quad_vbo);
        CHECKED_GL_CALL(glBufferData, GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        CHECKED_GL_CALL(glEnableVertexAttribArray, 0);
        CHECKED_GL_CALL(glVertexAttribPointer, 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        CHECKED_GL_CALL(glEnableVertexAttribArray, 1);
        CHECKED_GL_CALL(glVertexAttribPointer, 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    }
    CHECKED_GL_CALL(glBindVertexArray, m_quad_vao);
    CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
    CHECKED_GL_CALL(glBindVertexArray, 0);
}

}// namespace engine::graphics
