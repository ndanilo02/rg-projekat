#ifndef MATF_RG_PROJECT_POINT_SHADOW_HPP
#define MATF_RG_PROJECT_POINT_SHADOW_HPP

#include <engine/resources/Shader.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace engine::graphics {

/**
 * @class PointShadow
 * @brief Engine component for omnidirectional shadow mapping (Group B).
 */
class PointShadow {
public:
    /**
     * @brief Initializes the depth cubemap and framebuffer.
     * @param resolution The resolution of each face of the cubemap.
     */
    void initialize(int resolution = 1024);

    /**
     * @brief Cleans up OpenGL resources.
     */
    void destroy();

    /**
     * @brief Prepares the framebuffer for rendering depth.
     */
    void bind_for_rendering();

    /**
     * @brief Restores the viewport and unbinds the framebuffer.
     */
    void unbind();

    /**
     * @brief Computes the 6 view-projection matrices and sends them to the depth shader.
     * @param depth_shader The shader used for rendering depth.
     * @param light_pos The position of the point light.
     * @param near_plane The near plane of the light's projection.
     * @param far_plane The far plane of the light's projection.
     */
    void set_matrices(const resources::Shader *depth_shader, const glm::vec3 &light_pos, float near_plane, float far_plane);

    /**
     * @brief Binds the depth cubemap texture to a specified texture unit.
     */
    void bind_depth_map(int texture_unit);

    /**
     * @brief Gets the configured far plane.
     */
    float far_plane() const { return m_far_plane; }

    int resolution() const { return m_resolution; }

private:
    unsigned int m_depth_map_fbo{0};
    unsigned int m_depth_cubemap{0};
    int m_resolution{1024};
    float m_far_plane{25.0f};
    bool m_initialized{false};
};

}// namespace engine::graphics

#endif// MATF_RG_PROJECT_POINT_SHADOW_HPP
