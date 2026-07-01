#include <print>
#include <vector>
#include <cassert>
#include <cmath>
#include <ranges>
#include <memory>

#include <raylib.h>
#include <raymath.h>

#include "point.hpp"
#include "components.hpp"

#define DBG(stuff) std::println("{}: {}", #stuff, stuff);

class Simulator {
    public:
        Simulator() = default;

        void run() {

            #if 1
            auto v = std::make_unique<VoltageSource>(Point(2, 6));
            auto r = std::make_unique<Resistor>(Point(8, 6));
            m_components.push_back(std::make_unique<Wire>(v->terminal_pos(), r->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(v->terminal_neg(), r->terminal_neg()));
            m_components.push_back(std::move(v));
            m_components.push_back(std::move(r));
            #endif

            SetTraceLogLevel(LOG_ERROR);
            InitWindow(1600, 900, "sim");

            bool drawing_wire = false;
            Point wire_start;

            while (not WindowShouldClose()) {
                BeginDrawing();
                ClearBackground(BLACK);

                draw_grid();

                auto cursor = Point(GetMousePosition()).align_to_grid();

                draw_crosshair();

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
                        m_components.push_back(std::make_unique<Wire>(wire_start / GRID_CELL_SIZE, cursor_prime / GRID_CELL_SIZE));
                        wire_start = cursor;
                    }

                }

                if (IsKeyPressed(KEY_Q)) {
                    drawing_wire = false;
                }

                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    m_components.push_back(std::make_unique<VoltageSource>(cursor / GRID_CELL_SIZE));
                }

                if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
                    m_components.push_back(std::make_unique<Resistor>(cursor / GRID_CELL_SIZE));
                }

                for (auto& component : m_components) {
                    component->draw();
                }

                simulate();

                EndDrawing();
            }

            CloseWindow();

        }

    private:
        std::vector<std::unique_ptr<Component>> m_components;

        void simulate() const {

            auto v0_it = std::ranges::find_if(m_components, [](const std::unique_ptr<Component>& component) {
                return dynamic_cast<VoltageSource*>(component.get()) != nullptr;
            });
            assert(v0_it != m_components.end());

            VoltageSource& v0 = *dynamic_cast<VoltageSource*>(v0_it->get());
            auto pos = v0.terminal_pos();

        }

        void draw_crosshair() const {
            auto cursor = Point(GetMousePosition()).align_to_grid();
            DrawLine(cursor.x, 0, cursor.x, GetScreenHeight(), GRAY);
            DrawLine(0, cursor.y, GetScreenWidth(), cursor.y, GRAY);
        }

        void draw_grid() const {

            int width = GetScreenWidth();
            int height = GetScreenHeight();

            for (int x = 0; x < width; x += GRID_CELL_SIZE) {
                DrawLine(x, 0, x, height, GRID_COLOR);
            }

            for (int y = 0; y < height; y += GRID_CELL_SIZE) {
                DrawLine(0, y, width, y, GRID_COLOR);
            }

        }

};

int main() {

    Simulator simulator;
    simulator.run();

}