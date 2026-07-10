#ifndef CHESS_MOVE_HPP
#define CHESS_MOVE_HPP

#include "chess/pieces/square.hpp"
#include "chess/pieces/piece_type.hpp"
#include <string>

namespace chess::moves {

enum class MoveType : uint8_t {
    Normal,
    DoublePawnPush,
    EnPassant,
    Castling,
    Promotion
};

class Move {
public:
    constexpr Move() 
        : from_(pieces::Square::None()), 
          to_(pieces::Square::None()), 
          type_(MoveType::Normal), 
          promotion_(pieces::PieceType::None) {}

    constexpr Move(pieces::Square from, pieces::Square to, MoveType type = MoveType::Normal, pieces::PieceType promotion = pieces::PieceType::None)
        : from_(from), to_(to), type_(type), promotion_(promotion) {}

    constexpr pieces::Square from() const { return from_; }
    constexpr pieces::Square to() const { return to_; }
    constexpr MoveType type() const { return type_; }
    constexpr pieces::PieceType promotion() const { return promotion_; }

    constexpr bool operator==(const Move& other) const {
        return from_ == other.from_ && 
               to_ == other.to_ && 
               type_ == other.type_ && 
               promotion_ == other.promotion_;
    }

    constexpr bool operator!=(const Move& other) const {
        return !(*this == other);
    }

    std::string to_string() const;

private:
    pieces::Square from_;
    pieces::Square to_;
    MoveType type_;
    pieces::PieceType promotion_;
};

} // namespace chess::moves

#endif // CHESS_MOVE_HPP
