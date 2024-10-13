#pragma once

class Ship;
class Battle;

class Formation : public std::enable_shared_from_this<Formation>
{
public:
    std::vector<std::shared_ptr<Ship>> m_ships;
    float                              m_highest_distance;

    Formation(const std::vector<std::shared_ptr<Ship>>& selected_ships);

    bool operator== (const Formation& other) const
    {
        if (m_ships.size() != other.m_ships.size())
            return false;

        for (u32 i = 0; i < m_ships.size(); ++i)
        {
            if (m_ships[i] != other.m_ships[i])
                return false;
        }

        return true;
    }

    std::shared_ptr<Ship> get_leader()
    {
        return m_ships.front();
    }

    std::shared_ptr<Formation> get_shared_from_this()
    {
        return shared_from_this();
    }

    void update();
    glm::vec2 get_center();
};

class Formation_Controller
{
    std::vector<std::shared_ptr<Ship>> m_selected_ships;
    std::shared_ptr<Formation>         m_active_formation;
    ImVec2                             m_drag_src;
    ImVec2                             m_drag_dst;
    bool                               m_dragging;
    bool                               m_shift_dragging;

    void handle_ship_selection();

    // tries to find an existing parent formation for a given ship, returns [formation, index]
    std::pair<std::shared_ptr<Formation>, u32> find_in_existing_formation(const std::shared_ptr<Ship>& ship);

    // tries to find an identical formation of ships
    std::shared_ptr<Formation> find_existing_formation(const std::shared_ptr<Formation>& f);

    // finds if a ship is in an existing formation and removes it
    void remove_from_existing_formation(const std::shared_ptr<Ship>&);


public:
    Formation_Controller(const std::shared_ptr<Battle>& battle_parent);

    void update();
    void draw();

    ImRect                                  m_selection_rect;
    std::vector<std::shared_ptr<Formation>> m_formations;
    std::shared_ptr<Battle>                 m_battle_parent;
};
