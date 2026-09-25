/**
 * @file Mesh.hpp
 * @brief Defines the Mesh class that serves as the interface for mesh rendering and storing processed assimp scenes.
 */

#ifndef MATF_RG_PROJECT_MESH_HPP
#define MATF_RG_PROJECT_MESH_HPP

#include <engine/resources/Texture.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace engine::resources {
/**
* @struct Vertex
* @brief Represents a vertex in the mesh.
*/
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

/**
* @class Mesh
* @brief Represents a mesh in the model in the OpenGL context.
*/
class Mesh {
    friend class AssimpSceneProcessor;

public:
    /**
    * @brief Draws the mesh using a given shader. Called by the @ref Model::draw function to draw all the meshes in the model.
    * @param shader The shader to use for drawing.
    */
    void draw(const Shader *shader);

    /**
    * @brief Destroys the mesh in the OpenGL context.
    */
    void destroy();

    void set_material_colors(glm::vec3 diffuse, glm::vec3 specular) {
        m_diffuse_color = diffuse;
        m_specular_color = specular;
    }

private:
    /**
    * @brief Constructs a Mesh object.
    * @param vertices The vertices in the mesh.
    * @param indices The indices in the mesh.
    * @param textures The textures in the mesh.
     */
    Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices,
         std::vector<Texture *> textures, glm::vec3 diffuse_color, glm::vec3 specular_color);

    uint32_t m_vao{0};
    uint32_t m_num_indices{0};
    std::vector<Texture *> m_textures;
    glm::vec3 m_diffuse_color;
    glm::vec3 m_specular_color;
};
}// namespace engine::resources

#endif//MATF_RG_PROJECT_MESH_HPP
