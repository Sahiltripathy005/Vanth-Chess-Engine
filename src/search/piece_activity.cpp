#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>
#include <cmath>

namespace chess::search {



// Check if square is an outpost for a knight of color C
static bool is_knight_outpost(const board::Board& board, pieces::Square sq, pieces::Color color) {
    int rank = sq.rank();
    int file = sq.file();
    
    // Outpost ranks: 4, 5, 6 for White (indices 3, 4, 5); 3, 2, 1 for Black (indices 4, 3, 2)
    bool correct_rank = (color == pieces::Color::White) ? (rank >= 3 && rank <= 5) : (rank >= 2 && rank <= 4);
    if (!correct_rank) return false;

    // Check if it can be attacked by enemy pawns.
    // White knight on outpost: Black pawns at file-1, file+1 at rank >= rank must not exist or be able to reach/attack it.
    pieces::Color opp_color = pieces::opposite_color(color);
    int pawn_dir = (color == pieces::Color::White) ? 1 : -1;
    
    // Check if enemy pawns are on adjacent files at or behind the knight's rank (from enemy perspective)
    // For White knight: check if Black pawns are on adjacent files at rank >= knight_rank.
    // If none are present, the square is an outpost!
    bool can_be_attacked = false;
    for (int r = 0; r < 8; ++r) {
        // If there is an enemy pawn on adjacent files that could attack
        if ((color == pieces::Color::White && r >= rank) || (color == pieces::Color::Black && r <= rank)) {
            if (file > 0) {
                pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(file - 1), static_cast<uint8_t>(r)));
                if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == opp_color) {
                    can_be_attacked = true;
                    break;
                }
            }
            if (file < 7) {
                pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(file + 1), static_cast<uint8_t>(r)));
                if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == opp_color) {
                    can_be_attacked = true;
                    break;
                }
            }
        }
    }

    if (can_be_attacked) return false;

    // Check if defended by friendly pawn
    // White knight: pawn at file-1, rank-1 or file+1, rank-1
    int friendly_pawn_rank = rank - pawn_dir;
    if (friendly_pawn_rank >= 0 && friendly_pawn_rank < 8) {
        if (file > 0) {
            pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(file - 1), static_cast<uint8_t>(friendly_pawn_rank)));
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == color) return true;
        }
        if (file < 7) {
            pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(file + 1), static_cast<uint8_t>(friendly_pawn_rank)));
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == color) return true;
        }
    }

    return false;
}

// Check if bishop is trapped
static bool is_bishop_trapped(const board::Board& board, pieces::Square sq, pieces::Color color) {
    int file = sq.file();
    int rank = sq.rank();
    pieces::Color opp_color = pieces::opposite_color(color);

    // Common trapping pattern: Bishop on a7/h7 trapped by pawn on b6/g6 (for White)
    if (color == pieces::Color::White) {
        if (file == 0 && rank == 6) { // a7
            pieces::Piece p = board.piece_at(pieces::Square(1, 5)); // b6
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == opp_color) return true;
        }
        if (file == 7 && rank == 6) { // h7
            pieces::Piece p = board.piece_at(pieces::Square(6, 5)); // g6
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == opp_color) return true;
        }
    } else {
        if (file == 0 && rank == 1) { // a2
            pieces::Piece p = board.piece_at(pieces::Square(1, 2)); // b3
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == opp_color) return true;
        }
        if (file == 7 && rank == 1) { // h2
            pieces::Piece p = board.piece_at(pieces::Square(6, 2)); // g3
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == opp_color) return true;
        }
    }
    return false;
}

