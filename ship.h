#pragma once
#include <string>
#include <vector>
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
    glm::vec2   m_pos; // relative position
    float       m_rotation;
    SOCKET_TYPE m_type;
};

class Ship
{
public:
    Ship() {}
};
