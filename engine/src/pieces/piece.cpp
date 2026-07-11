#include "chess/pieces/piece.hpp"
#include <cctype>

namespace chess::pieces {

std::optional<Piece> Piece::from_char(char c) {
    Color color = std::isupper(static_cast<unsigned char>(c)) ? Color::White : Color::Black;
    char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    PieceType type;
    switch (lower) {
        case 'p': type = PieceType::Pawn; break;
        case 'n': type = PieceType::Knight; break;
        case 'b': type = PieceType::Bishop; break;
        case 'r': type = PieceType::Rook; break;
        case 'q': type = PieceType::Queen; break;
        case 'k': type = PieceType::King; break;
        default: return std::nullopt;
    }
    return Piece(type, color);
}

char Piece::to_char() const {
    if (is_none()) {
        return '.';
    }
    char c = pieces::to_char(type_);
    if (color_ == Color::White) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return c;
}

} // namespace chess::pieces
