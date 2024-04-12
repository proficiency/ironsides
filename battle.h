#pragma once
#include "random.h"

class Render;
class Ship;
class Battle
{
    std::vector<std::shared_ptr<Ship>> m_ships;
    std::vector<std::shared_ptr<Ship>> m_enemy_ships;
    std::vector<std::shared_ptr<Ship>> m_selected_ships;
    RNG                                m_rng;
    std::shared_ptr<Render>            m_render_parent;

public:
    Battle(const std::shared_ptr<Render>& render_parent) : m_render_parent(render_parent), m_zoom_level(FONT_MEDIUM) {}
    u8 m_zoom_level;

    void init();
    void update();
    void draw();
    void control_formation();
    void control_ai();
    void control_torpedo();
};
