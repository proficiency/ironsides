#include "render.h"
#include "components.h"
#include "ship.h"

void Ship::update()
{
    sail();

    if (is_alive())
    {
        heal();
        update_path();

        // update children components
        // todo: this + setting up images should go in an init func
        if (m_components.empty())
            m_components.push_back(std::make_shared<Socket_Turret>(get_shared_from_this()));

        for (auto& component : m_components)
            component->update();
    }
}

void Ship::draw(ImDrawList* dl, const ImVec2& min, const ImVec2& max, u8 zoom_level)
{
    auto ship_pos = m_pos + min;

    // draw ship image, set up screenspace bounds
    {
        // evil static keyword
        static ImVec2 uvs[4] = {ImVec2(0.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 1.0f)};

        const float cos_a = glm::cos(m_rotation);
        const float sin_a = glm::sin(m_rotation);

        // todo: we don't need to calculate this every time this func is called
        m_screenspace_bounds = glm::mat4x2(
            glm::vec2(-m_image.m_size.x * 0.5f, -m_image.m_size.y * 0.5f),
            glm::vec2(+m_image.m_size.x * 0.5f, -m_image.m_size.y * 0.5f),
            glm::vec2(+m_image.m_size.x * 0.5f, +m_image.m_size.y * 0.5f),
            glm::vec2(-m_image.m_size.x * 0.5f, +m_image.m_size.y * 0.5f)
        );

        glm::mat2 rotation(cos_a, sin_a, -sin_a, cos_a);
        for (u32 i = 0; i < 4; ++i)
            m_screenspace_bounds[i] = ship_pos + rotation * m_screenspace_bounds[i];

        // disable blending for this cuz it resulted in a 1px grey border around the texture when using multiple framebuffers, not sure why
        g_render->get_dl("game")->AddCallback([](const ImDrawList*, const ImDrawCmd*) { glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }, nullptr);
        g_render->get_dl("game")->AddImageQuad(
            ImTextureID(m_image.m_texture),
            m_screenspace_bounds[0],
            m_screenspace_bounds[1],
            m_screenspace_bounds[2],
            m_screenspace_bounds[3],
            uvs[0],
            uvs[1],
            uvs[2],
            uvs[3],
            IM_COL32(255, 255, 255, m_alpha)
        );
        g_render->get_dl("game")->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    }

    // misc info text
    g_render->get_dl("game")->AddText({ship_pos.x, ship_pos.y + ImGui::GetFontSize()}, IM_COL32(255, 255, 255, m_alpha), m_name.c_str());
    g_render->get_dl("game")->AddText(
        {ship_pos.x, ship_pos.y + ImGui::GetFontSize() * 2.0f}, IM_COL32(255, 255, 255, m_alpha), std::to_string((i32)m_hp).c_str()
    );

    // wish_move circle
    // todo: little fade out animation for this on click
    g_render->get_dl("game")->AddCircleFilled(m_dst + min, 10.0f, IM_COL32(255, 0, 0, m_alpha));

    // draw children components
    for (auto& component : m_components)
        component->draw();
}

// todo: flip ship's direction if it's sailing out of bounds
// todo: if a ship is getting too close to another ship, veer off a little bit to avoid a collision?
void Ship::sail()
{
    // we died, come to a stop
    if (!is_alive())
    {
        accelerate(0.0f);
        return;
    }

    const glm::vec2 forward = get_forward();

    // todo: fix this up
    if (glm::near_zero(m_dst) || glm::distance(m_pos, m_dst) < 10.0f)
        m_dst = m_pos + forward * 100.0f;

    if (m_target_speed < 1e-6f)
        m_target_speed = m_max_speed;

    // angle between current rotation and desired direction
    const glm::vec2 desired_dir  = glm::normalize(m_dst - m_pos);
    float           target_angle = glm::orientedAngle(forward, desired_dir);

    // todo: fix this. rotation needs to take into account current speed. some inertia would be nice too.
    const float delta_time          = ImGui::GetIO().DeltaTime;
    float       max_rotation_speed  = 0.15f;
    float       angle_delta         = glm::clamp(target_angle, -max_rotation_speed * delta_time, max_rotation_speed * delta_time);
    m_rotation                     += angle_delta;
    m_rotation                      = glm::mod(m_rotation + glm::pi<float>(), 2.0f * glm::pi<float>()) - glm::pi<float>();

    // accel(or decel) towards target speed
    accelerate(m_target_speed);
    // m_speed     = m_max_speed;

    m_velocity  = forward * m_speed;
    m_pos      += m_velocity * delta_time;
}

