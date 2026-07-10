#include "chess/search/evaluation.hpp"

namespace chess::search {

int evaluate_material(const board::Board& board) {
    int score = 0;
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present()) {
            int val = 0;
            switch (p.type()) {
                case pieces::PieceType::Pawn:   val = kPawnValue; break;
                case pieces::PieceType::Knight: val = kKnightValue; break;
                case pieces::PieceType::Bishop: val = kBishopValue; break;
                case pieces::PieceType::Rook:   val = kRookValue; break;
                case pieces::PieceType::Queen:  val = kQueenValue; break;
                default: break;
            }
            if (p.color() == pieces::Color::White) {
                score += val;
            } else {
                score -= val;
            }
        }
    }
    return score;
}

} // namespace chess::search
