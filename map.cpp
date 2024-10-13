#include "render.h"
#include "map.h"
#include "ship.h"

Map::Map() :
    m_map_size(),
    m_drag_offset(),
    m_viewport_min(),
    m_viewport_max(),
    m_minimap_pos(),
    m_minimap_size(290.0f, 200.0f),
    m_scan_pos(),
    m_scan_size(glm::vec2(30.0f, m_minimap_size.y))
{}

void Map::draw_water_effect() {}

// todo: this is extremely costly.
// i think a better route would be to sample random points on the map with high noise and get texels of neighboring points 
void Map::generate_clusters()
{
    m_clutter.clear();

    while (m_clutter.size() < 70)
    {
        glm::vec2 center = glm::vec2(g_rng.get_random(0.0f, m_minimap_size.x), g_rng.get_random(0.0f, m_minimap_size.y)) + m_minimap_pos;

        Clutter_Cluster clutter;

        for (u32 y = center.y; y < center.y + 15.0f; ++y)
        {
            for (u32 x = center.x; x < center.x + 15.0f; ++x)
            {
                const glm::vec2 point = glm::vec2(x, y);
                const float     noise = glm::perlin(point / 10.0f);

                if (noise > 0.5f)
                    clutter.m_cluster.push_back(std::make_pair(point, noise));
            }
        }

        if (!clutter.m_cluster.empty())
            m_clutter.push_back(clutter);
    }
}

bool Map::dragging_minimap()
{
    return ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(m_minimap_pos, m_minimap_pos + m_minimap_size);
}

