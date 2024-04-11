// tell sdl we're defining our own entry point
#define SDL_MAIN_HANDLED

// enable ImVec2 math operators
#define IMGUI_DEFINE_MATH_OPERATORS

// enables various math constants in cmath
// #define _USE_MATH_DEFINES

// omit min and max macros from windows headers
#define NOMINMAX

#define GL_CLAMP_TO_EDGE 0x812F

#include "render.h"
#include "battle.h"

int main()
{
    std::unique_ptr<Render> renderer = std::make_unique<Render>();

    renderer->init();
   
    system("pause");
}
