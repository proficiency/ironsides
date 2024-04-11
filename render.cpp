#include "render.h"
#include "ship.h"
#include "battle.h"

// https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_opengl3/main.cpp
bool Render::init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << SDL_GetError() << std::endl;
        return false;
    }

    const char* glsl_version = "#version 410";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Enable native IME.
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    m_window                     = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", 1920, 1040, window_flags);

    if (m_window == nullptr)
    {
        std::cout << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    m_gl_ctx = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_gl_ctx);
    SDL_GL_SetSwapInterval(0); // disable vsync
    SDL_ShowWindow(m_window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags                       |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags                       |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigWindowsMoveFromTitleBarOnly  = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(m_window, m_gl_ctx);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // ImGui uses the first loaded font as the default
    m_fonts[FONT_MEDIUM]   = io.Fonts->AddFontFromFileTTF("./assets/fonts/Inconsolata-Bold.ttf", 16);
    m_fonts[FONT_SMALLEST] = io.Fonts->AddFontFromFileTTF("./assets/fonts/Inconsolata-Bold.ttf", 8);
    m_fonts[FONT_SMALL]    = io.Fonts->AddFontFromFileTTF("./assets/fonts/Inconsolata-Bold.ttf", 12);
    m_fonts[FONT_BIG]      = io.Fonts->AddFontFromFileTTF("./assets/fonts/Inconsolata-Bold.ttf", 20);
    m_fonts[FONT_BIGGEST]  = io.Fonts->AddFontFromFileTTF("./assets/fonts/Inconsolata-Bold.ttf", 24);

    m_battle = std::make_unique<Battle>();
    m_battle->init();

    render();
    return true;
}

void Render::render()
{
    ImVec4   clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io          = ImGui::GetIO();

    while (true)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            ImGui_ImplSDL3_ProcessEvent(&event);        

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        update();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(m_gl_ctx);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Render::update() 
{
    if (!ImGui::Begin("Ironsides", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse))
        return;

    m_battle->update();

    ImGui::End();
}
