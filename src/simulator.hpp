#pragma once

#include <algorithm>
#include <functional>
#include <numeric>
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

        /// @returns a list of components at the given point, paired with their local root point,
        /// aswell as all of the intermediate points that have been visited. this is because this function ignores wires,
        /// and therefore the given root parameter might not be the same root as the local root of the component.
        [[nodiscard]] auto components_at_point_no_wires(Point root) const
        -> std::pair<std::vector<std::pair<Component*, Point>>, std::vector<Point>>
        {
            std::vector<std::pair<Component*, Point>> components;
            std::vector<Point> visited_nodes;

            std::queue<Point> frontier;
            std::unordered_set<Point> visited;

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
                        visited_nodes.push_back(point);

                    } else {
                        components.push_back({child, node});
                    }

                }

            }

            return {components, visited_nodes};
        }

        // TODO: code for calculating total circuit resistance:
        // auto conductances = children
        //     | std::views::filter([&](const auto& pair) {
        //         auto& [child, child_root] = pair;
        //         auto point = child->opposite_terminal(child_root);
        //         return not visited.contains(point);
        //     })
        //     | std::views::transform([](const auto& pair) {
        //         auto& [child, child_root] = pair;
        //         return child;
        //     })
        //     | std::views::filter([](Component* child) {
        //         return dynamic_cast<Resistor*>(child) != nullptr;
        //     })
        //     | std::views::transform([](Component* child) {
        //         auto resistor = dynamic_cast<Resistor*>(child);
        //         assert(resistor != nullptr);
        //         return 1.0 / resistor->resistance();
        //     });
        //
        // if (not conductances.empty())
        //     resistance += 1.0 / std::ranges::fold_left(conductances, 0.0, std::plus<Resistance>());

        void traverse_circuit(Point root) const {

            std::queue<Point> frontier;
            std::unordered_set<Point> visited;

            frontier.push(root);
            visited.insert(root);

            while (not frontier.empty()) {
                auto node = frontier.front();
                frontier.pop();

                auto [children, intermediate_path] = components_at_point_no_wires(node);
                for (auto& p : intermediate_path) {
                    visited.insert(p);
                }

                for (auto& [child, child_root] : children) {
                    assert(dynamic_cast<Wire*>(child) == nullptr);
                    auto point = child->opposite_terminal(child_root);

                    if (visited.contains(point)) continue;
                    frontier.push(point);
                    visited.insert(point);

                    std::print("{}, ", child->label());
                }
                std::println();

            }

        }

        /// @brief calls the given function for all parallel children of the given root. the function should
        /// return a list of booleans indicating whether to keep traversing the circuit from the given
        /// child node.
        using TraverseFn = std::vector<bool>(std::span<const Component*> children);
        void traverse_circuit_lambda(Point root, std::function<TraverseFn> fn) const {

            std::queue<Point> frontier;
            std::unordered_set<Point> visited;

            frontier.push(root);
            visited.insert(root);

            while (not frontier.empty()) {
                auto node = frontier.front();
                frontier.pop();

                auto [children, intermediate_path] = components_at_point_no_wires(node);
                for (auto& p : intermediate_path) {
                    visited.insert(p);
                }

                auto remove_visited_children = [&](const auto& pair) {
                    auto& [child, child_root] = pair;
                    assert(dynamic_cast<Wire*>(child) == nullptr);
                    auto point = child->opposite_terminal(child_root);
                    return not visited.contains(point);
                };

                auto unvisited_children = children
                    | std::views::filter(remove_visited_children)
                    | std::views::transform(&std::pair<Component*, Point>::first)
                    | std::ranges::to<std::vector<const Component*>>();

                auto should_visit_list = fn(unvisited_children);
                for (auto&& [should_visit, pair] : std::views::zip(should_visit_list, children)) {
                    if (not should_visit) continue;

                    auto& [child, child_root] = pair;
                    auto point = child->opposite_terminal(child_root);
                    frontier.push(point);
                    visited.insert(point);
                }

            }

        }

        void simulate() const {
            Point root(10, 10);

            // traverse_circuit(root);

            traverse_circuit_lambda(root, [&](std::span<const Component*> children) -> std::vector<bool> {
                for (auto& child : children) {
                    std::print("{}, ", child->label());
                }
                std::println();

                return children
                | std::views::transform([](auto&) {
                    return true;
                })
                | std::ranges::to<std::vector<bool>>();
            });
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