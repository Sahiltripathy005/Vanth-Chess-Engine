#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"

namespace chess::search {

int evaluate_bishop_pair(const board::Board& board) {
    int score = 0;
    int white_bishops = 0;
    int black_bishops = 0;

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present() && p.type() == pieces::PieceType::Bishop) {
            if (p.color() == pieces::Color::White) {
                white_bishops++;
            } else {
                black_bishops++;
            }
        }
    }

    if (white_bishops >= 2) {
        score += kBishopPairBonus;
    }
    if (black_bishops >= 2) {
        score -= kBishopPairBonus;
    }

    return score;
}

} // namespace chess::search
