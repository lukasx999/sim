#pragma once

#include <algorithm>
#include <functional>
#include <print>
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

        void split_wire_if_needed(Point point) {

            auto intersecting_wire = std::ranges::find_if(m_components, [&] (const std::unique_ptr<Component>& component) {
                bool is_wire = dynamic_cast<Wire*>(component.get()) != nullptr;
                if (not is_wire) return false;

                auto start = component->terminal1();
                auto end = component->terminal2();

                // no need to split at the direct start/end of the wire
                auto start_ = Vector2MoveTowards(start, end, 1);
                auto end_ = Vector2MoveTowards(end, start, 1);

                return CheckCollisionPointLine(point, start_, end_, 1);
            });

            bool need_split = intersecting_wire != m_components.end();
            if (need_split) {
                auto start = (*intersecting_wire)->terminal1();
                auto end = (*intersecting_wire)->terminal2();
                m_components.erase(intersecting_wire);
                m_components.push_back(std::make_unique<Wire>(start, point));
                m_components.push_back(std::make_unique<Wire>(point, end));
            }

        }

        void place_wire(Point point) {

            // TODO: also split wire of terminals of other components are in the way (eg: connecting 3 parallel resistors)

            // if the start/end of the wire is on the middle of another wire, split that wire into two,
            // to create an intersection
            split_wire_if_needed(point);
            split_wire_if_needed(m_wire_start);

            m_components.push_back(std::make_unique<Wire>(m_wire_start, point));
            m_wire_start = point;
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
                            place_wire(cursor);

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

        [[nodiscard]] std::vector<Component*> components_at_point(Point point) const {
            auto is_connected = [&](Component* component) {
                return component->terminal1() == point or component->terminal2() == point;
            };

            return m_components
            | std::views::transform(&std::unique_ptr<Component>::get)
            | std::views::filter(is_connected)
            | std::ranges::to<std::vector<Component*>>();
        }

        /// @returns a list of components at the given point, paired with their local root point.
        /// since this function eliminates wires, the local root point of a component might be different from the given root parameter.
        [[nodiscard]] auto components_at_point_no_wires(Point root, std::unordered_set<Point>& visited) const
        -> std::vector<std::pair<Component*, Point>> {
            std::vector<std::pair<Component*, Point>> components;

            std::queue<Point> frontier;

            frontier.push(root);
            visited.insert(root);

            while (not frontier.empty()) {
                auto node = frontier.front();
                frontier.pop();

                auto children = components_at_point(node);

                for (auto& child : children) {

                    bool is_wire = dynamic_cast<Wire*>(child) != nullptr;
                    if (is_wire) {
                        auto point = child->opposite_terminal(node);
                        if (visited.contains(point)) continue;

                        frontier.push(point);
                        visited.insert(point);

                    } else {
                        components.push_back({child, node});
                    }

                }

            }

            return components;
        }

        [[nodiscard]] std::vector<Point> end_of_junction_explore_path(Point root, std::unordered_set<Point>& visited) const {

            std::vector<Point> path;
            std::queue<Point> frontier;

            frontier.push(root);
            visited.insert(root);

            while (not frontier.empty()) {
                auto node = frontier.front();
                frontier.pop();

                path.push_back(node);
                std::println("{}", node);

                auto children = components_at_point_no_wires(node, visited);

                for (auto& [child, child_root] : children) {
                    auto successor = child->opposite_terminal(child_root);
                    if (visited.contains(successor)) continue;

                    frontier.push(successor);
                    visited.insert(successor);
                }

            }

            return path;
        }

        [[nodiscard]] Point end_of_junction(Point root, std::unordered_set<Point>& visited) const {
            auto children = components_at_point_no_wires(root, visited);

            auto paths = children
                | std::views::transform(&std::pair<Component*, Point>::second)
                | std::views::filter([&](Point child_root) {
                    return not visited.contains(child_root);
                })
                | std::views::transform([&](Point child_root) {
                    return end_of_junction_explore_path(child_root, visited);
                })
                | std::ranges::to<std::vector<std::vector<Point>>>();

            auto intersections = std::ranges::fold_left(paths, std::vector<Point>{}, [&](std::vector<Point> acc, const std::vector<Point>& path) {
                std::vector<Point> out;
                std::ranges::set_intersection(acc, path, std::back_inserter(out));
                return out;
            });

            for (auto& p : intersections) {
                std::println("{}", p);
            }

            return {};
            // return intersections.front();
        }

        /// @brief computes the resistance of the subcircuit between the given points
        [[nodiscard]] Resistance traverse_subcircuit(Point root, std::optional<Point> end,
            std::unordered_set<Point>& visited, const std::vector<Point>& to_ignore) const {

            Resistance r_total = 0;
            Point node = root;

            while (true) {

                if (end.has_value() and node == end)
                    break;

                Resistance g_total = 0;
                auto children = components_at_point_no_wires(node, visited);

                auto unvisited_children = children
                    | std::views::filter([&](const auto& pair) {
                        auto& [child, child_root] = pair;
                        bool should_be_ignored = std::ranges::find(to_ignore, child_root) != to_ignore.end();
                        return not visited.contains(child_root) and not should_be_ignored;
                    })
                    | std::ranges::to<std::vector<std::pair<Component*, Point>>>();

                if (unvisited_children.empty()) {
                    break;

                } else if (unvisited_children.size() == 1) {
                    auto& [child, child_root] = children.front();

                    auto res = dynamic_cast<Resistor*>(child)->resistance();
                    auto successor = child->opposite_terminal(child_root);

                    visited.insert(child_root);
                    r_total += res;
                    node = successor;

                } else {
                    for (auto& [child, child_root] : children) {

                        auto ignore_set = unvisited_children
                            | std::views::transform(&std::pair<Component*, Point>::second)
                            | std::views::filter([&](Point point) {
                                return point != child_root;
                            })
                            | std::ranges::to<std::vector<Point>>();

                        auto junction_end = end_of_junction(root, visited);
                        auto res = traverse_subcircuit(child_root, junction_end, visited, ignore_set);

                        g_total += 1 / res;
                        node = junction_end;
                    }
                    r_total += 1 / g_total;

                }

            }

            return r_total;
        }

        void simulate() const {
            Point root(10, 10);

            std::unordered_set<Point> visited;
            end_of_junction_explore_path(root, visited);
            // auto end = end_of_junction(root, visited);
            // std::println("end: {} {}", end.x, end.y);

            // std::unordered_set<Point> visited;
            // auto r = traverse_subcircuit(root, get_end_of_junction(root), visited, {});
            // std::println("r: {}", r);
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