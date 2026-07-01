#pragma once

#include <cmath>
#include <cstdint>

#include <raylib.h>

// TODO: move this somewhere else
inline const Color GRID_COLOR = DARKGRAY;
inline const int GRID_CELL_SIZE = 50;

struct Point {
    uint32_t x = 0;
    uint32_t y = 0;

    Point() = default;

    Point(uint32_t x, uint32_t y)
    : x(x)
    , y(y)
    { }

    Point(Vector2 vec)
    : x(vec.x)
    , y(vec.y)
    { }

    [[nodiscard]] Point align_to_grid() const {
        return {
            static_cast<uint32_t>(roundf(static_cast<float>(x) / GRID_CELL_SIZE) * GRID_CELL_SIZE),
            static_cast<uint32_t>(roundf(static_cast<float>(y) / GRID_CELL_SIZE) * GRID_CELL_SIZE),
        };
    }

    bool operator<=>(const Point& other) const = default;

    Point operator*(uint32_t value) const {
        return { x * value, y * value };
    }

    Point operator+(const Point& other) const {
        return { x + other.x, y + other.y };
    }

    Point operator-(const Point& other) const {
        return { x - other.x, y - other.y };
    }

    operator Vector2() const {
        return Vector2(x, y);
    }

};