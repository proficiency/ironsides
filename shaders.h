#pragma once
#include <vector>
#include <filesystem>
#include <imgui.h>
#include <glad/glad.h>

#include "hash.h"

class Shader
{
    friend class Shaders;

private:
    bool validate_shader();
    bool validate_program();

protected:
    std::filesystem::path m_vert_path;
    std::filesystem::path m_frag_path;
    u32                   m_hash;

public:
    GLuint      m_vert_shader;
    GLuint      m_frag_shader;
    GLuint      m_shader_program;
    std::string m_name;

    bool validate();
};

class Shaders
{
    std::vector<Shader> m_shaders;

    Shader get_shader(u32 name);

public:
    Shaders() = default;

    bool   load_shaders();
    Shader get_shader(std::string_view name);
    void   activate_shader(const Shader& shader);
    void   activate_shader(std::string_view name);
};
