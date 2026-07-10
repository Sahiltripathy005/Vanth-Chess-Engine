#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>
#include <cmath>

namespace chess::search {

int evaluate_endgame(const board::Board& board, int phase) {
    // Only evaluate heavily in late game/endgame (phase < 150)
    if (phase > 150) return 0;

    int white_score = 0;
    int black_score = 0;

    // Find Kings and count pawns
    pieces::Square white_king_sq = pieces::Square::None();
    pieces::Square black_king_sq = pieces::Square::None();
    int white_pawns = 0;
    int black_pawns = 0;
    int white_pieces = 0;
    int black_pieces = 0;

    std::vector<pieces::Square> pawn_squares;

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_none()) continue;

        if (p.type() == pieces::PieceType::King) {
            if (p.color() == pieces::Color::White) white_king_sq = sq;
            else black_king_sq = sq;
        } else if (p.type() == pieces::PieceType::Pawn) {
            pawn_squares.push_back(sq);
            if (p.color() == pieces::Color::White) white_pawns++;
            else black_pawns++;
        } else {
            if (p.color() == pieces::Color::White) white_pieces++;
            else black_pieces++;
        }
    }

    // 1. King Activity (centralization and closeness to pawns)
    if (!white_king_sq.is_none()) {
        // Closeness to pawns: compute distance to all pawns
        int dist_sum = 0;
        for (const auto& p_sq : pawn_squares) {
            dist_sum += std::abs(white_king_sq.file() - p_sq.file()) + std::abs(white_king_sq.rank() - p_sq.rank());
        }
        if (!pawn_squares.empty()) {
            // Reward smaller distance
            white_score += (14 * static_cast<int>(pawn_squares.size()) - dist_sum) * 2;
        }
    }
    if (!black_king_sq.is_none()) {
        int dist_sum = 0;
        for (const auto& p_sq : pawn_squares) {
            dist_sum += std::abs(black_king_sq.file() - p_sq.file()) + std::abs(black_king_sq.rank() - p_sq.rank());
        }
        if (!pawn_squares.empty()) {
            black_score += (14 * static_cast<int>(pawn_squares.size()) - dist_sum) * 2;
        }
    }

    // 2. Opposition Detection
    // Direct opposition: Kings are on the same file or rank with exactly one square between them.
    if (!white_king_sq.is_none() && !black_king_sq.is_none()) {
        int file_diff = std::abs(white_king_sq.file() - black_king_sq.file());
        int rank_diff = std::abs(white_king_sq.rank() - black_king_sq.rank());

        if ((file_diff == 2 && rank_diff == 0) || (file_diff == 0 && rank_diff == 2)) {
            // The side whose turn it is NOT has the opposition.
            if (board.active_color() == pieces::Color::White) {
                // Black has the opposition
                black_score += 25;
            } else {
                // White has the opposition
                white_score += 25;
            }
        }
    }

    // 3. Simplification Bonus
    // Encourages trading pieces when ahead, and trading pawns when behind
    int material_diff = evaluate_material(board);
    int total_pieces = white_pieces + black_pieces;

    if (material_diff > 150) {
        // White is ahead, encourage trading pieces (smaller total_pieces)
        white_score += (16 - total_pieces) * 8;
    } else if (material_diff < -150) {
        // Black is ahead
        black_score += (16 - total_pieces) * 8;
    }

    // Scale evaluation with game phase: more endgame knowledge applied as phase approaches 0
    int weight = (150 - phase); // 0 to 150
    return ((white_score - black_score) * weight) / 150;
}

} // namespace chess::search
