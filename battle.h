#pragma once
#include <ranges>

class Ship;
class Formation_Controller;
class Formation;

class Battle : public std::enable_shared_from_this<Battle>
{
    float m_last_spawn_time;

public:
    Battle();

    std::vector<std::shared_ptr<Ship>>      m_ships;
    std::vector<std::shared_ptr<Ship>>      m_enemy_ships;
    std::vector<std::shared_ptr<Formation>> m_enemy_formations;
    std::shared_ptr<Formation_Controller>   m_formation_controller;

    std::shared_ptr<Battle> get_shared_from_this()
    {
        return shared_from_this();
    }

    void init();
    void update();
    void draw();
    void spawn_enemies();
    void control_ai();
    void draw_target_arrows(); // todo: maybe instead of target arrows, a radar display? slightly transparent in one of the corners
};
