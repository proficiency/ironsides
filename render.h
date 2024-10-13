#pragma once

// tell sdl we're defining our own entry point
#define SDL_MAIN_HANDLED

// let ImGui know about our imconfig header
#define IMGUI_USER_CONFIG "ironsides_imconfig.h"

// enables various math constants in cmath
// #define _USE_MATH_DEFINES

// omit min and max macros from windows headers
#define NOMINMAX

#define GL_CLAMP_TO_EDGE 0x812F

#include <Windows.h>

#include <array>
#include <iostream>
#include <filesystem>
#include <string>
#include <atomic>
#include <format>
#include <numeric>
#include <map>
#include <print>

#include "shaders.h"
#include <glad/glad.h>
#include <glm\glm.hpp>
#include <glm\ext.hpp>
#include <glm\gtx\vector_angle.hpp>

#include <SDL3\SDL.h>
#include <SDL3\SDL_opengl.h>
#include <SDL3_image\SDL_image.h>

#include <imgui_internal.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include "hash.h"
#include "random.h"

class Image
{
public:
    GLuint       m_texture;
    u32          m_hash;
    ImVec2       m_size;
    SDL_Surface* m_surface;
};

class Images
{
    std::vector<Image> m_images;

public:
    bool load(const std::filesystem::path& path, std::string_view name, GLint wrap_param = GL_CLAMP_TO_EDGE)
    {
        Image img{};

        SDL_Surface* surface = IMG_Load(path.string().c_str());
        if (surface == nullptr)
            return false;

        SDL_PixelFormat* target = SDL_CreatePixelFormat(SDL_PIXELFORMAT_RGBA32);
        surface                 = SDL_ConvertSurface(surface, target);

        glGenTextures(1, &img.m_texture);
        glBindTexture(GL_TEXTURE_2D, img.m_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_param);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_param);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

        glBindTexture(GL_TEXTURE_2D, 0);

        img.m_hash    = Fnv_Hash::hash(name);
        img.m_size    = {(float)surface->w, (float)surface->h};
        img.m_surface = surface;

        m_images.push_back(img);

        return true;
    }

    Image get(std::string_view name)
    {
        return get(Fnv_Hash::hash(name));
    }

    Image get(u32 hash)
    {
        for (auto& img : m_images)
        {
            if (img.m_hash == hash)
                return img;
        }

        return {};
    }
};

enum FONT_SIZES
{
    FONT_SMALLEST = 0,
    FONT_SMALL,
    FONT_MEDIUM,
    FONT_BIG,
    FONT_BIGGEST,

    FONT_MAX,
};

class Battle;

class FPS_Counter
{
    float              m_last_update_time;
    std::vector<float> m_fps_history;
    std::string        m_display;

public:
    FPS_Counter() : m_last_update_time(), m_fps_history(), m_display() {}

    std::string get_fps()
    {
        m_fps_history.push_back(ImGui::GetIO().Framerate);

        if (m_display.empty() || ImGui::GetTime() - m_last_update_time > 0.5f)
        {
            const float average_fps = std::accumulate(m_fps_history.begin(), m_fps_history.end(), 0.0f) / m_fps_history.size();

            m_display          = std::format("FPS: {:.1f} Average: {:.1f}", m_fps_history.back(), average_fps);
            m_last_update_time = ImGui::GetTime();
        }

        return m_display;
    }
};

class Render
{
    SDL_Window*             m_window;
    SDL_GLContext           m_gl_ctx;
    std::shared_ptr<Battle> m_battle;
    bool                    m_quit;
    FPS_Counter             m_fps_counter;

    bool load_images();
    void update();

public:
    // todo: maybe this should hold an std::optional<Shader> object, that would look nice in the rendering code
    class Layer
    {
        std::unique_ptr<ImDrawData> make_drawdata();

    public:
        GLuint                      m_fbo;
        GLuint                      m_rbo;
        GLuint                      m_texture;
        std::shared_ptr<ImDrawList> m_dl;
        std::unique_ptr<ImDrawData> m_drawdata;

        Layer();
        void on_new_frame();
        void on_render();
    };

    Render() : m_quit(true), m_screen_size(1730.0f, 825.0f) {}

    ImVec2                                             m_min;
    ImVec2                                             m_max;
    ImDrawList*                                        m_dl;
    std::map<std::string, Layer>                       m_layers;
    Images                                             m_images;
    Shaders                                            m_shaders;
    ImVec2                                             m_screen_size;

    bool init();
    void render();
    std::shared_ptr<ImDrawList> get_dl(std::string_view name);
};

inline std::shared_ptr<Render> g_render;
