#ifndef CHESS_COLOR_HPP
#define CHESS_COLOR_HPP

#include <cstdint>
#include <string_view>

namespace chess::pieces {

enum class Color : uint8_t {
    White = 0,
    Black = 1,
    None = 2
};

constexpr Color opposite_color(Color color) {
    if (color == Color::White) return Color::Black;
    if (color == Color::Black) return Color::White;
    return Color::None;
}

constexpr std::string_view to_string(Color color) {
    switch (color) {
        case Color::White: return "White";
        case Color::Black: return "Black";
        default: return "None";
    }
}

} // namespace chess::pieces

#endif // CHESS_COLOR_HPP