void Map::update()
{
    static glm::vec2 last_mouse_pos = {};
    static bool      dragging       = false;

    if (glm::near_zero(m_map_size))
        m_map_size = ((g_render->m_max) * 4.0f);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
    {
        dragging       = true;
        last_mouse_pos = ImGui::GetMousePos() - g_render->m_min;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
        dragging = false;

    if (dragging)
    {
        const glm::vec2 current_mouse_pos = ImGui::GetMousePos() - g_render->m_min;
        const glm::vec2 mouse_delta       = current_mouse_pos - last_mouse_pos;

        m_drag_offset  += mouse_delta;
        m_viewport_min -= mouse_delta;
        last_mouse_pos  = current_mouse_pos;
    }

    if (dragging_minimap())
    {
        const glm::vec2 relative_pos = (ImGui::GetMousePos() - m_minimap_pos) / m_minimap_size;
        const glm::vec2 world_pos    = relative_pos * m_map_size;

        // not sure if this is right
        m_viewport_min = world_pos - (g_render->m_max - g_render->m_min) * 0.5f;
        m_drag_offset  = g_render->m_min - m_viewport_min;
    }

    m_viewport_max = m_viewport_min + (g_render->m_max - g_render->m_min);
}

void Map::draw_map() {}
void add_radial_gradient(ImDrawList* draw_list, const ImVec2& center, float radius, ImU32 col_in, ImU32 col_out)
{
    if (((col_in | col_out) & IM_COL32_A_MASK) == 0 || radius < 0.5f)
        return;

    // Use arc with automatic segment count
    draw_list->_PathArcToFastEx(center, radius, 0, IM_DRAWLIST_ARCFAST_SAMPLE_MAX, 0);
    const int count = draw_list->_Path.Size - 1;

    unsigned int vtx_base = draw_list->_VtxCurrentIdx;
    draw_list->PrimReserve(count * 3, count + 1);

    // Submit vertices
    const ImVec2 uv = draw_list->_Data->TexUvWhitePixel;
    draw_list->PrimWriteVtx(center, uv, col_in);
    for (int n = 0; n < count; n++)
        draw_list->PrimWriteVtx(draw_list->_Path[n], uv, col_out);

    // Submit a fan of triangles
    for (int n = 0; n < count; n++)
    {
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + n));
        draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((n + 1) % count)));
    }
    draw_list->_Path.Size = 0;
}
// todo: only update positions when the scan bar passes over them, fade out in opacity the longer since last detection
void Map::draw_minimap(const std::vector<std::shared_ptr<Ship>>& friendly_ships, const std::vector<std::shared_ptr<Ship>>& enemy_ships)
{
    const static ImU32 grad_col_l = IM_COL32(0, 57, 12, 255);
    const static ImU32 grad_col_r = IM_COL32(0, 24, 5, 2555);
    const static ImU32 perim_col  = IM_COL32(232, 255, 255, 110);
    const static ImU32 grid_col   = IM_COL32(135, 255, 56, 32);
    const static ImU32 enemy_col  = IM_COL32(255, 163, 199, 255);
    const static ImU32 friend_col = IM_COL32(255, 255, 255, 255);
    const static ImU32 scan_r     = IM_COL32(0, 165, 33, 135);
    const static ImU32 scan_l     = IM_COL32(0, 0, 0, 0);

    m_minimap_pos = {g_render->m_max.x - m_minimap_size.x - 5.0f, g_render->m_min.y + 5.0f};

    g_render->get_dl("ui")->AddRectFilledMultiColor(m_minimap_pos, m_minimap_pos + m_minimap_size, grad_col_l, grad_col_l, grad_col_r, grad_col_r);
    g_render->get_dl("ui")->AddRect(m_minimap_pos, m_minimap_pos + m_minimap_size, perim_col);

    g_render->get_dl("ui")->PushClipRect(m_minimap_pos + glm::vec2(1.0f, 1.0f), (m_minimap_pos + m_minimap_size) - glm::vec2(1.0f, 1.0f));

    // draw grid
    {
        glm::vec2 max       = m_minimap_pos + m_minimap_size;
        glm::vec2 cell_size = {18, 18};

        u32   num_rows    = u32(m_minimap_size.y / cell_size.y);
        u32   num_columns = u32(m_minimap_size.x / cell_size.x);
        float col_width   = m_minimap_size.x / num_columns;
        float row_height  = m_minimap_size.y / num_rows;

        for (u32 i = 1; i < num_rows; ++i)
        {
            const float y = m_minimap_pos.y + i * row_height;
            g_render->get_dl("ui")->AddLine(ImVec2(m_minimap_pos.x + 1.0f, y), ImVec2(max.x - 1.0f, y), grid_col);
        }

        for (u32 i = 1; i < num_columns; ++i)
        {
            const float x = m_minimap_pos.x + i * row_height;
            g_render->get_dl("ui")->AddLine(ImVec2(x, m_minimap_pos.y), ImVec2(x, max.y - 2.0f), grid_col);
        }
    }

    // draw ground clutter effect, todo: revisit this at some point
    {
        for (auto& cluster : m_clutter)
        {
            for (auto& [point, intensity] : cluster.m_cluster)
            {
                if (scan_contains(point))
                    cluster.m_detection_time = ImGui::GetTime();

                int opacity = 0;
                if (cluster.m_detection_time > 1e-6f)
                {
                    float time_since_detection = ImGui::GetTime() - cluster.m_detection_time;
                    float fade_factor          = 1.0f - (time_since_detection / 0.75f);
                    fade_factor                = std::max(0.0f, fade_factor);

                    opacity = static_cast<int>(230 * intensity * fade_factor);
                }

                // g_render->get_dl("ui")->AddRectFilled(point, point + glm::vec2(1.0f, 1.0f), IM_COL32(0, 255 * intensity, 30, opacity));
                add_radial_gradient(g_render->get_dl("ui").get(), point, 4.0f, IM_COL32(0, 255 * intensity, 30, opacity), scan_l);
            }
        }
    }

    for (auto& ship : friendly_ships)
    {
        if (scan_contains(get_minimap_space(ship->m_pos)) && !ship->m_detected_radar)
        {
            m_detections.push_back(std::make_pair(ship->m_pos, ImGui::GetTime()));
            ship->m_detected_radar = true;
        }

        // add_radial_gradient(g_render->get_dl("ui"), get_minimap_space(ship->m_pos), 6.0f, scan_r | (220 << 24), scan_l);
        // g_render->get_dl("ui")->AddCircleFilled(get_minimap_space(ship->m_pos), 2.0f, friend_col);
        // std::print("%d\n", viewport_contains(ship->m_pos));
    }

    for (auto& ship : enemy_ships)
    {
        if (scan_contains(get_minimap_space(ship->m_pos)) && !ship->m_detected_radar)
        {
            m_detections.push_back(std::make_pair(ship->m_pos, ImGui::GetTime()));
            ship->m_detected_radar = true;
        }

        // add_radial_gradient(g_render->get_dl("ui"), get_minimap_space(ship->m_pos), 6.0f, scan_r | (220 << 24), scan_l);
    }

    for (auto& [pos, detection_time] : m_detections)
    {
        int opacity = 0;
        if (detection_time > 1e-6f)
        {
            float time_since_detection = ImGui::GetTime() - detection_time;
            float fade_factor          = 1.0f - (time_since_detection / 0.75f);
            fade_factor                = std::max(0.0f, fade_factor);

            opacity = static_cast<int>(230 * fade_factor);
            // std::print("%i, %.1f, %.1f\n", opacity, fade_factor, time_since_detection);
        }

        add_radial_gradient(g_render->get_dl("ui").get(), get_minimap_space(pos), 6.0f, scan_r | (opacity << 24), scan_l);
    }

    // draw radar scan bar effect
    {
        // intialize
        if (glm::near_zero(m_scan_pos))
        {
            m_scan_pos = m_minimap_pos - glm::vec2(m_scan_size.x, 0.0f);
            generate_clusters();
        }

        static float last_scan_time = 0.0f;

        // reset the scan if it's finished
        if (m_scan_pos.x - m_minimap_pos.x > m_minimap_size.x + m_scan_size.x)
        {
            m_scan_pos.x   = m_minimap_pos.x - m_scan_size.x;
            last_scan_time = ImGui::GetTime();

            // generate new ground clutter for the next scan
            generate_clusters();

            for (auto& ship : friendly_ships)
                ship->m_detected_radar = false;

            for (auto& ship : enemy_ships)
                ship->m_detected_radar = false;

            m_detections.clear();
        }

        // start the scan if it's been long enough since the last
        if (ImGui::GetTime() - last_scan_time > 3.0f)
        {
            g_render->get_dl("ui")->AddRectFilledMultiColor(m_scan_pos, m_scan_pos + m_scan_size, scan_l, scan_r, scan_r, scan_l);
            m_scan_pos.x += 70.0f * ImGui::GetIO().DeltaTime;
        }
    }

    // draw viewport rect, the bitwise op just sets the alpha channel
    g_render->get_dl("ui")->AddRect(get_minimap_space(m_viewport_min), get_minimap_space(m_viewport_max), grid_col | (120 << 24));
    g_render->get_dl("ui")->PopClipRect();
}

bool Map::viewport_contains(const glm::vec2& pos)
{
    return ImRect(m_viewport_min, m_viewport_max).Contains(pos);
}

bool Map::viewport_contains(const ImRect& rect)
{
    return ImRect(m_viewport_min, m_viewport_max).Overlaps(rect);
}

bool Map::scan_contains(const glm::vec2& pos)
{
    return ImRect(m_scan_pos, m_scan_pos + (m_scan_size - glm::vec2(10.0, 0.0))).Contains(pos);
}

glm::vec2 Map::get_minimap_space(const glm::vec2& world_space)
{
    const glm::vec2 minimap_space = ((world_space / m_map_size) * m_minimap_size) + m_minimap_pos;
    return minimap_space;
}

glm::vec2 Map::get_map_space(const glm::vec2& world_space)
{
    return glm::vec2();
}
