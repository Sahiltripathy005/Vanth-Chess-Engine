#include "chess/search/see.hpp"
#include <cmath>
#include <algorithm>

namespace chess::search {

static int get_see_piece_value(pieces::PieceType type) {
    switch (type) {
        case pieces::PieceType::Pawn:   return 100;
        case pieces::PieceType::Knight: return 320;
        case pieces::PieceType::Bishop: return 330;
        case pieces::PieceType::Rook:   return 500;
        case pieces::PieceType::Queen:  return 900;
        case pieces::PieceType::King:   return 20000;
        default: return 0;
    }
}

static bool is_line_clear(const board::Board& board, pieces::Square from, pieces::Square to) {
    int f1 = from.file();
    int r1 = from.rank();
    int f2 = to.file();
    int r2 = to.rank();

    int df = f2 - f1;
    int dr = r2 - r1;

    int step_f = (df == 0) ? 0 : ((df > 0) ? 1 : -1);
    int step_r = (dr == 0) ? 0 : ((dr > 0) ? 1 : -1);

    int f = f1 + step_f;
    int r = r1 + step_r;

    while (f != f2 || r != r2) {
        if (board.piece_at(pieces::Square(static_cast<uint8_t>(f), static_cast<uint8_t>(r))).is_present()) {
            return false;
        }
        f += step_f;
        r += step_r;
    }
    return true;
}

static bool is_piece_attacking(const board::Board& board, pieces::Square from, pieces::Square to) {
    pieces::Piece p = board.piece_at(from);
    if (!p.is_present()) return false;

    int f1 = from.file();
    int r1 = from.rank();
    int f2 = to.file();
    int r2 = to.rank();

    int df = f2 - f1;
    int dr = r2 - r1;

    switch (p.type()) {
        case pieces::PieceType::Pawn: {
            int dir = (p.color() == pieces::Color::White) ? 1 : -1;
            return (dr == dir) && (std::abs(df) == 1);
        }
        case pieces::PieceType::Knight: {
            return (std::abs(df) == 1 && std::abs(dr) == 2) || (std::abs(df) == 2 && std::abs(dr) == 1);
        }
        case pieces::PieceType::King: {
            return (std::abs(df) <= 1 && std::abs(dr) <= 1);
        }
        case pieces::PieceType::Bishop: {
            return (std::abs(df) == std::abs(dr)) && is_line_clear(board, from, to);
        }
        case pieces::PieceType::Rook: {
            return (df == 0 || dr == 0) && is_line_clear(board, from, to);
        }
        case pieces::PieceType::Queen: {
            return (std::abs(df) == std::abs(dr) || df == 0 || dr == 0) && is_line_clear(board, from, to);
        }
        default:
            return false;
    }
}

static int see_evaluate(board::Board& board, pieces::Square target_sq, pieces::Color active_color) {
    // Find the least valuable attacker of active_color attacking target_sq
    pieces::Square attacker_sq = pieces::Square::None();
    int min_val = 999999;
    pieces::PieceType attacker_type = pieces::PieceType::None;

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present() && p.color() == active_color) {
            if (is_piece_attacking(board, sq, target_sq)) {
                int val = get_see_piece_value(p.type());
                if (val < min_val) {
                    min_val = val;
                    attacker_sq = sq;
                    attacker_type = p.type();
                }
            }
        }
    }

    if (attacker_sq.is_none()) {
        return 0;
    }

    // Simulate the capture
    pieces::Piece captured_piece = board.piece_at(target_sq);
    pieces::Piece attacking_piece = board.piece_at(attacker_sq);

    // Make capture on our temporary board copy
    board.set_piece(attacker_sq, pieces::Piece());
    board.set_piece(target_sq, attacking_piece);

    // The material gain of this step is the value of the captured piece
    int next_gain = get_see_piece_value(captured_piece.type());

    // Opponent's turn to defend/attack. They will try to minimize our gain.
    int opponent_gain = see_evaluate(board, target_sq, pieces::opposite_color(active_color));

    // If opponent recaptures, our net gain is: next_gain - opponent_gain.
    // If next_gain - opponent_gain < 0, we choose not to make the capture in the first place, returning 0.
    return std::max(0, next_gain - opponent_gain);
}

int see(const board::Board& board, const moves::Move& move) {
    pieces::Square target_sq = move.to();
    pieces::Piece captured = board.piece_at(target_sq);
    
    if (!captured.is_present() && move.type() != moves::MoveType::EnPassant) {
        return 0; // Not a capture
    }

    board::Board sim_board = board;
    pieces::Color active_color = sim_board.active_color();
    pieces::Color opponent_color = pieces::opposite_color(active_color);

    // First capture is already determined by `move`
    pieces::Piece attacker = sim_board.piece_at(move.from());
    
    // Make first capture
    sim_board.set_piece(move.from(), pieces::Piece());
    if (move.type() == moves::MoveType::EnPassant) {
        // En Passant captures the pawn on the adjacent square
        int pawn_dir = (active_color == pieces::Color::White) ? 1 : -1;
        pieces::Square ep_pawn_sq(static_cast<uint8_t>(target_sq.file()), static_cast<uint8_t>(target_sq.rank() - pawn_dir));
        captured = board.piece_at(ep_pawn_sq);
        sim_board.set_piece(ep_pawn_sq, pieces::Piece());
    }
    
    pieces::Piece landing_piece = attacker;
    if (move.type() == moves::MoveType::Promotion) {
        landing_piece = pieces::Piece(move.promotion(), active_color);
    }
    sim_board.set_piece(target_sq, landing_piece);

    // Initial gain is the value of the captured piece (plus promotion value increase if any)
    int initial_gain = get_see_piece_value(captured.type());
    if (move.type() == moves::MoveType::Promotion) {
        initial_gain += (get_see_piece_value(move.promotion()) - get_see_piece_value(pieces::PieceType::Pawn));
    }

    int opponent_gain = see_evaluate(sim_board, target_sq, opponent_color);
    return initial_gain - opponent_gain;
}

} // namespace chess::search
