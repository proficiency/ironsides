#include "render.h"
#include "ship.h"
#include "battle.h"

void Battle::init()
{
    if (!m_render_parent->m_images.load("./assets/components/hull1.png", "Component_Hull1"))
    {
        std::cout << "failed to load hull image" << std::endl;
        return;
    }
 
    const auto img = m_render_parent->m_images.get("Component_Hull1");

    // todo: big json file of ship names, classes, etc
    m_ships.push_back(std::make_shared<Ship>("Iowa", img));
    m_ships.push_back(std::make_shared<Ship>("Alaska", img));
    m_ships.push_back(std::make_shared<Ship>("Nevada", img));
    m_ships.push_back(std::make_shared<Ship>("California", img));
 
   
    m_enemy_ships.push_back(std::make_shared<Ship>("Fuso", img));
    m_enemy_ships.push_back(std::make_shared<Ship>("Ise", img));
    m_enemy_ships.push_back(std::make_shared<Ship>("Nagato", img));
    m_enemy_ships.push_back(std::make_shared<Ship>("Mutsu", img));
}

void Battle::update()
{
    // todo: maybe add queued actions(shift click)?
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsMouseHoveringRect(m_render_parent->m_min, m_render_parent->m_max))
    {
        //m_selected_ships.back()->update_destination(ImGui::GetMousePos() - m_render_parent->m_min);
        m_ships.back()->update_destination(ImGui::GetMousePos() - m_render_parent->m_min);
    }

    for (auto& ship : m_ships)
    {
        ship->sail();
        ship->update_wake();
    }

    control_ai();
}

void Battle::draw()
{
    ImGui::PushFont(m_render_parent->m_fonts[m_zoom_level]);

    for (auto& ship : m_ships)
        ship->draw(m_render_parent->m_dl, m_render_parent->m_min, m_render_parent->m_max, 0);

    for (auto& ship : m_enemy_ships)
        ship->draw(m_render_parent->m_dl, m_render_parent->m_min, m_render_parent->m_max, 0);

    ImGui::PopFont();
}

void Battle::control_formation() 
{
    if (m_selected_ships.empty())
        return;

    // determine which ship is closest to the destination point, that becomes the leader? alternatively, determine which ship is nearest the front of the current extremely rough formation
    // to minimize turning. then offset m_dst by -index_in_formation * spacing
    // alternatively, just have each ship follow the next ship in the formation? need to test around with it
}

void Battle::control_ai()
{
    for (auto& ship : m_enemy_ships)
    {
        if (glm::length(ship->m_dst) == 0.0f || glm::distance(ship->m_dst, ship->m_pos) < 1.0f)
        {
            glm::vec2 window_min = ImGui::GetWindowContentRegionMin() - ImGui::GetWindowPos();
            glm::vec2 window_max = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowPos();
            
            ship->m_dst.x = m_rng.get_random(window_min.x, window_max.x);
            ship->m_dst.y = m_rng.get_random(window_min.y, window_max.y);
        }

        ship->sail();
        ship->update_wake();
    }
}
