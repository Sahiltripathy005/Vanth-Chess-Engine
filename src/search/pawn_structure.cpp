#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>
#include <cmath>

namespace chess::search {

int evaluate_pawn_structure(const board::Board& board) {
    int score = 0;

    // Populates a lookup table for pawn positions
    // index 0: White, index 1: Black
    bool has_pawn[2][8][8] = {false};
    int pawns_per_file[2][8] = {0};
    
    // Find kings for passed pawn races and pawn storms
    pieces::Square king_sq[2] = {pieces::Square::None(), pieces::Square::None()};
    bool has_non_king_pieces[2] = {false, false};

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present()) {
            if (p.type() == pieces::PieceType::Pawn) {
                int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                has_pawn[c_idx][sq.file()][sq.rank()] = true;
                pawns_per_file[c_idx][sq.file()]++;
            } else if (p.type() == pieces::PieceType::King) {
                int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                king_sq[c_idx] = sq;
            } else {
                int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                has_non_king_pieces[c_idx] = true;
            }
        }
    }

    // Traverse colors
    for (int c = 0; c < 2; ++c) {
        int opp = 1 - c;
        int pawn_dir = (c == 0) ? 1 : -1;
        int starting_rank = (c == 0) ? 1 : 6;
        int promotion_rank = (c == 0) ? 7 : 0;
        pieces::Color color = (c == 0) ? pieces::Color::White : pieces::Color::Black;

        int color_score = 0;

        // 1. Pawn Islands
        int islands = 0;
        bool in_island = false;
        for (int file = 0; file < 8; ++file) {
            if (pawns_per_file[c][file] > 0) {
                if (!in_island) {
                    islands++;
                    in_island = true;
                }
            } else {
                in_island = false;
            }
        }
        if (islands > 1) {
            color_score += (islands - 1) * -12; // Island penalty
        }

        // 2. Pawn Majorities (Queenside: 0-2, Center: 3-4, Kingside: 5-7)
        int own_queenside = pawns_per_file[c][0] + pawns_per_file[c][1] + pawns_per_file[c][2];
        int opp_queenside = pawns_per_file[opp][0] + pawns_per_file[opp][1] + pawns_per_file[opp][2];
        if (own_queenside > opp_queenside) color_score += 8;

        int own_center = pawns_per_file[c][3] + pawns_per_file[c][4];
        int opp_center = pawns_per_file[opp][3] + pawns_per_file[opp][4];
        if (own_center > opp_center) color_score += 10;

        int own_kingside = pawns_per_file[c][5] + pawns_per_file[c][6] + pawns_per_file[c][7];
        int opp_kingside = pawns_per_file[opp][5] + pawns_per_file[opp][6] + pawns_per_file[opp][7];
        if (own_kingside > opp_kingside) color_score += 8;

        // 3. Pawn Storms
        // If opponent's king is castled (file <= 2 or file >= 5) and we push adjacent pawns
        if (!king_sq[opp].is_none()) {
            int opp_king_file = king_sq[opp].file();
            int opp_king_rank = king_sq[opp].rank();
            bool opp_king_castled_kingside = (opp_king_file >= 5 && ((opp == 1 && opp_king_rank >= 6) || (opp == 0 && opp_king_rank <= 1)));
            bool opp_king_castled_queenside = (opp_king_file <= 2 && ((opp == 1 && opp_king_rank >= 6) || (opp == 0 && opp_king_rank <= 1)));

            if (opp_king_castled_kingside) {
                // Count advanced pawns on f, g, h files (files 5, 6, 7)
                for (int f = 5; f <= 7; ++f) {
                    for (int r = 0; r < 8; ++r) {
                        if (has_pawn[c][f][r]) {
                            int advancement = (c == 0) ? r - 1 : 6 - r;
                            if (advancement >= 3) {
                                color_score += (advancement - 2) * 8; // Storm bonus
                            }
                        }
                    }
                }
            } else if (opp_king_castled_queenside) {
                // Count advanced pawns on a, b, c files (files 0, 1, 2)
                for (int f = 0; f <= 2; ++f) {
                    for (int r = 0; r < 8; ++r) {
                        if (has_pawn[c][f][r]) {
                            int advancement = (c == 0) ? r - 1 : 6 - r;
                            if (advancement >= 3) {
                                color_score += (advancement - 2) * 8;
                            }
                        }
                    }
                }
            }
        }

        // Loop over squares for individual pawn features
        for (int file = 0; file < 8; ++file) {
            // Count doubled pawns
            if (pawns_per_file[c][file] > 1) {
                color_score += (pawns_per_file[c][file] - 1) * kPawnWeights.doubled_pawn;
            }

            for (int rank = 0; rank < 8; ++rank) {
                if (!has_pawn[c][file][rank]) continue;

                // 4. Isolated pawn check
                bool has_neighbors = false;
                if (file > 0 && pawns_per_file[c][file - 1] > 0) has_neighbors = true;
                if (file < 7 && pawns_per_file[c][file + 1] > 0) has_neighbors = true;
                if (!has_neighbors) {
                    color_score += kPawnWeights.isolated_pawn;
                }

                // 5. Passed pawn check
                bool is_passed = true;
                for (int r = rank + pawn_dir; r >= 0 && r < 8; r += pawn_dir) {
                    if (has_pawn[opp][file][r]) { is_passed = false; break; }
                }
                if (is_passed && file > 0) {
                    for (int r = rank + pawn_dir; r >= 0 && r < 8; r += pawn_dir) {
                        if (has_pawn[opp][file - 1][r]) { is_passed = false; break; }
                    }
                }
                if (is_passed && file < 7) {
                    for (int r = rank + pawn_dir; r >= 0 && r < 8; r += pawn_dir) {
                        if (has_pawn[opp][file + 1][r]) { is_passed = false; break; }
                    }
                }

                if (is_passed) {
                    color_score += kPawnWeights.passed_pawn;
                    int relative_rank = (c == 0) ? rank : 7 - rank;
                    color_score += kPawnWeights.passed_pawn_by_rank[relative_rank];

                    // --- Passed Pawn Races (Rule of the Square) ---
                    // Applicable in endgames (opponent has no non-king pieces)
                    if (!has_non_king_pieces[opp] && !king_sq[opp].is_none()) {
                        int steps = (c == 0) ? (7 - rank) : rank;
                        int opp_king_file = king_sq[opp].file();
                        int opp_king_rank = king_sq[opp].rank();
                        int king_dist = std::max(std::abs(opp_king_file - file), std::abs(opp_king_rank - promotion_rank));

                        int pawn_moves = (rank == starting_rank) ? (steps - 1) : steps;
                        int active_turn = (board.active_color() == color) ? 0 : 1; // 0 if it's pawn's side's turn

                        bool king_escapes = false;
                        if (active_turn == 0) {
                            king_escapes = (pawn_moves >= king_dist);
                        } else {
                            king_escapes = (pawn_moves + 1 >= king_dist);
                        }

                        if (!king_escapes) {
                            color_score += 250; // Massive bonus for runaway passed pawn
                        }
                    }

                    // --- Outside Passed Pawn ---
                    if (file < 2 || file > 5) {
                        color_score += 25;
                    }

                    // --- Protected Passed Pawn ---
                    bool is_protected = false;
                    if (file > 0 && has_pawn[c][file - 1][rank - pawn_dir]) is_protected = true;
                    if (file < 7 && has_pawn[c][file + 1][rank - pawn_dir]) is_protected = true;
                    if (is_protected) {
                        color_score += 20;
                    }

                    // --- Connected Passed Pawns ---
                    bool is_connected_passed = false;
                    if (file > 0 && has_pawn[c][file - 1][rank]) is_connected_passed = true;
                    if (file < 7 && has_pawn[c][file + 1][rank]) is_connected_passed = true;
                    if (is_connected_passed) {
                        color_score += 30;
                    }

                } else {
                    // 6. Candidate passed pawn
                    bool candidate = true;
                    for (int r = rank + pawn_dir; r >= 0 && r < 8; r += pawn_dir) {
                        if (has_pawn[opp][file][r]) { candidate = false; break; }
                    }
                    if (candidate) {
                        color_score += kPawnWeights.candidate_passed;
                    }
                }

                // 7. Pawn Chain
                bool is_chained = false;
                if (file > 0 && has_pawn[c][file - 1][rank - pawn_dir]) is_chained = true;
                if (file < 7 && has_pawn[c][file + 1][rank - pawn_dir]) is_chained = true;
                if (is_chained) {
                    color_score += kPawnWeights.pawn_chain;
                }

                // 8. Backward pawn check
                bool has_support_behind = false;
                if (file > 0) {
                    for (int r = rank; r >= 0 && r < 8; r -= pawn_dir) {
                        if (has_pawn[c][file - 1][r]) { has_support_behind = true; break; }
                    }
                }
                if (!has_support_behind && file < 7) {
                    for (int r = rank; r >= 0 && r < 8; r -= pawn_dir) {
                        if (has_pawn[c][file + 1][r]) { has_support_behind = true; break; }
                    }
                }
                if (!has_support_behind) {
                    int front_rank = rank + pawn_dir;
                    if (front_rank >= 0 && front_rank < 8) {
                        bool front_attacked = false;
                        if (file > 0 && has_pawn[opp][file - 1][front_rank]) front_attacked = true;
                        if (file < 7 && has_pawn[opp][file + 1][front_rank]) front_attacked = true;
                        if (front_attacked) {
                            color_score += kPawnWeights.backward_pawn;
                        }
                    }
                }

                // 9. Blocked pawn
                int front_rank = rank + pawn_dir;
                if (front_rank >= 0 && front_rank < 8) {
                    pieces::Square front_sq(static_cast<uint8_t>(file), static_cast<uint8_t>(front_rank));
                    if (board.piece_at(front_sq).is_present()) {
                        color_score += kPawnWeights.blocked_pawn;
                    }
                }

                // 10. Central pawn
                if ((file == 3 || file == 4) && (rank == 3 || rank == 4)) {
                    color_score += kPawnWeights.central_pawn;
                }

                // 11. Pawn Levers
                // Diagonally attacking an opponent pawn
                if (front_rank >= 0 && front_rank < 8) {
                    if (file > 0 && has_pawn[opp][file - 1][front_rank]) {
                        color_score += 5; // Lever tension bonus
                    }
                    if (file < 7 && has_pawn[opp][file + 1][front_rank]) {
                        color_score += 5;
                    }
                }
            }
        }

        if (c == 0) score += color_score;
        else score -= color_score;
    }

    return score;
}

} // namespace chess::search
