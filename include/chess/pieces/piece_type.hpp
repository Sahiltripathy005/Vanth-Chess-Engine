#ifndef CHESS_PIECE_TYPE_HPP
#define CHESS_PIECE_TYPE_HPP

#include <cstdint>
#include <string_view>

namespace chess::pieces {

enum class PieceType : uint8_t {
    Pawn = 0,
    Knight = 1,
    Bishop = 2,
    Rook = 3,
    Queen = 4,
    King = 5,
    None = 6
};

constexpr std::string_view to_string(PieceType type) {
    switch (type) {
        case PieceType::Pawn:   return "Pawn";
        case PieceType::Knight: return "Knight";
        case PieceType::Bishop: return "Bishop";
        case PieceType::Rook:   return "Rook";
        case PieceType::Queen:  return "Queen";
        case PieceType::King:   return "King";
        default:                return "None";
    }
}

constexpr char to_char(PieceType type) {
    switch (type) {
        case PieceType::Pawn:   return 'p';
        case PieceType::Knight: return 'n';
        case PieceType::Bishop: return 'b';
        case PieceType::Rook:   return 'r';
        case PieceType::Queen:  return 'q';
        case PieceType::King:   return 'k';
        default:                return ' ';
    }
}

} // namespace chess::pieces

#endif // CHESS_PIECE_TYPE_HPP
