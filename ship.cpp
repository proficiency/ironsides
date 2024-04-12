#include "render.h"
#include "ship.h"

void Ship::sail()
{
    // is our destination null?
    if (glm::length(m_dst) == 0.0f)
        return;

    // time constant
    float delta_time = ImGui::GetIO().DeltaTime;

    // get direction between dst and pos
    glm::vec2 pos         = {m_pos.x, m_pos.y};
    glm::vec2 desired_dir = glm::normalize(m_dst - pos);

    // angle between our current rotation and the desired direction
    float target_angle = glm::orientedAngle(glm::vec2(glm::cos(m_rotation), glm::sin(m_rotation)), desired_dir);

    // update rotation
    float max_rotation_acceleration  = 0.15f;
    float max_rotation_speed         = 0.5f;
    float angle_delta                = glm::clamp(target_angle, -max_rotation_speed * delta_time, max_rotation_speed * delta_time);
    m_rotation                      += angle_delta;

    // cap rotation to -180 +180
    m_rotation = glm::mod(m_rotation + glm::pi<float>(), 2.0f * glm::pi<float>()) - glm::pi<float>();

    // calculate linear accel
    float     max_linear_acceleration = 1.0f;
    float     max_linear_speed        = 25.0f;
    glm::vec2 current_dir             = glm::vec2(glm::cos(m_rotation), glm::sin(m_rotation));
    glm::vec2 velocity                = current_dir * m_speed;
    glm::vec2 desired_velocity        = desired_dir * max_linear_speed;
    glm::vec2 acceleration            = (desired_velocity - velocity) * max_linear_acceleration;

    // extrapolate velocity
    velocity += acceleration * delta_time;

    // cap linear speed
    float speed = glm::length(velocity);
    if (speed > max_linear_speed)
    {
        velocity = glm::normalize(velocity) * max_linear_speed;
    }

    // extrapolate position via velocity
    pos += velocity * delta_time;

    m_pos = ImVec2(pos.x, pos.y);
}

void Ship::draw(ImDrawList* dl, const ImVec2& min, const ImVec2& max, u8 zoom_level)
{
    auto rotated_image = [](const Image& img, ImDrawList* dl, const ImVec2& center, float rotation)
    {
        ImVec2 size  = img.m_size;
        float  cos_a = std::cosf(rotation);
        float  sin_a = std::sinf(rotation);

        ImVec2 pos[4] = {
            center + ImRotate(ImVec2(-size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
            center + ImRotate(ImVec2(+size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
            center + ImRotate(ImVec2(+size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a),
            center + ImRotate(ImVec2(-size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a)};

        ImVec2 uvs[4] = {ImVec2(0.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 1.0f)};

        dl->AddImageQuad(ImTextureID(img.m_texture), pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
        dl->PathClear();
        dl->PathLineTo(center);
        dl->PathLineTo(center + ImVec2(cosf(rotation), std::sinf(rotation)) * 150.0f);
        dl->PathStroke(IM_COL32(255, 255, 255, 255), false);
        dl->AddCircleFilled(center, 10.0f, IM_COL32(255, 0, 0, 175));
    };

    auto ship_pos = m_pos + min;

    draw_wake(dl, min, max, zoom_level);

    // draw ship image
    rotated_image(m_image, dl, ship_pos, m_rotation);

    // center circle
    dl->AddCircleFilled(ship_pos, 10.0f, IM_COL32(0, 0, 255, 175));

    // misc. info text
    dl->AddText(
        {ship_pos.x, ship_pos.y + ImGui::GetFontSize()},
        IM_COL32(255, 255, 255, 255),
        std::to_string(i32(m_rotation * 180.0f / glm::pi<float>())).c_str()
    );
    dl->AddText({ship_pos.x, ship_pos.y + ImGui::GetFontSize() * 2}, IM_COL32(255, 255, 255, 255), m_name.c_str());

    // target line
    if (m_target)
        dl->AddLine(ship_pos, m_target->m_pos + min, IM_COL32(255, 0, 0, 255));

    // wish_move circle
    dl->AddCircleFilled(m_dst + min, 10.0f, IM_COL32(255, 0, 0, 175));
}

void Ship::update_destination(const glm::vec2& dst)
{
    m_dst = dst;
}

void Ship::update_wake()
{
    // todo: find a good distance
    if (m_wake_nodes.empty() || glm::length(m_pos - m_wake_nodes.back()) > 25.0f)
        m_wake_nodes.push_back(m_pos);

    if (m_wake_nodes.size() > 5u)
        m_wake_nodes.pop_front();
}

// i would like to draw a caret shape rotated to the angle between the previous wake position and the next
void Ship::draw_wake(ImDrawList* dl, const ImVec2& min, const ImVec2& max, u8 zoom_level)
{
    for (auto& node : m_wake_nodes)
        dl->AddCircleFilled(node + min, 2.0f, IM_COL32(230, 230, 230, 255));
}
