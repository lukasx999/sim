#pragma once

#include <print>
#include <vector>
#include <cassert>
#include <ranges>
#include <memory>

#include <raylib.h>
#include <raymath.h>

#include "point.hpp"
#include "components.hpp"

class Simulator {
    public:
        Simulator() = default;

        void run() {

            #if 0
            auto v = std::make_unique<VoltageSource>(Point(2, 6), "V0");
            auto r = std::make_unique<Resistor>(Point(8, 6), "R1");
            m_components.push_back(std::make_unique<Wire>(v->terminal_pos(), r->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(r->terminal_neg(), v->terminal_neg()));
            m_components.push_back(std::move(v));
            m_components.push_back(std::move(r));
            #endif

            // current divider parallel
            #if 1
            auto v = std::make_unique<VoltageSource>(Point(2, 6), "V0");
            auto r1 = std::make_unique<Resistor>(Point(8, 6), "R1");
            auto r2 = std::make_unique<Resistor>(Point(11, 6), "R2");
            m_components.push_back(std::make_unique<Wire>(v->terminal_pos(), r1->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(r1->terminal_neg(), v->terminal_neg()));
            m_components.push_back(std::make_unique<Wire>(r1->terminal_pos(), r2->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(r2->terminal_neg(), r1->terminal_neg()));
            m_components.push_back(std::move(v));
            m_components.push_back(std::move(r1));
            m_components.push_back(std::move(r2));
            #endif

            // voltage divider series
            #if 0
            auto v = std::make_unique<VoltageSource>(Point(2, 6), "V0");
            auto r1 = std::make_unique<Resistor>(Point(8, 6), "R1");
            auto r2 = std::make_unique<Resistor>(Point(8, 13), "R2");
            m_components.push_back(std::make_unique<Wire>(v->terminal_pos(), r1->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(r1->terminal_neg(), r2->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(r2->terminal_neg(), Point(2, 15)));
            m_components.push_back(std::make_unique<Wire>(Point(2, 15), v->terminal_neg()));
            m_components.push_back(std::move(v));
            m_components.push_back(std::move(r1));
            m_components.push_back(std::move(r2));
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
                        m_components.push_back(std::make_unique<Resistor>(cursor / GRID_CELL_SIZE, "R"));
                    }

                } break;

                case VOLTAGE_SOURCE: {

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        m_components.push_back(std::make_unique<VoltageSource>(cursor / GRID_CELL_SIZE, "V"));
                    }

                } break;
            }

            for (auto& component : m_components) {
                component->draw();
            }

            simulate();

        }

        struct Node {
            std::vector<std::unique_ptr<Node>> m_children;
            enum class Type { SERIES, PARALLEL } m_type;
            Component* m_component = nullptr;
        };

        [[nodiscard]] std::unique_ptr<Node> get_subcircuit_parallel(const Component& component) const {
            auto point = component.terminal_pos();

            auto components = m_components
                | std::views::transform(&std::unique_ptr<Component>::get)
                | std::views::filter([&](Component* c) {
                    return c->terminal_pos() == point and c != &component;
                })
                | std::ranges::to<std::vector<Component*>>();

            assert(not components.empty());

            if (components.size() == 1) {
                return std::make_unique<Node>(std::vector<std::unique_ptr<Node>>{}, Node::Type::PARALLEL, components.at(0));
            }

            auto children = components
                | std::views::transform([&](Component* c) {
                    return get_subcircuit_series(*c);
                })
                | std::ranges::to<std::vector<std::unique_ptr<Node>>>();

            return std::make_unique<Node>(std::move(children), Node::Type::PARALLEL, nullptr);
        }

        [[nodiscard]] std::unique_ptr<Node> get_subcircuit_series(const Component& component) const {
            auto node = std::make_unique<Node>();
            auto point = component.terminal_pos();

            while (point != component.terminal_neg()) {
                auto component_it = std::ranges::find_if(m_components, [&](const std::unique_ptr<Component>& c) {
                    return c->terminal_pos() == point and c.get() != &component;
                });
                assert(component_it != m_components.end());

                Component& comp = **component_it;
                node->m_children.push_back(get_subcircuit_parallel(comp));
                std::println("{}", comp.label());

                point = comp.terminal_neg();
            }

            return node;
        }

        void simulate() const {

            auto v0 = std::ranges::find_if(m_components, [](const std::unique_ptr<Component>& component) {
                return dynamic_cast<VoltageSource*>(component.get()) != nullptr;
            });
            assert(v0 != m_components.end());

            // get_subcircuit_series(**v0);
            get_subcircuit_parallel(**v0);

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