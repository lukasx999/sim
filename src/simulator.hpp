#pragma once

#include <print>
#include <unordered_map>
#include <unordered_set>
#include <queue>
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

            // current divider parallel
            #if 0
            auto r1 = std::make_unique<Resistor>(Point(8, 6), "R1");
            auto r2 = std::make_unique<Resistor>(Point(11, 6), "R2");
            m_components.push_back(std::make_unique<Wire>(r1->terminal_pos(), r2->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(r2->terminal_neg(), r1->terminal_neg()));
            m_components.push_back(std::move(r1));
            m_components.push_back(std::move(r2));
            #endif

            // voltage divider series
            #if 0
            auto r1 = std::make_unique<Resistor>(Point(8, 6), "R1");
            auto r2 = std::make_unique<Resistor>(Point(8, 13), "R2");
            m_components.push_back(std::make_unique<Wire>(r1->terminal_neg(), r2->terminal_pos()));
            m_components.push_back(std::move(r1));
            m_components.push_back(std::move(r2));
            #endif

            #if 0
            auto r1 = std::make_unique<Resistor>(Point(8, 6), "R1");
            m_components.push_back(std::move(r1));
            #endif

            #if 0
            auto r1 = std::make_unique<Resistor>(Point(8, 6), "R1");
            auto r2 = std::make_unique<Resistor>(Point(6, 12), "R2");
            auto r3 = std::make_unique<Resistor>(Point(10, 12), "R3");
            m_components.push_back(std::make_unique<Wire>(r1->terminal_neg(), Point(8, 10)));
            m_components.push_back(std::make_unique<Wire>(Point(8, 10), r2->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(Point(8, 10), r3->terminal_pos()));
            m_components.push_back(std::make_unique<Wire>(r2->terminal_neg(), Point(8, 14)));
            m_components.push_back(std::make_unique<Wire>(r3->terminal_neg(), Point(8, 14)));
            m_components.push_back(std::move(r1));
            m_components.push_back(std::move(r2));
            m_components.push_back(std::move(r3));
            #endif

            SetTraceLogLevel(LOG_ERROR);
            InitWindow(1920, 1080, "sim");
            SetExitKey(0);

            while (not WindowShouldClose()) {
                BeginDrawing();
                if (on_iter()) break;
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

        uint32_t m_label_count = 1;

        [[nodiscard]] static std::string_view stringify_mode(Mode mode) {
            switch (mode) {
                using enum Mode;

                case WIRE: return "wire";
                case RESISTOR: return "resistor";
                case VOLTAGE_SOURCE: return "voltage source";
            }

            std::unreachable();
        }

        bool on_iter() {

            ClearBackground(BLACK);

            draw_grid();
            DrawText(std::string(stringify_mode(m_mode)).c_str(), 0, 0, 50, WHITE);

            if (IsKeyPressed(KEY_W))
                m_mode = Mode::WIRE;

            else if (IsKeyPressed(KEY_R))
                m_mode = Mode::RESISTOR;

            else if (IsKeyPressed(KEY_V))
                m_mode = Mode::VOLTAGE_SOURCE;

            else if (IsKeyPressed(KEY_S))
                simulate();

            else if (IsKeyPressed(KEY_Q))
                return true;

            auto cursor = Point(GetMousePosition()).align_to_grid() / GRID_CELL_SIZE;

            switch (m_mode) {
                using enum Mode;

                case WIRE: {
                    draw_crosshair();
                    // TODO: wire splitting at junction

                    if (m_is_drawing_wire) {
                        DrawLineV(m_wire_start * GRID_CELL_SIZE, cursor * GRID_CELL_SIZE, WHITE);
                    }

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                        if (m_is_drawing_wire) {

                            auto intersecting_wire = std::ranges::find_if(m_components, [&] (const std::unique_ptr<Component>& component) {
                                bool is_wire = dynamic_cast<Wire*>(component.get()) != nullptr;
                                auto start = component->terminal1();
                                auto end = component->terminal2();

                                // no need to split at the direct start/end of the wire
                                auto start_ = Vector2MoveTowards(start, end, 1);
                                auto end_ = Vector2MoveTowards(end, start, 1);

                                bool need_split = CheckCollisionPointLine(cursor, start_, end_, 1);
                                return is_wire and need_split;
                            });

                            bool need_split = intersecting_wire != m_components.end();
                            if (need_split) {
                                auto start = (*intersecting_wire)->terminal1();
                                auto end = (*intersecting_wire)->terminal2();
                                m_components.erase(intersecting_wire);
                                m_components.push_back(std::make_unique<Wire>(start, cursor));
                                m_components.push_back(std::make_unique<Wire>(cursor, end));
                            }

                            m_components.push_back(std::make_unique<Wire>(m_wire_start, cursor));
                            m_wire_start = cursor;

                        } else {
                            m_is_drawing_wire = true;
                            m_wire_start = cursor;
                        }

                    }

                    if (IsKeyPressed(KEY_ESCAPE)) {
                        m_is_drawing_wire = false;
                    }

                } break;

                case RESISTOR: {

                    Resistor(cursor, "R").draw();

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        auto label = std::format("R{}", m_label_count++);
                        m_components.push_back(std::make_unique<Resistor>(cursor, std::move(label)));
                    }

                } break;

                case VOLTAGE_SOURCE: {

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        m_components.push_back(std::make_unique<VoltageSource>(cursor, "V"));
                    }

                } break;
            }

            for (auto& component : m_components) {
                component->draw();
            }

            return false;
        }

        struct Node {
            std::vector<std::unique_ptr<Node>> m_children;
            enum class Type { SERIES, PARALLEL } m_type;
            Component* m_component = nullptr;
        };

        [[nodiscard]] std::vector<Component*> get_components_at_point(Point point) const {
            return m_components
            | std::views::transform(&std::unique_ptr<Component>::get)
            | std::views::filter([&](Component* c) {
                return c->terminal1() == point or c->terminal2() == point;
            })
            | std::ranges::to<std::vector<Component*>>();
        }

        void traverse_circuit(Point root) const {

            std::queue<Point> frontier;
            std::unordered_set<Point> visited;

            frontier.push(root);
            visited.insert(root);

            while (not frontier.empty()) {
                auto node = frontier.front();
                frontier.pop();

                auto children = get_components_at_point(node);
                for (auto& child : children) {

                    Point point = child->terminal1() == node
                        ? child->terminal2()
                        : child->terminal1();

                    if (visited.contains(point)) continue;
                    frontier.push(point);
                    visited.insert(node);

                    std::println("{}", child->label());
                }

            }

        }

        void simulate() const {

            traverse_circuit(Point(8, 4));

        }

        void draw_crosshair() const {
            auto cursor = Point(GetMousePosition()).align_to_grid();
            DrawLine(cursor.x, 0, cursor.x, GetScreenHeight(), GRAY);
            DrawLine(0, cursor.y, GetScreenWidth(), cursor.y, GRAY);
        }

        void draw_grid() const {

            int width = GetScreenWidth();
            int height = GetScreenHeight();
            int text_size = 20;

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