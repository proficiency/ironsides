#include "render.h"
#include "map.h"
#include "components.h"
#include "ship.h"

void Socket_Turret::fire()
{
    if (!m_can_fire || !m_on_target)
        return;

    m_can_fire = false;
    m_needs_fx = true;
}

void Socket_Turret::reload()
{
    if (m_reload_time <= 1e-6f)
    {
        // spawn projectile
        if (m_projectile == nullptr)
            m_projectile = std::make_unique<Projectile>(get_shared_from_this(), m_ship_parent->m_target);

        // todo: store these as a "reload time remaining" and a "base reload time" member in the class
        m_reload_time = 3.0f + g_rng.get_random(0.1f, 2.5f);
        m_can_fire    = true;
        return;
    }

    m_reload_time -= ImGui::GetIO().DeltaTime;
}

void Socket_Turret::slew()
{
    // if the target is nullptr, then our target should be our own bow
    if (m_ship_parent->m_target == nullptr)
        m_target_pos = m_pos + glm::vec2(glm::cos(m_ship_parent->m_rotation), glm::sin(m_ship_parent->m_rotation));

    else
        m_target_pos = m_ship_parent->m_target->m_pos;

    m_pos = glm::rotate(glm::vec2(39.0f, 0.0f), m_ship_parent->m_rotation) + m_ship_parent->m_pos;    

    const glm::vec2 forward      = glm::vec2(glm::cos(m_rotation), glm::sin(m_rotation));
    const glm::vec2 desired_dir  = glm::normalize(m_target_pos - m_pos);
    float           target_angle = glm::orientedAngle(forward, desired_dir);

    // todo: fix this. rotation needs to take into account current speed. some inertia would be nice too.
    const float delta_time          = ImGui::GetIO().DeltaTime;
    float       max_rotation_speed  = 0.85f;
    float       angle_delta         = glm::clamp(target_angle, -max_rotation_speed * delta_time, max_rotation_speed * delta_time);
    m_rotation                     += angle_delta;
    // m_rotation                     += max_rotation_speed * delta_time;
    m_rotation  = glm::mod(m_rotation + glm::pi<float>(), 2.0f * glm::pi<float>()) - glm::pi<float>();
    m_on_target = glm::abs(angle_delta) <= 1e-6f;
}

// todo: periodically pick random spot along the ship to target
void Socket_Turret::update()
{
    slew();

    if (m_ship_parent->m_target == nullptr)
    {
        m_projectile = nullptr;
        return;
    }

    fire();
    reload();

    // a shot happened, send out our projectile
    if (m_needs_fx && m_projectile != nullptr)
    {
        m_projectile->update();

        if (m_projectile->m_target == nullptr || !m_projectile->m_target->is_alive())
        {
            m_needs_fx   = false;
            m_projectile = nullptr;
        }

        // todo: proper hit detection       
        else if (glm::distance(m_projectile->m_pos, m_projectile->m_dst) < 15.0f)
        {
            m_projectile = nullptr;
            m_ship_parent->m_target->take_damage(m_ship_parent);
        }

        // our projectile has lived for way longer than expected, it probably missed
        else if (m_projectile->m_lifetime >= m_projectile->m_expected_lifetime + 1.0f)
        {
            m_projectile = nullptr;
            m_needs_fx   = false;
        }
    }
}

void Socket_Turret::draw()
{
    const glm::vec2 turret_center = glm::rotate(glm::vec2(-4.0f, 0.0f), m_rotation);
    const glm::mat4 rotation      = glm::rotate(glm::mat4(1.0f), m_rotation, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec2 corners[4] = {
        glm::vec2(-m_image.m_size.x * 0.5f, -m_image.m_size.y * 0.5f),
        glm::vec2(+m_image.m_size.x * 0.5f, -m_image.m_size.y * 0.5f),
        glm::vec2(+m_image.m_size.x * 0.5f, +m_image.m_size.y * 0.5f),
        glm::vec2(-m_image.m_size.x * 0.5f, +m_image.m_size.y * 0.5f)};

    for (auto& corner : corners)
    {
        const glm::vec4 translated = glm::vec4(corner - turret_center, 0.0f, 1.0f);

        corner = glm::vec2(rotation * translated) + turret_center;
    }

    // todo: set up a cast operator in ironsides_imconfig.h for this
    ImVec2 uvs[4]   = {ImVec2(0.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 1.0f)};
    ImVec2 quads[4] = {
        glm::vec2(m_pos + g_render->m_min + g_map->m_drag_offset) + corners[0],
        glm::vec2(m_pos + g_render->m_min + g_map->m_drag_offset) + corners[1],
        glm::vec2(m_pos + g_render->m_min + g_map->m_drag_offset) + corners[2],
        glm::vec2(m_pos + g_render->m_min + g_map->m_drag_offset) + corners[3],
    };

    //g_render->m_dl->AddImageQuad((ImTextureID)m_image.m_texture, quads[0], quads[1], quads[2], quads[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32(255,255,255, m_ship_parent->m_alpha));
    g_render->m_dl->AddCircleFilled(m_pos + turret_center + g_render->m_min + g_map->m_drag_offset, 3.0f, IM_COL32(200, 90, 20, 255));

    if (m_needs_fx && m_projectile != nullptr)
        m_projectile->draw();
}

Projectile::Projectile(const std::shared_ptr<Socket>& socket_parent, const std::shared_ptr<Ship>& target) :
    m_socket_parent(socket_parent), m_target(target)
{
    m_pos = m_socket_parent->m_pos + glm::rotate(glm::vec2(5.0f, 0.0f), m_socket_parent->m_rotation);
}

void Projectile::update()
{
    const float delta_time     = ImGui::GetIO().DeltaTime;
    const float time_to_target = glm::distance(m_target->m_pos, m_pos) / m_speed;

    if (m_expected_lifetime < 1e-6f)
        m_expected_lifetime = time_to_target;

    if (glm::near_zero(m_dst))
        m_dst = m_target->m_pos + m_target->m_velocity * time_to_target;

    if (glm::near_zero(m_direction))
        m_direction = glm::normalize(m_dst - m_pos);

    m_pos      += m_direction * (m_speed * delta_time);
    m_lifetime += delta_time;
}

void Projectile::draw() 
{
    g_render->m_dl->AddCircleFilled(m_pos + g_render->m_min + g_map->m_drag_offset, 2.0f, IM_COL32(200, 200, 200, 255));
}

void Socket::handle_death() 
{
    // todo: idk explosion or something?
}
