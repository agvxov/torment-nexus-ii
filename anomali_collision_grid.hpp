#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

/* Splits the map into a regular grid,
 *  each cell caches the anomalies found there.
 * Used to optimize collision checking of anomalies vs stalkers.
 * Note that the size of stalkers is always one unit, they move every frame,
 *  cannot collide with each other and are short-lived;
 *  consequently we do not try to optimize them in any similar way.
 */

constexpr int collision_cell_size  = 20;
constexpr int collision_grid_w     = W / collision_cell_size;
constexpr int collision_grid_h     = H / collision_cell_size;
constexpr int collision_grid_count = collision_grid_w * collision_grid_h;

std::array<std::vector<entt::entity>, collision_grid_count> anomali_collision_grid;

static
int clamp_collision_coord(float v, int max_cell) {
    int cell = static_cast<int>(std::floor(v / collision_cell_size));
    if (cell < 0) return 0;
    if (cell > max_cell) return max_cell;
    return cell;
}

static
void hitbox_to_cell_range(const hitbox_t& hb, int& min_cx, int& min_cy, int& max_cx, int& max_cy) {
    min_cx = clamp_collision_coord(hb.x, collision_grid_w - 1);
    min_cy = clamp_collision_coord(hb.y, collision_grid_h - 1);

    // Treat the box as [x, x + w) / [y, y + h)
    const float right  = std::nextafter(hb.x + hb.w, -std::numeric_limits<float>::infinity());
    const float bottom = std::nextafter(hb.y + hb.h, -std::numeric_limits<float>::infinity());

    max_cx = clamp_collision_coord(right, collision_grid_w - 1);
    max_cy = clamp_collision_coord(bottom, collision_grid_h - 1);
}

static
void erase_entity_from_vector(std::vector<entt::entity>& vec, entt::entity e) {
    vec.erase(std::remove(vec.begin(), vec.end(), e), vec.end());
}

// ---------------

int position2collision_grid_index(float x, float y) {
    const int cx = clamp_collision_coord(x, collision_grid_w - 1);
    const int cy = clamp_collision_coord(y, collision_grid_h - 1);
    return cy * collision_grid_w + cx;
}

void register_new_anomali_collision(const entt::entity e, const hitbox_t position) {
    int min_cx, min_cy, max_cx, max_cy;
    hitbox_to_cell_range(position, min_cx, min_cy, max_cx, max_cy);

    for (int cy = min_cy; cy <= max_cy; ++cy) {
        for (int cx = min_cx; cx <= max_cx; ++cx) {
            anomali_collision_grid[cy * collision_grid_w + cx].push_back(e);
        }
    }
}

void register_modified_anomali_collision(
    const entt::entity e,
    const hitbox_t position,
    const hitbox_t old_position
) {
    int old_min_cx, old_min_cy, old_max_cx, old_max_cy;
    hitbox_to_cell_range(old_position, old_min_cx, old_min_cy, old_max_cx, old_max_cy);

    for (int cy = old_min_cy; cy <= old_max_cy; ++cy) {
        for (int cx = old_min_cx; cx <= old_max_cx; ++cx) {
            erase_entity_from_vector(anomali_collision_grid[cy * collision_grid_w + cx], e);
        }
    }

    register_new_anomali_collision(e, position);
}

const std::vector<entt::entity> & colliding_anomalies(float x, float y) {
    const int index = position2collision_grid_index(x, y);
    return anomali_collision_grid[index];
}
