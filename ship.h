#pragma once
#include <string>
#include <vector>
#include <deque>
#include <memory>

#include "typedefs.h"

class Socket;
class Socket_Torpedo;
class Socket_Turret;
class Formation;

enum Nationality : u8
{
    NAT_USA = 0,
    NAT_JAP,
    NAT_GER,

    NAT_MAX
};

class PID_Controller
{
public:
    PID_Controller() : m_integral(), m_last_error() {}

    inline float compute(float error, float delta_time)
    {
        m_integral             += error * delta_time;
        const float derivative  = (error - m_last_error) / delta_time;
        m_last_error            = error;

        return m_kp * error + m_ki * m_integral + m_kd * derivative;
    }

private:
    static constexpr float m_kp = 0.35f;
    static constexpr float m_ki = 0.01f;
    static constexpr float m_kd = 0.05f;
    float                  m_integral;
    float                  m_last_error;
};

class Ship : public std::enable_shared_from_this<Ship>
{
public:
    Ship() :
        m_name(),
        m_pos(),
        m_dst(),
        m_velocity(),
        m_speed(0.0f),
        m_target_speed(0.0f),
        m_max_speed(25.0f),
        m_rotation(),
        m_image(),
        m_wake_nodes(),
        m_sink_anim_done(),
        m_alpha(255.0f)

    {
    }

    Ship(const std::string& name, Nationality nationality) :
        m_name(name),
        m_pos(),
        m_dst(),
        m_velocity(),
        m_speed(0.0f),
        m_target_speed(0.0f),
        m_max_speed(25.0f),
        m_rotation(),
        m_hp(100.0f),
        m_wake_nodes(),
        m_sink_anim_done(),
        m_alpha(255.0f),
        m_nationality(nationality)
    {
        m_image            = g_render->m_images.get("Component_Hull");
    }

    void  update();
    void  draw(ImDrawList* dl, const ImVec2& min, const ImVec2& max, u8 zoom_level);
    void  sail();
    void  accelerate(const float target_speed);
    void  update_path();
    void  select_target(const std::vector<std::shared_ptr<Ship>>& enemy_ships);
    void  take_damage(const std::shared_ptr<Ship>& attacker);
    void  heal();
    bool  handle_death();
    ImU32 get_debug_color();

    inline glm::vec2 get_screenspace_pos()
    {
        return m_pos + g_render->m_min;
    }

    inline glm::vec2 get_stern_pos()
    {
        return glm::rotate(glm::vec2(-m_image.m_size.x * 0.5f, 0.0f), m_rotation) + m_pos;
    }

    inline glm::vec2 get_bow_pos()
    {
        return glm::rotate(glm::vec2(m_image.m_size.x * 0.5f, 0.0f), m_rotation) + m_pos;
    }

    inline glm::vec2 get_forward()
    {
        return glm::vec2(glm::cos(m_rotation), glm::sin(m_rotation));
    }

    inline glm::vec2 get_right()
    {
        const glm::vec2 forward = get_forward();
        return glm::vec2(-forward.y, forward.x);
    }

    inline bool is_enemy()
    {
        return m_nationality == NAT_JAP || m_nationality == NAT_GER;
    }

    inline bool is_alive()
    {
        return m_hp > 1e-6f;
    }

    std::shared_ptr<Ship> get_shared_from_this()
    {
        return shared_from_this();
    }

    std::string                          m_name;
    glm::vec2                            m_pos;
    glm::vec2                            m_dst;
    glm::vec2                            m_velocity;
    glm::mat4x2                          m_screenspace_bounds;
    float                                m_speed;
    float                                m_max_speed;
    float                                m_target_speed;
    float                                m_rotation;
    float                                m_hp;
    float                                m_last_hurt_time;
    bool                                 m_detected_radar;
    Nationality                          m_nationality;
    Image                                m_image;
    PID_Controller                       m_pid_controller;
    std::deque<glm::vec2>                m_wake_nodes;
    std::deque<glm::vec2>                m_path;
    std::shared_ptr<Ship>                m_target;
    std::shared_ptr<Formation>           m_parent_formation;
    std::vector<std::shared_ptr<Socket>> m_components;
    bool                                 m_sink_anim_done;
    float                                m_alpha;

private:
    void draw_sink_animation();

    std::shared_ptr<Ship> m_target_candidate;
};
