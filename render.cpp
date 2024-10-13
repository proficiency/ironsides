#include "render.h"
#include "map.h"
#include "ship.h"
#include "battle.h"

std::atomic<ImRect>       g_titlebar_bounds;
SDL_HitTestResult SDLCALL hit_test_callback(SDL_Window* win, const SDL_Point* area, void* userdata)
{
    // todo: figure out a good solution for handling resize for the nonclient area
    auto titlebar_bounds = g_titlebar_bounds.load();

    if (titlebar_bounds.Contains(ImVec2(area->x, area->y)))
        return SDL_HITTEST_DRAGGABLE;

    return SDL_HITTEST_NORMAL;
}

// https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_opengl3/main.cpp
bool Render::init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << SDL_GetError() << std::endl;
        return false;
    }

    const char* glsl_version = "#version 460";
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
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS);
    m_window                     = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", m_screen_size.x, m_screen_size.y, window_flags);

    SDL_SetWindowHitTest(m_window, hit_test_callback, nullptr);

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

    gladLoadGL();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags                       |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigWindowsMoveFromTitleBarOnly  = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(m_window, m_gl_ctx);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // ImGui uses the first loaded font as the default
    io.Fonts->AddFontFromFileTTF("./assets/fonts/Inconsolata-Bold.ttf", 16);

    if (!load_images())
        return false;

    if (!m_shaders.load_shaders())
        return false;

    m_battle = std::make_unique<Battle>();
    m_battle->init();

    return true;
}

bool Render::load_images()
{
    if (!m_images.load("./assets/components/hull1.png", "Component_Hull"))
    {
        std::cout << "failed to load hull texture" << std::endl;
        return false;
    }

    if (!m_images.load("./assets/components/turret.png", "Component_Turret"))
    {
        std::cout << "failed to load turret texture" << std::endl;
        return false;
    }

    if (!m_images.load("./assets/vignette.png", "Vignette"))
    {
        std::cout << "failed to load vignette texture" << std::endl;
        return false;
    }

    if (!m_images.load("./assets/noise_transparent.png", "Noise_Transparent"))
    {
        std::cout << "failed to load transparent noise texture" << std::endl;
        return false;
    }

    if (!m_images.load("./assets/noise_opaque.png", "Noise_Opaque"))
    {
        std::cout << "failed to load noise_opaque texture" << std::endl;
        return false;
    }

    return true;
}

void Render::render()
{
    ImVec4   clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io          = ImGui::GetIO();

    while (m_quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                {
                    m_quit = false;
                    break;
                }

                default:
                    break;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        update();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::GetForegroundDrawList()->PushClipRect(m_min, m_max);
        for (auto& [name, layer] : m_layers)
        {
            if (layer.m_dl->CmdBuffer.size() == 1)
                continue;

            // render each layer to it's own framebuffer/texture & add the resulting texture to the final drawlist
            layer.on_render();
            ImGui::GetForegroundDrawList()->AddImage((ImTextureID)layer.m_texture, ImVec2(0.0f, 0.0f), io.DisplaySize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        }
        ImGui::GetForegroundDrawList()->PopClipRect();

        // bind the default framebuffer and render to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

std::shared_ptr<ImDrawList> Render::get_dl(std::string_view name)
{
    auto& ret = m_layers[name.data()].m_dl;
    return ret;
}

void Render::update()
{
    ImGui::SetNextWindowSize(m_screen_size);
    ImGui::SetNextWindowPos({});

    const std::string name = std::format("Ironsides({})", m_fps_counter.get_fps());
    if (!ImGui::Begin(name.c_str(), &m_quit, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings))
        return;

    // if (&g_titlebar_bounds.load()[0] == 0x0)
    {
        ImRect bounds = {};
        bounds.Min    = ImGui::GetWindowPos();
        bounds.Max    = bounds.Min + ImGui::GetItemRectSize();

        g_titlebar_bounds.store(bounds);
    }

    // upper-left corner of content region, in window-space.
    m_min = ImGui::GetWindowContentRegionMin();

    // bottom-right corner of the content region, in window-space.
    m_max = ImGui::GetWindowContentRegionMax();

    // convert min and max to screen-space
    m_min += ImGui::GetWindowPos();
    m_max += ImGui::GetWindowPos();

    m_dl = ImGui::GetForegroundDrawList();

    // init drawlists and their respective contexts, prepare them for a new frame
    {
        if (m_layers.empty())
        {
            m_layers["bg"]    = Layer();
            m_layers["game"]  = Layer();
            m_layers["bloom"] = Layer();
            m_layers["ui"]    = Layer();
            m_layers["final"] = Layer();
        }

        for (auto& [name, layer] : m_layers)
            layer.on_new_frame();
    }

    m_dl->PushClipRect(m_min, m_max);
    {
        m_battle->update();
        m_battle->draw();
    }
    m_dl->PopClipRect();

    ImGui::End();
    ImGui::ShowStyleEditor();
}

Render::Layer::Layer() : m_fbo(), m_rbo(), m_texture()
{
    const ImVec2 framebuffer_size = ImGui::GetIO().DisplaySize;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_size.x, framebuffer_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebuffer_size.x, framebuffer_size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

    auto res = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (res != GL_FRAMEBUFFER_COMPLETE)
        std::print("framebuffer incomplete: 0x%lx\n", res);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    m_dl       = std::make_shared<ImDrawList>(ImGui::GetDrawListSharedData());
    m_drawdata = std::make_unique<ImDrawData>();

    m_dl->AddDrawCmd();
}

void Render::Layer::on_new_frame() 
{
    m_drawdata->Clear();
    m_drawdata->Valid            = true;
    m_drawdata->DisplayPos       = ImGui::GetMainViewport()->Pos;
    m_drawdata->DisplaySize      = ImGui::GetMainViewport()->Size;
    m_drawdata->FramebufferScale = ImVec2(1.0f, 1.0f);

    m_dl->_ResetForNewFrame();
    m_dl->PushClipRectFullScreen();
    m_dl->PushTextureID(ImGui::GetIO().Fonts->TexID);
}

void Render::Layer::on_render() 
{
    m_drawdata->AddDrawList(m_dl.get());

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(m_drawdata.get());
}

std::unique_ptr<ImDrawData> Render::Layer::make_drawdata()
{
    return {};
}
