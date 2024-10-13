#include "render.h"
#include "map.h"
#include "battle.h"
#include "ship.h"
#include "formation.h"

// todo: we can move away from using a pointer to battle 
Formation_Controller::Formation_Controller(const std::shared_ptr<Battle>& battle_parent) : m_battle_parent(battle_parent) {}

void Formation_Controller::update()
{
    handle_ship_selection();

    if (m_selected_ships.size() > 1u)
    {
        const auto new_formation = std::make_shared<Formation>(m_selected_ships);
        m_active_formation       = new_formation;

        if (find_existing_formation(new_formation) == nullptr)
        {
            for (auto& follower : new_formation->m_ships)
            {
                auto [existing_formation, index] = find_in_existing_formation(follower);

                if (existing_formation == nullptr)
                    continue;

                existing_formation->m_ships.erase(existing_formation->m_ships.begin() + index);
            }

            m_formations.push_back(new_formation);
        }
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsMouseHoveringRect(g_render->m_min, g_render->m_max))
    {
        const glm::vec2 dst = (ImGui::GetMousePos() - g_render->m_min) - g_map->m_drag_offset;

        if (m_selected_ships.size() == 1u)
            m_selected_ships.back()->m_dst = dst;

        else if (m_active_formation != nullptr)
            m_active_formation->get_leader()->m_dst = dst;
    }

    if (m_formations.empty() || m_active_formation == nullptr)
        return;

    for (u32 i = 0; i < m_formations.size(); ++i)
    {
        auto& formation = m_formations[i];
        if (formation == nullptr)
            continue;

        // formation is either empty or only has one ship in it, no longer valid
        if (formation->m_ships.size() <= 1u)
        {
            m_formations.erase(m_formations.begin() + i);
            continue;
        }

        formation->update();
    }

    m_selected_ships.clear();
}

void Formation_Controller::draw()
{
    // render the selection area
    if (m_dragging)
    {
        g_render->get_dl("ui")->AddRectFilled(m_drag_src, m_drag_dst, IM_COL32(0, 200, 0, 55));
        g_render->get_dl("ui")->AddRect(m_drag_src, m_drag_dst, IM_COL32(0, 255, 0, 175));
        g_render->get_dl("ui")->AddRect(m_drag_src - ImVec2(1, 1), m_drag_dst + ImVec2(1, 1), IM_COL32(0, 255, 0, 175));
    }

    // draw the list of formations
    // todo: make this a clickable ui element that lets you select the formation or right click to delete it(or click an X)
    if (!m_formations.empty())
    {
        ImU32  color  = IM_COL32_WHITE;
        ImVec2 cursor = g_render->m_min + ImVec2(5.0f, 0.0f);
        for (u32 i = 0; i < m_formations.size(); ++i)
        {
            const auto& formation = m_formations[i];

            g_render->get_dl("ui")->AddText(cursor, IM_COL32_WHITE, (std::string("formation #") + std::to_string(i)).c_str());
            cursor.y += ImGui::GetFontSize();

            for (auto& follower : formation->m_ships | std::views::reverse)
            {
                std::string name_str = follower->m_name;

                if (follower == formation->get_leader())
                    name_str += "(leader)";

                g_render->get_dl("ui")->AddText(cursor + ImVec2(10.0f, 0.0f), IM_COL32_WHITE, name_str.c_str());
                cursor.y += ImGui::GetFontSize();
            }

            cursor.y += ImGui::GetFontSize();
        }
    }
}

// todo: pass ships as an arg
void Formation_Controller::handle_ship_selection()
{
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !g_map->dragging_minimap())
    {
        const ImVec2 mouse_pos = ImGui::GetMousePos();

        m_shift_dragging = ImGui::IsKeyDown(ImGuiKey_LeftShift);

        // drag just started, store drag src
        if (!m_dragging)
        {
            m_drag_src = mouse_pos;
            m_dragging = true;
        }

        // update drag dst
        m_drag_dst = mouse_pos;
    }

    // we just stopped dragging
    else if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_dragging)
    {
        const ImVec2 selection_min = ImVec2(std::min(m_drag_src.x, m_drag_dst.x), std::min(m_drag_src.y, m_drag_dst.y));
        const ImVec2 selection_max = ImVec2(std::max(m_drag_src.x, m_drag_dst.x), std::max(m_drag_src.y, m_drag_dst.y));
        m_selection_rect           = ImRect(selection_min, selection_max);

        m_selected_ships.clear();

        for (auto& ship : m_battle_parent->m_ships)
        {
            if (m_selection_rect.Contains(ship->m_pos + g_map->m_drag_offset + g_render->m_min))
            {
                if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && m_active_formation != nullptr)
                {
                    m_active_formation->m_ships.push_back(ship);
                    m_shift_dragging = false;
                }

                m_selected_ships.push_back(ship);
            }
        }

        m_dragging = false;
        m_drag_src = {};
        m_drag_dst = {};
    }
}

