#include "chess/search/evaluation.hpp"

namespace chess::search {

int get_game_phase(const board::Board& board) {
    int phase_value = 0;
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present()) {
            switch (p.type()) {
                case pieces::PieceType::Knight: phase_value += 1; break;
                case pieces::PieceType::Bishop: phase_value += 1; break;
                case pieces::PieceType::Rook:   phase_value += 2; break;
                case pieces::PieceType::Queen:  phase_value += 4; break;
                default: break;
            }
        }
    }

    if (phase_value > 24) phase_value = 24;
    return (phase_value * 256) / 24;
}

} // namespace chess::search
