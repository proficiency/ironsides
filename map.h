#pragma once
#include <vector>
#include <memory>
#include <glm\glm.hpp>

class Ship;

class Map : public std::enable_shared_from_this<Map>
{
    // draw the scrolling water effect using noise textures
    void draw_water_effect();

    // generate the false return clusters for the minimap
    void generate_clusters();

    // point, intensity
    using Clutter = std::pair<glm::vec2, float>;
    class Clutter_Cluster
    {
    public:
        Clutter_Cluster() : m_cluster(), m_detection_time() {}

        static constexpr float m_cluster_radius = 100.0f;

        std::vector<Clutter> m_cluster;
        float                m_detection_time;
    };

    // position, detection time
    using Detection = std::pair<glm::vec2, float>;

    std::vector<Detection>       m_detections;
    std::vector<Clutter_Cluster> m_clutter;
    glm::vec2                    m_minimap_pos;
    glm::vec2                    m_minimap_size;
    glm::vec2                    m_scan_pos;
    glm::vec2                    m_scan_size;

public:
    Map();

    bool dragging_minimap();
    void update();

    // draw background, overlapping noise, etc
    void draw_map();

    // this will need to be rendered above everything
    void draw_minimap(const std::vector<std::shared_ptr<Ship>>& friendly_ships, const std::vector<std::shared_ptr<Ship>>& enemy_ships);

    bool viewport_contains(const glm::vec2& pos);
    bool viewport_contains(const ImRect& rect);
    bool scan_contains(const glm::vec2& pos);

    glm::vec2 get_minimap_space(const glm::vec2& world_space);
    glm::vec2 get_map_space(const glm::vec2& world_space);

    inline std::shared_ptr<Map> get_shared_from_this()
    {
        return shared_from_this();
    }

    glm::vec2 m_map_size;
    glm::vec2 m_drag_offset;
    glm::vec2 m_viewport_min;
    glm::vec2 m_viewport_max;
};

inline std::shared_ptr<Map> g_map;