int evaluate_piece_activity(const board::Board& board, int phase) {
    int white_score = 0;
    int black_score = 0;

    // 1. Mobility scoring
    // Call similar logic as old mobility
    auto add_mobility = [&](const board::Board& b, pieces::Color color) {
        auto moves = moves::MoveGenerator::generate_pseudo_legal_moves(b);
        int total_mobility = 0;
        for (const auto& move : moves) {
            pieces::Piece p = b.piece_at(move.from());
            if (p.is_present() && p.color() == color) {
                int val = 0;
                switch (p.type()) {
                    case pieces::PieceType::Knight: val = kMobilityWeights.knight_per_square; break;
                    case pieces::PieceType::Bishop: val = kMobilityWeights.bishop_per_square; break;
                    case pieces::PieceType::Rook:   val = kMobilityWeights.rook_per_square; break;
                    case pieces::PieceType::Queen:  val = kMobilityWeights.queen_per_square; break;
                    default: break;
                }
                total_mobility += val;
            }
        }
        return total_mobility;
    };

    white_score += add_mobility(board, pieces::Color::White);
    
    board::Board temp_board = board;
    temp_board.set_active_color(pieces::opposite_color(board.active_color()));
    black_score += add_mobility(temp_board, pieces::Color::Black);

    // 2. Bishop Pair
    int bp_score = evaluate_bishop_pair(board);
    white_score += (bp_score > 0) ? bp_score : 0;
    black_score += (bp_score < 0) ? -bp_score : 0;

    // 3. Rook Activity (reuse/call evaluate_rook_activity)
    // Note: evaluate_rook_activity is defined in rook_activity.cpp, we declare it or implement it.
    // Let's implement/reuse the rook activity since it's already declared in evaluation.hpp.
    // Wait, evaluation.hpp has evaluate_rook_activity(const board::Board& board).
    // Let's call it!
    int ra_score = evaluate_rook_activity(board);
    white_score += (ra_score > 0) ? ra_score : 0;
    black_score += (ra_score < 0) ? -ra_score : 0;

    // 4. Knights
    // Outposts, Centralization, Rim penalties, Support squares
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present() && p.type() == pieces::PieceType::Knight) {
            int knight_bonus = 0;
            
            // Outpost
            if (is_knight_outpost(board, sq, p.color())) {
                knight_bonus += 35;
            }

            // Centralization (closeness to center)
            int file = sq.file();
            int rank = sq.rank();
            int center_dist = std::abs(file - 3) + std::abs(rank - 3);
            knight_bonus += (6 - center_dist) * 3;

            // Rim penalty
            if (file == 0 || file == 7) {
                knight_bonus -= 15;
            }

            // Support squares (defended by pawn)
            int pawn_dir = (p.color() == pieces::Color::White) ? 1 : -1;
            int def_rank = rank - pawn_dir;
            if (def_rank >= 0 && def_rank < 8) {
                if (file > 0) {
                    pieces::Piece def_p = board.piece_at(pieces::Square(static_cast<uint8_t>(file - 1), static_cast<uint8_t>(def_rank)));
                    if (def_p.is_present() && def_p.type() == pieces::PieceType::Pawn && def_p.color() == p.color()) {
                        knight_bonus += 10;
                    }
                }
                if (file < 7) {
                    pieces::Piece def_p = board.piece_at(pieces::Square(static_cast<uint8_t>(file + 1), static_cast<uint8_t>(def_rank)));
                    if (def_p.is_present() && def_p.type() == pieces::PieceType::Pawn && def_p.color() == p.color()) {
                        knight_bonus += 10;
                    }
                }
            }

            if (p.color() == pieces::Color::White) white_score += knight_bonus;
            else black_score += knight_bonus;
        }

        // 5. Bishops
        // Good/Bad bishop, Long diagonals, Trapped bishops
        if (p.is_present() && p.type() == pieces::PieceType::Bishop) {
            int bishop_bonus = 0;

            // Good/Bad bishop: count friendly pawns on the same color square as the bishop
            bool bishop_square_color = ((sq.file() + sq.rank()) % 2 == 0); // true for dark, false for light
            int bad_pawns = 0;
            for (uint8_t j = 0; j < 64; ++j) {
                pieces::Piece pawn = board.piece_at(pieces::Square(j));
                if (pawn.is_present() && pawn.type() == pieces::PieceType::Pawn && pawn.color() == p.color()) {
                    bool pawn_square_color = (((j % 8) + (j / 8)) % 2 == 0);
                    if (pawn_square_color == bishop_square_color) {
                        bad_pawns++;
                    }
                }
            }
            bishop_bonus -= bad_pawns * 2; // Penalty for pawns on same color squares

            // Long diagonals: check if bishop is on a1-h8 or h1-a8 diagonals or adjacent ones
            int diag_length = 8 - std::abs(sq.file() - sq.rank());
            int anti_diag_length = 8 - std::abs(sq.file() - (7 - sq.rank()));
            int max_diag = std::max(diag_length, anti_diag_length);
            if (max_diag >= 6) {
                bishop_bonus += 10;
            }

            // Trapped bishop
            if (is_bishop_trapped(board, sq, p.color())) {
                bishop_bonus -= 120;
            }

            if (p.color() == pieces::Color::White) white_score += bishop_bonus;
            else black_score += bishop_bonus;
        }

        // 6. Queens
        // Early development penalty, Activity, Coordination
        if (p.is_present() && p.type() == pieces::PieceType::Queen) {
            int queen_bonus = 0;

            // Early development penalty (middlegame/opening)
            if (phase > 180) {
                // If Queen has moved but minor pieces are still at home
                int unmoved_minors = 0;
                if (p.color() == pieces::Color::White) {
                    if (sq.index() != 3) { // d1 is Queen starting square
                        uint8_t white_minors[] = {1, 2, 5, 6};
                        for (uint8_t idx : white_minors) {
                            pieces::Piece m = board.piece_at(pieces::Square(idx));
                            if (m.is_present() && m.color() == pieces::Color::White) unmoved_minors++;
                        }
                    }
                } else {
                    if (sq.index() != 59) { // d8 is Queen starting square
                        uint8_t black_minors[] = {57, 58, 61, 62};
                        for (uint8_t idx : black_minors) {
                            pieces::Piece m = board.piece_at(pieces::Square(idx));
                            if (m.is_present() && m.color() == pieces::Color::Black) unmoved_minors++;
                        }
                    }
                }
                queen_bonus -= unmoved_minors * 15;
            }

            if (p.color() == pieces::Color::White) white_score += queen_bonus;
            else black_score += queen_bonus;
        }
    }

    return white_score - black_score;
}

} // namespace chess::search
