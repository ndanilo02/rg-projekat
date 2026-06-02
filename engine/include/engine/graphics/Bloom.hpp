#ifndef MATF_RG_PROJECT_BLOOM_HPP
#define MATF_RG_PROJECT_BLOOM_HPP

#include <engine/resources/Shader.hpp>

namespace engine::graphics {

/**
 * @class Bloom
 * @brief Engine component for rendering Bloom (Group A) effect using HDR and Framebuffers.
 */
class Bloom {
public:
    /**
     * @brief Initializes the framebuffers and textures required for the Bloom effect.
     * @param width The screen width.
     * @param height The screen height.
     */
    void initialize(int width, int height);

    /**
     * @brief Cleans up the OpenGL resources.
     */
    void destroy();

    /**
     * @brief Binds the HDR framebuffer to capture the scene rendering.
     * Should be called before rendering the main scene.
     */
    void bind_for_rendering();

    /**
     * @brief Processes the captured scene, applies Gaussian blur to the bright areas, 
     * and renders the final composite to the default framebuffer.
     * @param blur_shader The shader used for Gaussian blur.
     * @param final_shader The shader used for final composition and HDR tone mapping.
     */
    void render_bloom(const resources::Shader *blur_shader, const resources::Shader *final_shader);

    /**
     * @brief Resizes the framebuffers if the window size changes.
     */
    void resize(int width, int height);

    float exposure{1.0f};
    int blur_amount{10};

private:
    void init_framebuffers(int width, int height);

    unsigned int m_hdr_fbo{0};
    unsigned int m_color_buffers[2]{0, 0};
    unsigned int m_rbo_depth{0};

    unsigned int m_pingpong_fbo[2]{0, 0};
    unsigned int m_pingpong_color_buffers[2]{0, 0};

    unsigned int m_quad_vao{0};
    unsigned int m_quad_vbo{0};
    void render_quad();

    int m_width{0};
    int m_height{0};
    bool m_initialized{false};
};

}// namespace engine::graphics

#endif// MATF_RG_PROJECT_BLOOM_HPP
