#pragma once

#include <string>
#include <string_view>

#include <raylib.h>

#include "point.hpp"

// TODO: move somewhere else
inline const int NODE_RADIUS = 5;

class Component {
    public:
        Component() = default;
        virtual ~Component() = default;

        virtual void draw() const = 0;

        [[nodiscard]] virtual Point terminal1() const = 0;
        [[nodiscard]] virtual Point terminal2() const = 0;
        [[nodiscard]] virtual std::string_view label() const = 0;

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
            DrawLineV(m_start * GRID_CELL_SIZE, m_end * GRID_CELL_SIZE, m_color);
        }

        [[nodiscard]] Point terminal1() const override {
            return m_start;
        }

        [[nodiscard]] Point terminal2() const override {
            return m_end;
        }

        [[nodiscard]] std::string_view label() const override {
            return "";
        }

    private:
        Point m_start;
        Point m_end;

        static inline const Color m_color = WHITE;

};

class VoltageSource : public Component {
    public:
        VoltageSource(Point position, std::string label)
        : m_position(position)
        , m_label(std::move(label))
        { }

        [[nodiscard]] Point position() const {
            return m_position;
        }

        [[nodiscard]] Point terminal1() const override {
            return { m_position.x, m_position.y - m_terminal_distance };
        }

        [[nodiscard]] Point terminal2() const override {
            return { m_position.x, m_position.y + m_terminal_distance };
        }

        [[nodiscard]] std::string_view label() const override {
            return m_label;
        }

        void draw() const override {

            int radius = m_terminal_distance * GRID_CELL_SIZE / 2;
            auto scaled_pos = m_position * GRID_CELL_SIZE;
            auto scaled_term_pos = terminal1() * GRID_CELL_SIZE;
            auto scaled_term_neg = terminal2() * GRID_CELL_SIZE;

            DrawCircleLinesV(scaled_pos, radius, m_color);

            DrawLineV(scaled_pos, scaled_term_pos, m_color);
            DrawLineV(scaled_pos, scaled_term_neg, m_color);

            DrawCircleV(scaled_term_pos, NODE_RADIUS, m_color);
            DrawCircleV(scaled_term_neg, NODE_RADIUS, m_color);
        }

    private:
        Point m_position;
        int m_voltage = 10;
        const std::string m_label;

        static const int m_terminal_distance = 2; // distance from center

};

class Resistor : public Component {
    public:
        Resistor(Point position, std::string label)
        : m_position(position)
        , m_label(std::move(label))
        { }

        [[nodiscard]] Point position() const {
            return m_position;
        }

        [[nodiscard]] Point terminal1() const override {
            return { m_position.x, m_position.y - m_terminal_distance };
        }

        [[nodiscard]] Point terminal2() const override {
            return { m_position.x, m_position.y + m_terminal_distance };
        }

        [[nodiscard]] std::string_view label() const override {
            return m_label;
        }

        void draw() const override {

            auto scaled_pos = m_position * GRID_CELL_SIZE;
            auto scaled_term_pos = terminal1() * GRID_CELL_SIZE;
            auto scaled_term_neg = terminal2() * GRID_CELL_SIZE;

            int y = scaled_pos.y - m_terminal_distance * GRID_CELL_SIZE / 2;
            int width = m_terminal_distance * GRID_CELL_SIZE / 2;
            DrawRectangleLines(scaled_pos.x - width / 2, y, width, m_terminal_distance * GRID_CELL_SIZE, WHITE);

            DrawLineV(scaled_term_pos, scaled_term_pos + Point(0, m_terminal_distance * GRID_CELL_SIZE / 2), WHITE);
            DrawLineV(scaled_term_neg, scaled_term_neg - Point(0, m_terminal_distance * GRID_CELL_SIZE / 2), WHITE);

            DrawCircleV(scaled_term_pos, NODE_RADIUS, WHITE);
            DrawCircleV(scaled_term_neg, NODE_RADIUS, WHITE);

            DrawText(m_label.c_str(), width + m_position.x * GRID_CELL_SIZE, m_position.y * GRID_CELL_SIZE, 30, WHITE);
        }

    private:
        Point m_position;
        int m_resistance = 10'000;
        const std::string m_label;

        static const int m_terminal_distance = 2; // distance from center

};