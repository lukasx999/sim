#include <print>
#include <vector>
#include <cmath>
#include <ranges>
#include <memory>

#include <raylib.h>
#include <raymath.h>

#include "point.hpp"

#define DBG(stuff) std::println("{}: {}", #stuff, stuff);

namespace {

    class Component {
        public:
            Component() = default;
            virtual ~Component() = default;

            virtual void draw() const = 0;

        protected:
            static inline const Color m_color = WHITE;

    };

    class Wire : public Component {
        public:
            Wire(Point start, Point end)
            : m_start(start)
            , m_end(end)
            { }

            void draw() const override {
                DrawLineV(m_start, m_end, m_color);
            }

        private:
            Point m_start;
            Point m_end;

            static inline const Color m_color = WHITE;

    };

    class VoltageSource : public Component {
        public:
            explicit VoltageSource(Point position)
            : m_position(position)
            { }

            void draw() const override {
                auto terminal = Point(0, m_terminal_distance * GRID_CELL_SIZE);

                int radius = GRID_CELL_SIZE / 2;
                DrawCircleLinesV(m_position, radius, m_color);

                DrawLineV(m_position, m_position + terminal, m_color);
                DrawLineV(m_position, m_position - terminal, m_color);

                DrawCircleV(m_position + terminal, 5, m_color);
                DrawCircleV(m_position - terminal, 5, m_color);
            }

        private:
            Point m_position;

            static const int m_terminal_distance = 1;

    };

    void draw_grid() {

        int width = GetScreenWidth();
        int height = GetScreenHeight();

        for (int x = 0; x < width; x += GRID_CELL_SIZE) {
            DrawLine(x, 0, x, height, GRID_COLOR);
        }

        for (int y = 0; y < height; y += GRID_CELL_SIZE) {
            DrawLine(0, y, width, y, GRID_COLOR);
        }

    }

} // namespace

int main() {

    std::vector<std::unique_ptr<Component>> components;

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(1600, 900, "sim");

    bool drawing_wire = false;
    Point wire_start;

    while (not WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        draw_grid();

        auto cursor = Point(GetMousePosition()).align_to_grid();

        DrawLine(cursor.x, 0, cursor.x, GetScreenHeight(), WHITE);
        DrawLine(0, cursor.y, GetScreenWidth(), cursor.y, WHITE);

        // VoltageSource(cursor).draw();

        auto diff = cursor - wire_start;
        auto cursor_prime = diff.x > diff.y
            ? Point(cursor.x, wire_start.y)
            : Point(wire_start.x, cursor.y);

        if (drawing_wire) {
            DrawLineV(wire_start, cursor_prime, WHITE);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

            if (not drawing_wire) {
                drawing_wire = true;
                wire_start = cursor;

            } else {
                components.push_back(std::make_unique<Wire>(wire_start, cursor_prime));
                wire_start = cursor;
            }

        }

        if (IsKeyPressed(KEY_Q)) {
            drawing_wire = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            components.push_back(std::make_unique<VoltageSource>(cursor));
        }

        for (auto& component : components) {
            component->draw();
        }

        EndDrawing();
    }

    CloseWindow();
}