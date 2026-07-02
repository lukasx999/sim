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

            while (not WindowShouldClose()) {
                BeginDrawing();
                on_iter();
                EndDrawing();
            }

            CloseWindow();

        }

    private:
        enum class Mode {
            WIRE,
            RESISTOR,
            VOLTAGE_SOURCE,
        };

        Mode m_mode = Mode::WIRE;
        std::vector<std::unique_ptr<Component>> m_components;

        bool m_is_drawing_wire = false;
        Point m_wire_start;

        [[nodiscard]] static std::string_view stringify_mode(Mode mode) {
            switch (mode) {
                using enum Mode;

                case WIRE: return "wire";
                case RESISTOR: return "resistor";
                case VOLTAGE_SOURCE: return "voltage source";
            }

            std::unreachable();
        }

        void on_iter() {

            ClearBackground(BLACK);

            draw_grid();
            draw_crosshair();
            auto cursor = Point(GetMousePosition()).align_to_grid();

            DrawText(std::string(stringify_mode(m_mode)).c_str(), 0, 0, 50, WHITE);

            if (IsKeyPressed(KEY_W))
                m_mode = Mode::WIRE;

            else if (IsKeyPressed(KEY_R))
                m_mode = Mode::RESISTOR;

            else if (IsKeyPressed(KEY_V))
                m_mode = Mode::VOLTAGE_SOURCE;

            switch (m_mode) {
                using enum Mode;

                case WIRE: {

                    auto diff = cursor - m_wire_start;
                    auto cursor_prime = diff.x > diff.y
                        ? Point(cursor.x, m_wire_start.y)
                        : Point(m_wire_start.x, cursor.y);

                    if (m_is_drawing_wire) {
                        DrawLineV(m_wire_start, cursor_prime, WHITE);
                    }

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                        if (not m_is_drawing_wire) {
                            m_is_drawing_wire = true;
                            m_wire_start = cursor;

                        } else {
                            m_components.push_back(std::make_unique<Wire>(m_wire_start / GRID_CELL_SIZE, cursor_prime / GRID_CELL_SIZE));
                            m_wire_start = cursor;
                        }

                    }

                    if (IsKeyPressed(KEY_Q)) {
                        m_is_drawing_wire = false;
                    }

                } break;

                case RESISTOR: {

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        m_components.push_back(std::make_unique<Resistor>(cursor / GRID_CELL_SIZE));
                    }

                } break;

                case VOLTAGE_SOURCE: {

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        m_components.push_back(std::make_unique<VoltageSource>(cursor / GRID_CELL_SIZE));
                    }

                } break;
            }

            for (auto& component : m_components) {
                component->draw();
            }

        }

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
            int text_size = 30;

            for (int x = 0; x < width; x += GRID_CELL_SIZE) {
                auto label = std::format("{}", x / GRID_CELL_SIZE);
                DrawText(label.c_str(), x, 0, text_size, GRID_COLOR);
                DrawLine(x, 0, x, height, GRID_COLOR);
            }

            for (int y = 0; y < height; y += GRID_CELL_SIZE) {
                auto label = std::format("{}", y / GRID_CELL_SIZE);
                DrawText(label.c_str(), 0, y, text_size, GRID_COLOR);
                DrawLine(0, y, width, y, GRID_COLOR);
            }

        }

};

int main() {

    Simulator simulator;
    simulator.run();

}