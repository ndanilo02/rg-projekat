
// clang-format off
#include <glad/glad.h>
// clang-format on
#include <engine/graphics/OpenGL.hpp>
#include <engine/resources/Mesh.hpp>
#include <engine/resources/Shader.hpp>
#include <engine/util/Utils.hpp>
#include <unordered_map>

namespace engine::resources {

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices,
           std::vector<Texture *> textures, glm::vec3 diffuse_color, glm::vec3 specular_color) {
    // NOLINTBEGIN
    static_assert(std::is_trivial_v<Vertex>);
    uint32_t VAO, VBO, EBO;
    CHECKED_GL_CALL(glGenVertexArrays, 1, &VAO);
    CHECKED_GL_CALL(glGenBuffers, 1, &VBO);
    CHECKED_GL_CALL(glGenBuffers, 1, &EBO);

    CHECKED_GL_CALL(glBindVertexArray, VAO);
    CHECKED_GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, VBO);
    CHECKED_GL_CALL(glBufferData, GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);

    CHECKED_GL_CALL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, EBO);
    CHECKED_GL_CALL(glBufferData, GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);

    CHECKED_GL_CALL(glEnableVertexAttribArray, 0);
    CHECKED_GL_CALL(glVertexAttribPointer, 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, Position));

    CHECKED_GL_CALL(glEnableVertexAttribArray, 1);
    CHECKED_GL_CALL(glVertexAttribPointer, 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, Normal));

    CHECKED_GL_CALL(glEnableVertexAttribArray, 2);
    CHECKED_GL_CALL(glVertexAttribPointer, 2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, TexCoords));

    CHECKED_GL_CALL(glEnableVertexAttribArray, 3);
    CHECKED_GL_CALL(glVertexAttribPointer, 3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, Tangent));

    CHECKED_GL_CALL(glEnableVertexAttribArray, 4);
    CHECKED_GL_CALL(glVertexAttribPointer, 4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, Bitangent));

    CHECKED_GL_CALL(glBindVertexArray, 0);
    // NOLINTEND
    m_vao = VAO;
    m_num_indices = indices.size();
    m_textures = std::move(textures);
    m_diffuse_color = diffuse_color;
    m_specular_color = specular_color;
}

void Mesh::draw(const Shader *shader) {
    shader->set_vec3("material.diffuse_color", m_diffuse_color);
    shader->set_vec3("material.specular_color", m_specular_color);
    static uint32_t default_white_texture_id = 0;
    if (default_white_texture_id == 0) {
        CHECKED_GL_CALL(glGenTextures, 1, &default_white_texture_id);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, default_white_texture_id);
        unsigned char white_pixel[] = {255, 255, 255, 255};
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    std::unordered_map<std::string_view, uint32_t> counts;
    std::string uniform_name;
    uniform_name.reserve(32);

    int texture_unit_index = 0;

    for (int i = 0; i < m_textures.size(); i++) {
        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0 + texture_unit_index);
        const auto &texture_type = Texture::uniform_name_convention(m_textures[i]->type());
        uniform_name.append(texture_type);
        const auto count = (counts[texture_type] += 1);
        uniform_name.append(std::to_string(count));

        shader->set_int(uniform_name, texture_unit_index);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_textures[i]->id());
        uniform_name.clear();
        texture_unit_index++;
    }

    // Bind fallback white texture if the mesh lacks diffuse or specular maps
    if (counts.find("texture_diffuse") == counts.end()) {
        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0 + texture_unit_index);
        shader->set_int("texture_diffuse1", texture_unit_index);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, default_white_texture_id);
        texture_unit_index++;
    }
    if (counts.find("texture_specular") == counts.end()) {
        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0 + texture_unit_index);
        shader->set_int("texture_specular1", texture_unit_index);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, default_white_texture_id);
        texture_unit_index++;
    }

    CHECKED_GL_CALL(glBindVertexArray, m_vao);
    CHECKED_GL_CALL(glDrawElements, GL_TRIANGLES, m_num_indices, GL_UNSIGNED_INT, (void *) 0);
    CHECKED_GL_CALL(glBindVertexArray, 0);
}

void Mesh::destroy() {
    CHECKED_GL_CALL(glDeleteVertexArrays, 1, &m_vao);
}

}// namespace engine::resources