void Ship::accelerate(const float target_speed)
{
    constexpr float acceleration = 3.0f;
    const float     delta        = target_speed - m_speed;
    const float     step         = acceleration * ImGui::GetIO().DeltaTime;

    // accel
    if (delta > 0.0f)
        m_speed = std::min(m_speed + step, target_speed);

    // decel
    else if (delta < 0.0f)
        m_speed = std::max(m_speed - step, target_speed);
}

void Ship::update_path()
{
    m_path.push_back(m_pos);
}

/*void Ship::update_wake()
{
    if (m_speed < 1e-6f)
        return;

    const glm::vec2 stern_position = glm::rotate(glm::vec2(-m_image.m_size.x * 0.5f, 0.0f), m_rotation) + m_pos;

    if (m_wake_nodes.empty() || glm::distance(stern_position, m_wake_nodes.back()) > 25.0f)
        m_wake_nodes.push_back(stern_position);

    if (m_wake_nodes.size() > 5u)
        m_wake_nodes.pop_front();
}*/

// todo: draw a caret shape rotated to the angle between the previous wake position and the next, also they should fade out as they get further from
// the stern of the ship making the wake
/* void Ship::draw_wake(ImDrawList* dl, const ImVec2& min, const ImVec2& max, u8 zoom_level)
{
    for (auto& node : m_wake_nodes)
        dl->AddCircleFilled(node + min, 2.0f, IM_COL32(230, 230, 230, 255));
}*/

void Ship::select_target(const std::vector<std::shared_ptr<Ship>>& enemy_ships)
{
    if (enemy_ships.empty())
    {
        m_target           = nullptr;
        m_target_candidate = nullptr;
        return;
    }

    if (m_target != nullptr && !m_target->is_alive())
    {
        m_target           = nullptr;
        m_target_candidate = nullptr;
    }

    if (m_target_candidate == nullptr)
        m_target_candidate = enemy_ships.back();

    for (auto& enemy : enemy_ships)
    {
        if (!enemy->is_alive())
            continue;

        if (glm::length(enemy->m_pos - m_pos) < glm::length(m_target_candidate->m_pos - m_pos))
            m_target_candidate = enemy;
    }

    m_target = m_target_candidate;
}

void Ship::take_damage(const std::shared_ptr<Ship>& attacker)
{
    float damage = g_rng.get_random(55, 90);

    if (g_rng.get_random(1, 10) == 10)
        damage *= 2.0f;

    // god bless america
    if (attacker->is_enemy() && !is_enemy())
        damage *= 1.65f;

    m_hp             = std::max(m_hp - damage, 0.0f);
    m_last_hurt_time = ImGui::GetTime();

    std::print(
        "%s attacked %s for %d, new hp is %d, last hurt time is %.1f\n",
        attacker->m_name.c_str(),
        m_name.c_str(),
        (i32)damage,
        (i32)m_hp,
        m_last_hurt_time
    );
}

void Ship::heal()
{
    if (ImGui::GetTime() - m_last_hurt_time > 1.0f)
        m_hp = std::min(m_hp + (1.0f * ImGui::GetIO().DeltaTime), 100.0f);
}

bool Ship::handle_death()
{
    draw_sink_animation();

    for (auto& component : m_components)
        component->handle_death();

    return m_sink_anim_done;
}

void Ship::draw_sink_animation()
{
    m_alpha          = std::max(m_alpha - (50.0f * ImGui::GetIO().DeltaTime), 0.0f);
    m_sink_anim_done = m_alpha < 1e-6f;
}

ImU32 Ship::get_debug_color()
{
    // todo: store this as a member
    const u32 hash = Fnv_Hash::hash(m_name);

    return IM_COL32((hash & 0xFF0000) >> 16u, (hash & 0x00FF00) >> 8u, hash & 0x0000FF, 255);
}
