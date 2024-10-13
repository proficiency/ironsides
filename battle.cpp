#include "render.h"
#include "map.h"
#include "ship.h"
#include "battle.h"
#include "formation.h"

Battle::Battle()
{
    // todo: big json file of ship names, classes, etc
    m_ships.push_back(std::make_shared<Ship>("Iowa", NAT_USA));
    m_ships.push_back(std::make_shared<Ship>("Alaska", NAT_USA));
    m_ships.push_back(std::make_shared<Ship>("Nevada", NAT_USA));
    m_ships.push_back(std::make_shared<Ship>("California", NAT_USA));

    for (u32 i = 0; i < m_ships.size(); ++i)
    {
        auto& ship = m_ships[i];

        if (ship)
            ship->m_pos = glm::vec2(150.0f + (i * 200.0f), 700.0f);
    }
}

void Battle::init()
{
    m_formation_controller = std::make_shared<Formation_Controller>(get_shared_from_this());
    g_map                  = std::make_shared<Map>();
}

void Battle::update()
{
    // todo: maybe add queued actions(shift click)?
    m_formation_controller->update();
    g_map->update();

    spawn_enemies();

    for (u32 i = 0; i < m_ships.size(); ++i)
    {
        auto& ship = m_ships[i];
        if (ship == nullptr)
            continue;

        ship->update();

        if (ship->is_alive())
            ship->select_target(m_enemy_ships);

        else if (ship->handle_death())
        {
            ship = nullptr;
            m_ships.erase(m_ships.begin() + i);
        }
    }

    control_ai();
}

glm::vec2 center{};
void      Battle::draw()
{
    // background
    g_render->get_dl("bg")->AddRectFilled(g_render->m_min, g_render->m_max, IM_COL32(27, 54, 84, 255));
    g_render->get_dl("bg")->AddImage((ImTextureID)g_render->m_images.get("Noise_Transparent").m_texture, g_render->m_min, g_render->m_max);

    for (auto& ship : m_ships)
        ship->draw(g_render->m_dl, g_render->m_min + g_map->m_drag_offset, g_render->m_max + g_map->m_drag_offset, 0);

    for (auto& ship : m_enemy_ships)
        ship->draw(g_render->m_dl, g_render->m_min + g_map->m_drag_offset, g_render->m_max + g_map->m_drag_offset, 0);

    /* g_render->m_dl->AddCallback(
        [](const ImDrawList* dl, const ImDrawCmd* cmd)
        {
            ImDrawData* draw_data = ImGui::GetDrawData();
            float       L         = draw_data->DisplayPos.x;
            float       R         = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
            float       T         = draw_data->DisplayPos.y;
            float       B         = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

            ImGuiWindow* window = (ImGuiWindow*)cmd->UserCallbackData;
            const auto   ortho  = glm::ortho(L, R, B, T);

            auto shader         = g_render->m_shaders.get_shader("fog");
            auto shader_program = shader.m_shader_program;

            g_render->m_shaders.activate_shader(shader);

            ImVec2 window_center = (window->Pos + (window->Size * 0.5f)) / ImGui::GetIO().DisplaySize;
            center               = center / ImGui::GetIO().DisplaySize;

            glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, &ortho[0][0]);
            glUniform1f(glGetUniformLocation(shader_program, "time"), ImGui::GetTime() / 1.5f);
            glUniform1f(glGetUniformLocation(shader_program, "mask_radius"), 1.5f);
            glUniform2f(glGetUniformLocation(shader_program, "mask_center"), center.x, center.y);
            glUniform2f(glGetUniformLocation(shader_program, "resolution"), ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
        },
        (void*)ImGui::GetCurrentWindow()
    );

    g_render->m_dl->AddImage((ImTextureID)g_render->m_images.get("Vignette").m_texture, g_render->m_min, g_render->m_max);
    g_render->m_dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr);*/

    // draw our vignette over everything
    g_render->m_dl->AddImage((ImTextureID)g_render->m_images.get("Vignette").m_texture, g_render->m_min, g_render->m_max);

    // draw list of formations
    m_formation_controller->draw();

    // draw minimap/radar
    g_map->draw_minimap(m_ships, m_enemy_ships);

    if (!m_formation_controller->m_formations.empty())
    {
        center = m_formation_controller->m_formations.back()->get_center() + g_render->m_min + g_map->m_drag_offset;
        // this is the area i want to draw a mask about
        g_render->m_dl->AddCircleFilled(
            m_formation_controller->m_formations.back()->get_center() + g_render->m_min + g_map->m_drag_offset, 15.0f, IM_COL32_WHITE
        );
    }
}

// todo: determine when we can spawn enemies, spawn them somewhere offscreen, give them an intercept course, etc
void Battle::spawn_enemies()
{
    if (ImGui::GetTime() - m_last_spawn_time < g_rng.get_random(150.0f, 250.0f) && !m_enemy_ships.empty())
        return;

    u32 start_pos = m_enemy_ships.size();
    m_enemy_ships.push_back(std::make_shared<Ship>("Fuso", NAT_JAP));
    m_enemy_ships.push_back(std::make_shared<Ship>("Ise", NAT_JAP));
    m_enemy_ships.push_back(std::make_shared<Ship>("Nagato", NAT_JAP));
    m_enemy_ships.push_back(std::make_shared<Ship>("Mutsu", NAT_JAP));
    const auto new_ships = std::vector(m_enemy_ships.begin() + start_pos, m_enemy_ships.end());

    const glm::vec2 spawn_pos = glm::vec2(g_rng.get_random(0.0f, g_map->m_map_size.x), g_rng.get_random(0.0f, g_map->m_map_size.y));
    for (; start_pos < m_enemy_ships.size(); ++start_pos)
    {
        auto& ship = m_enemy_ships[start_pos];

        if (ship)
            ship->m_pos = glm::vec2(spawn_pos.x + (start_pos * 200.0f), spawn_pos.y);
    }

    m_enemy_formations.push_back(std::make_shared<Formation>(new_ships));
    m_last_spawn_time = ImGui::GetTime();
}

void Battle::control_ai()
{
    for (u32 i = 0; i < m_enemy_formations.size(); ++i)
    {
        auto& formation = m_enemy_formations[i];

        if (formation->m_ships.empty())
        {
            m_enemy_formations.erase(m_enemy_formations.begin() + i);
            continue;
        }

        auto leader = formation->get_leader();
        if (glm::near_zero(leader->m_dst) || glm::distance(leader->m_dst, leader->m_pos) < 1.0f)
        {
            glm::vec2 window_min = ImGui::GetWindowContentRegionMin() - ImGui::GetWindowPos();
            glm::vec2 window_max = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowPos();

            leader->m_dst.x = g_rng.get_random(window_min.x, window_max.x);
            leader->m_dst.y = g_rng.get_random(window_min.y, window_max.y);
        }

        formation->update();
    }

    for (u32 i = 0; i < m_enemy_ships.size(); ++i)
    {
        auto& ship = m_enemy_ships[i];

        ship->update();
        ship->select_target(m_ships);

        if (!ship->is_alive() && ship->handle_death())
        {
            ship = nullptr;
            m_enemy_ships.erase(m_enemy_ships.begin() + i);
        }
    }
}
