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

#include <gl\GL.h>
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

class Image
{
public:
    GLuint m_texture;
    Hash   m_hash;
    ImVec2 m_size;
};

class Images
{
    std::vector<Image> m_images;

public:
    bool load(const std::filesystem::path& path, std::string_view name)
    {
        Image img{};

        SDL_Surface* surface = IMG_Load(path.string().c_str());
        if (!surface)
        {
            return false;
        }

        GLenum           mode   = GL_RGBA;
        SDL_PixelFormat* target = SDL_CreatePixelFormat(SDL_PIXELFORMAT_RGBA32);
        surface                 = SDL_ConvertSurface(surface, target);

        glGenTextures(1, &img.m_texture);
        glBindTexture(GL_TEXTURE_2D, img.m_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

        glBindTexture(GL_TEXTURE_2D, 0);

        img.m_hash = Fnv_Hash::hash(name);
        img.m_size = {(float)surface->w, (float)surface->h};

        m_images.push_back(img);

        return true;
    }

    Image get(std::string_view name)
    {
        return get(Fnv_Hash::hash(name));
    }

    Image get(Hash hash)
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
class Render : public std::enable_shared_from_this<Render>
{
    SDL_Window*             m_window;
    SDL_GLContext           m_gl_ctx;
    std::shared_ptr<Battle> m_battle;
    bool                    m_quit;

public:
    Render() : m_quit(true) {}

    std::array<ImFont*, FONT_MAX> m_fonts;
    ImVec2                        m_min;
    ImVec2                        m_max;
    ImDrawList*                   m_dl;
    Images                        m_images;

    std::shared_ptr<Render> get_shared_from_this()
    {
        return shared_from_this();
    }

    bool init();
    void render();

    void update();
};