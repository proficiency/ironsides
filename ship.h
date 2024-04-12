#pragma once
#include <string>
#include <vector>
#include <deque>
#include <memory>

#include "typedefs.h"

enum SOCKET_TYPE
{
    SOCKET_NONE = 0,
    SOCKET_TURRET,
    SOCKET_TORPEDO,
    SOCKET_BRIDGE,
};

class Socket
{
public:
    glm::vec2   m_pos; // relative position or offset from parent ship's position
    float       m_rotation;
    SOCKET_TYPE m_type;
};

class Ship
{
public:
    Ship() : m_name(), m_pos(), m_dst(), m_speed(25), m_rotation(), m_image(), m_wake_nodes(){};
    Ship(const std::string& name, const Image& img) : m_name(name), m_pos(), m_dst(), m_speed(25), m_rotation(), m_image(img), m_wake_nodes() {}

    void update_destination(const glm::vec2& dst);
    void sail();
    void draw(ImDrawList* dl, const ImVec2& min, const ImVec2& max, u8 zoom_level);
    void update_wake();
    void draw_wake(ImDrawList* dl, const ImVec2& min, const ImVec2& max, u8 zoom_level);
    void update_sockets();
    void draw_sockets();

    std::string           m_name;
    glm::vec2             m_pos;
    glm::vec2             m_dst;
    float                 m_speed;
    float                 m_rotation;
    Image                 m_image;
    std::deque<glm::vec2> m_wake_nodes;
    std::shared_ptr<Ship> m_target;
};
