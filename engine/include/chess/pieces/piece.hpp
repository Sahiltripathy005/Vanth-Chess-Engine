#ifndef CHESS_PIECE_HPP
#define CHESS_PIECE_HPP

#include "color.hpp"
#include "piece_type.hpp"
#include <optional>

namespace chess::pieces {

class Piece {
public:
    constexpr Piece() : type_(PieceType::None), color_(Color::None) {}
    constexpr Piece(PieceType type, Color color) : type_(type), color_(color) {}

    constexpr PieceType type() const { return type_; }
    constexpr Color color() const { return color_; }

    constexpr bool is_none() const { return type_ == PieceType::None; }
    constexpr bool is_present() const { return type_ != PieceType::None; }

    constexpr bool operator==(const Piece& other) const {
        return type_ == other.type_ && color_ == other.color_;
    }

    constexpr bool operator!=(const Piece& other) const {
        return !(*this == other);
    }

    static std::optional<Piece> from_char(char c);
    char to_char() const;

private:
    PieceType type_;
    Color color_;
};

} // namespace chess::pieces

#endif // CHESS_PIECE_HPP
