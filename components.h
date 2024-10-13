#pragma once

class Ship;

enum SOCKET_TYPE
{
    SOCKET_NONE = 0,
    SOCKET_TURRET,
    SOCKET_TORPEDO,
    SOCKET_BRIDGE,
};

class Ship;
class Socket;
class Projectile
{
public:
    Projectile(const std::shared_ptr<Socket>& socket_parent, const std::shared_ptr<Ship>& target);

    void update();
    void draw();

    glm::vec2               m_pos;
    glm::vec2               m_dst;
    glm::vec2               m_direction;
    float                   m_expected_lifetime;
    float                   m_lifetime;
    std::shared_ptr<Ship>   m_target;
    std::shared_ptr<Socket> m_socket_parent;
    static constexpr float  m_speed = 40.0f;
};

// how much damage should a torpedo do?
class Projectile_Torpedo : public Projectile
{

};

class Socket : public std::enable_shared_from_this<Socket>
{
    virtual void slew() = 0;

public:
    Socket(const std::shared_ptr<Ship>& ship_parent) : m_pos(), m_rotation(), m_type(SOCKET_NONE), m_ship_parent(ship_parent) {}
    Socket() : m_pos(), m_rotation(), m_type(SOCKET_NONE), m_ship_parent() {}

    void handle_death();

    glm::vec2                                m_pos; // relative position or offset from parent ship's position
    float                                    m_rotation;
    float                                    m_reload_time;
    bool                                     m_can_fire;
    bool                                     m_needs_fx;
    SOCKET_TYPE                              m_type;
    Image                                    m_image;
    std::shared_ptr<Ship>                    m_ship_parent;
    std::unique_ptr<Projectile>              m_projectile;
    glm::vec2                                m_target_pos;
    bool                                     m_on_target;

    auto get_shared_from_this()
    {
        return shared_from_this();
    }

    virtual void update() = 0;
    virtual void draw()   = 0;
};

class Socket_Turret : public Socket
{
    void fire();
    void reload();
    void slew() override;

public:
    Socket_Turret(const std::shared_ptr<Ship>& ship_parent)
    {
        m_ship_parent = ship_parent;
        m_type        = SOCKET_TURRET;
        m_image       = g_render->m_images.get("Component_Turret");
    }

    void update() override;
    void draw() override;
};

// todo: pressing 'space' will activate the torpedo launcher, slew to target(during this time the aiming reticle is red), and once it reaches the
// target rotation, the reticle will be yellow once the torpedo is fired(by releasing space), a torpedo will be launched.
// additionally, consider drawing a lead indicator at the estimated position the ship would be by the time the torpedo would arrive
class Socket_Torpedo : public Socket
{
private:
    glm::vec2 m_torpedo_pos;
    glm::vec2 m_torpedo_velocity;
    float     m_torpedo_speed;

public:
    virtual void slew_to_target() = 0;
    virtual void update()         = 0;
};