std::pair<std::shared_ptr<Formation>, u32> Formation_Controller::find_in_existing_formation(const std::shared_ptr<Ship>& ship)
{
    for (auto& formation : m_formations)
    {
        for (u32 i = 0; i < formation->m_ships.size(); ++i)
        {
            if (formation->m_ships[i] == ship)
                return std::make_pair(formation, i);
        }
    }

    return {nullptr, 0};
}

std::shared_ptr<Formation> Formation_Controller::find_existing_formation(const std::shared_ptr<Formation>& f)
{
    for (auto& formation : m_formations)
    {
        if (*formation == *f)
            return formation;
    }

    return nullptr;
}

void Formation_Controller::remove_from_existing_formation(const std::shared_ptr<Ship>& ship) 
{
    auto existing_formation = nullptr;//find_existing_formation(ship);

    // the ship isn't in a formation
    if (existing_formation == nullptr)
        return;
}

// todo:
// store path variable in Ship class, in Formation find the point along the path nearest to our current ship(take into account rotation), use that as
// the starting position to 'join up' like find the point along the path that's closest to m_pos + forward * 150.0f
Formation::Formation(const std::vector<std::shared_ptr<Ship>>& selected_ships) : m_ships(selected_ships), m_highest_distance()
{
    std::shared_ptr<Ship> leader = nullptr;
    for (const auto& ship : selected_ships)
    {
        glm::vec2 reference = selected_ships.front()->m_pos;

        float dist = glm::dot(ship->m_pos - reference, ship->get_forward());

        if (dist > m_highest_distance)
        {
            m_highest_distance = dist;
            leader             = ship;
        }
    }

    if (leader == nullptr)
    {
        // something went terribly wrong
        std::print("formation failed to find a leader, this should never happen.\n");
        return;
    }

    const glm::vec2 pos = leader->m_pos;

    auto sort_pred = [pos](const std::shared_ptr<Ship>& lhs, const std::shared_ptr<Ship>& rhs)
    {
        return glm::distance(lhs->m_pos, pos) < glm::distance(rhs->m_pos, pos);
    };

    std::sort( m_ships.begin(), m_ships.end(), sort_pred);
}

// todo: if selecting a subformation, don't recalculate the leader
void Formation::update()
{
    for (u32 i = 0; i < m_ships.size(); ++i)
    {
        const auto& ship = m_ships[i];

        // remove any dead ships from the formation
        if (!ship->is_alive())
            m_ships.erase(m_ships.begin() + i);   

        ship->m_parent_formation = get_shared_from_this();

        if (m_ships.empty() || ship == get_leader())
            continue;

        // todo: fix this abomination to all that is holy
        const auto& next_ship = m_ships[std::max(i - 1u, 0u)];

        if (!next_ship || next_ship->m_path.empty())
            continue;

        const float dist_to_node = glm::distance(ship->m_pos, next_ship->m_path.front());
        const bool  too_far      = glm::distance(ship->m_pos, next_ship->m_pos) > 250.0f;

        // todo: if we remove the ship from the formation, this won't get reset
        if (too_far)
            ship->m_target_speed = ship->m_max_speed * 1.5f;

        else
            ship->m_target_speed = ship->m_max_speed;

        if (!ship->is_enemy())
            std::print("%s distance to %s: %.1f\n", ship->m_name.c_str(), next_ship->m_name.c_str(), dist_to_node);

        // cull if we're passing the node, or if we're just way too far away we wanna intercept instead of following the path
        while (too_far || dist_to_node < glm::distance(ship->get_bow_pos() + ship->get_forward() * 20.0f, next_ship->m_path.front()))
        {
            if (next_ship->m_path.size() <= 1)
                break;

            next_ship->m_path.pop_front();
        }
        
        ship->m_dst = next_ship->m_path.front();

        /* for (auto& point : next_ship->m_path)
            g_render->get_dl("ui")->PathLineTo(point + g_render->m_min);

        g_render->get_dl("ui")->PathStroke(next_ship->get_color());*/
    }
}

glm::vec2 Formation::get_center()
{
    // this should never happen really
    if (m_ships.empty())
        return {};

    auto reduce_op = [](const glm::vec2& sum, const std::shared_ptr<Ship>& ship)
    {
        return sum + ship->m_pos;
    };

    return std::accumulate(m_ships.begin(), m_ships.end(), glm::vec2(0.0f), reduce_op) / (float)m_ships.size();
}
