#include <print>
#include <fstream>
#include <map>

#include "shaders.h"

bool Shader::validate_shader()
{
    auto validate_shader = [](GLuint handle, std::string descriptor)
    {
        GLint status = 0, log_length = 0;
        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);

        if ((GLboolean)status == GL_FALSE)
            std::print("failed to compile '%s'\n", descriptor.c_str());

        if (log_length > 1)
        {
            ImVector<char> buf{};

            buf.resize((int)(log_length + 1));
            glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());

            std::print("%s\n", buf.begin());
        }

        return (GLboolean)status == GL_TRUE;
    };

    return validate_shader(m_vert_shader, m_vert_path.filename().string()) && validate_shader(m_frag_shader, m_frag_path.filename().string());
}

bool Shader::validate_program()
{
    GLint status = 0, log_length = 0;
    glGetProgramiv(m_shader_program, GL_LINK_STATUS, &status);
    glGetProgramiv(m_shader_program, GL_INFO_LOG_LENGTH, &log_length);

    if ((GLboolean)status == GL_FALSE)
        std::print("failed to link program for '%s'\n", m_name.c_str());

    if (log_length > 1)
    {
        ImVector<char> buf{};

        buf.resize((int)(log_length + 1));
        glGetProgramInfoLog(m_shader_program, log_length, NULL, (GLchar*)buf.begin());

        std::print("%s\n", buf.begin());
    }

    return (GLboolean)status == GL_TRUE;
}

bool Shader::validate()
{
    return validate_shader() && validate_program();
}

Shader Shaders::get_shader(u32 name)
{
    Shader ret{};
    for (auto& shader : m_shaders)
    {
        if (name == shader.m_hash)
            ret = shader;
    }

    return ret;
}

void Shaders::activate_shader(const Shader& shader)
{
    glUseProgram(shader.m_shader_program);
}

Shader Shaders::get_shader(std::string_view name)
{
    return get_shader(Fnv_Hash::hash(name));
}

void Shaders::activate_shader(std::string_view name)
{
    activate_shader(get_shader(name));
}

bool Shaders::load_shaders()
{
    auto load_file = [](const std::filesystem::path& p)
    {
        std::ifstream file(p, std::ios::in);

        const size_t filesize = std::filesystem::file_size(p);

        std::string ret(filesize, '\0');
        file.read(ret.data(), filesize);

        return ret;
    };

    const auto parent_folder = std::filesystem::path("./assets/shaders");

    if (parent_folder.empty())
        return false;

    // shader_name(e.g. 'fog') : vert_path, frag_path
    std::map<std::string, std::pair<std::filesystem::path, std::filesystem::path>> shader_paths;
    for (auto& it : std::filesystem::directory_iterator(parent_folder))
    {
        auto& path = it.path();

        if (path.extension() != ".glsl")
            continue;

        const std::string filename = path.filename().string();

        // e.g. fog.vert.glsl -> fog
        const std::string shader_name = filename.substr(0, filename.find_first_of('.'));

        if (filename.find("vert") != std::string::npos)
            shader_paths[shader_name].first = path;

        else
            shader_paths[shader_name].second = path;
    }

    if (shader_paths.empty())
        return false;

    // load all the shaders, compile them, put them into m_shaders
    for (auto& [key, value] : shader_paths)
    {
        const auto& [vert_path, frag_path] = value;

        Shader shader{};
        shader.m_name      = key;
        shader.m_hash      = Fnv_Hash::hash(shader.m_name);
        shader.m_vert_path = vert_path;
        shader.m_frag_path = frag_path;

        if (vert_path.empty() || frag_path.empty())
            return false;

        const std::string vert_src = load_file(vert_path);
        const std::string frag_src = load_file(frag_path);

        const char* vert_src_ptr = vert_src.data();
        const char* frag_src_ptr = frag_src.data();

        shader.m_vert_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(shader.m_vert_shader, 1, &vert_src_ptr, nullptr);
        glCompileShader(shader.m_vert_shader);

        shader.m_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader.m_frag_shader, 1, &frag_src_ptr, nullptr);
        glCompileShader(shader.m_frag_shader);

        shader.m_shader_program = glCreateProgram();
        glAttachShader(shader.m_shader_program, shader.m_vert_shader);
        glAttachShader(shader.m_shader_program, shader.m_frag_shader);
        glLinkProgram(shader.m_shader_program);

        if (!shader.validate())
            return false;

        m_shaders.push_back(shader);
    }

    return true;
}
