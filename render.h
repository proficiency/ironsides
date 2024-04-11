#pragma once
#include <Windows.h>

#include <array>
#include <iostream>

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
class Render
{
    SDL_Window*             m_window;
    SDL_GLContext           m_gl_ctx;
    std::unique_ptr<Battle> m_battle;

protected:
    std::array<ImFont*, FONT_MAX> m_fonts;

public:
    bool init();
    void render();

    void update();
};
