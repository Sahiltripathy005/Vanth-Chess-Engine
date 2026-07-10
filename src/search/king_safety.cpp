#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace chess::search {

static int get_tropism_weight(pieces::PieceType type) {
    switch (type) {
        case pieces::PieceType::Knight: return 2;
        case pieces::PieceType::Bishop: return 2;
        case pieces::PieceType::Rook:   return 3;
        case pieces::PieceType::Queen:  return 4;
        default: return 0;
    }
}

int evaluate_king_safety(const board::Board& board, int phase) {
    pieces::Square king_sq[2] = {pieces::Square::None(), pieces::Square::None()};

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.type() == pieces::PieceType::King) {
            int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
            king_sq[c_idx] = sq;
        }
    }

    auto evaluate_color_king_safety = [&](pieces::Color color, pieces::Square k_sq) {
        if (k_sq.is_none()) return 0;

        int safety_score = 0;
        int kf = k_sq.file();
        int kr = k_sq.rank();
        pieces::Color opp_color = pieces::opposite_color(color);
        int pawn_dir = (color == pieces::Color::White) ? 1 : -1;

        // 1. Castled King Bonus
        if (color == pieces::Color::White) {
            if (k_sq.index() == 6 || k_sq.index() == 2) {
                safety_score += kKingSafetyWeights.castled_king_bonus;
            }
        } else {
            if (k_sq.index() == 62 || k_sq.index() == 58) {
                safety_score += kKingSafetyWeights.castled_king_bonus;
            }
        }

        // 2. Pawn Shield Quality
        // Check 1st row in front of King
        int row_1 = kr + pawn_dir;
        if (row_1 >= 0 && row_1 < 8) {
            for (int f = kf - 1; f <= kf + 1; ++f) {
                if (f >= 0 && f < 8) {
                    pieces::Square p_sq(static_cast<uint8_t>(f), static_cast<uint8_t>(row_1));
                    pieces::Piece p = board.piece_at(p_sq);
                    if (p.type() == pieces::PieceType::Pawn && p.color() == color) {
                        safety_score += kKingSafetyWeights.shield_pawn_1st_row;
                    }
                }
            }
        }

        // Check 2nd row in front of King
        int row_2 = kr + 2 * pawn_dir;
        if (row_2 >= 0 && row_2 < 8) {
            for (int f = kf - 1; f <= kf + 1; ++f) {
                if (f >= 0 && f < 8) {
                    pieces::Square p_sq(static_cast<uint8_t>(f), static_cast<uint8_t>(row_2));
                    pieces::Piece p = board.piece_at(p_sq);
                    if (p.type() == pieces::PieceType::Pawn && p.color() == color) {
                        safety_score += kKingSafetyWeights.shield_pawn_2nd_row;
                    }
                }
            }
        }

        // 3. Open/Semi-open Files near King
        for (int f = kf - 1; f <= kf + 1; ++f) {
            if (f >= 0 && f < 8) {
                int friendly_pawns = 0;
                int enemy_pawns = 0;
                for (int r = 0; r < 8; ++r) {
                    pieces::Square p_sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
                    pieces::Piece p = board.piece_at(p_sq);
                    if (p.type() == pieces::PieceType::Pawn) {
                        if (p.color() == color) friendly_pawns++;
                        else enemy_pawns++;
                    }
                }
                if (friendly_pawns == 0 && enemy_pawns == 0) {
                    safety_score += kKingSafetyWeights.open_file_near_king;
                } else if (friendly_pawns == 0 && enemy_pawns > 0) {
                    safety_score += kKingSafetyWeights.semi_open_file_near_king;
                }
            }
        }

        // 4. Safe Squares and Attack Maps
        // Safe squares around King (3x3 grid)
        int safe_squares = 0;
        int enemy_attackers = 0;
        int attack_weight_sum = 0;

        for (int df = -1; df <= 1; ++df) {
            for (int dr = -1; dr <= 1; ++dr) {
                if (df == 0 && dr == 0) continue;
                int nf = kf + df;
                int nr = kr + dr;
                if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
                    pieces::Square adj_sq(static_cast<uint8_t>(nf), static_cast<uint8_t>(nr));
                    bool is_att = moves::MoveGenerator::is_square_attacked(board, adj_sq, opp_color);
                    if (is_att) {
                        safety_score += kKingSafetyWeights.king_exposure_penalty;
                    } else {
                        safe_squares++;
                    }
                }
            }
        }
        safety_score += safe_squares * 4; // Bonus for safe squares

        // 5. King Tropism (distance of enemy pieces to King)
        // 6. Enemy Piece Pressure
        for (uint8_t i = 0; i < 64; ++i) {
            pieces::Square sq(i);
            pieces::Piece p = board.piece_at(sq);
            if (p.is_present() && p.color() == opp_color) {
                int trop_w = get_tropism_weight(p.type());
                if (trop_w > 0) {
                    int dist = std::abs(sq.file() - kf) + std::abs(sq.rank() - kr);
                    safety_score -= (14 - dist) * trop_w; // Closer = worse safety

                    // Check if piece attacks king's 3x3 zone
                    // We can check if it attacks any square in the king's zone
                    bool attacks_zone = false;
                    for (int df = -1; df <= 1; ++df) {
                        for (int dr = -1; dr <= 1; ++dr) {
                            int nf = kf + df;
                            int nr = kr + dr;
                            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
                                pieces::Square zone_sq(static_cast<uint8_t>(nf), static_cast<uint8_t>(nr));
                                // Check if this piece specifically attacks zone_sq
                                // Simple distance approximation for speed
                                if (moves::MoveGenerator::is_square_attacked(board, zone_sq, opp_color)) {
                                    attacks_zone = true;
                                    break;
                                }
                            }
                        }
                        if (attacks_zone) break;
                    }

                    if (attacks_zone) {
                        enemy_attackers++;
                        attack_weight_sum += trop_w * 10;
                    }
                }
            }
        }

        // Scale pressure penalty by number of attackers
        // 1 attacker: 0, 2: -10, 3: -30, 4: -70, 5+: -150
        int pressure_penalty = 0;
        if (enemy_attackers == 2) pressure_penalty = -15;
        else if (enemy_attackers == 3) pressure_penalty = -40;
        else if (enemy_attackers == 4) pressure_penalty = -80;
        else if (enemy_attackers >= 5) pressure_penalty = -160;

        safety_score += pressure_penalty;
        safety_score -= attack_weight_sum;

        // 7. Open Diagonals near King
        // Check if diagonals emanating from King are open
        int diag_steps[4][2] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };
        for (int d = 0; d < 4; ++d) {
            int df = diag_steps[d][0];
            int dr = diag_steps[d][1];
            int f = kf + df;
            int r = kr + dr;
            int open_squares = 0;
            while (f >= 0 && f < 8 && r >= 0 && r < 8) {
                pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(f), static_cast<uint8_t>(r)));
                if (p.is_none()) {
                    open_squares++;
                } else {
                    break;
                }
                f += df;
                r += dr;
            }
            if (open_squares >= 3) {
                safety_score -= 10; // Open diagonal penalty
            }
        }

        // 8. Mating Threats
        // If King is in check and has <= 1 escape square
        if (moves::MoveGenerator::is_in_check(board, color)) {
            int escape_squares = 0;
            // Generate legal moves and see if King has escape options
            std::vector<moves::Move> legal_moves = moves::MoveGenerator::generate_pseudo_legal_moves(board);
            for (const auto& m : legal_moves) {
                if (board.piece_at(m.from()).type() == pieces::PieceType::King) {
                    escape_squares++;
                }
            }
            if (escape_squares <= 1) {
                safety_score -= 100; // Dangerous mating threat!
            }
        }

        // 9. Endgame Activity
        if (phase < 128) {
            if (kf >= 2 && kf <= 5 && kr >= 2 && kr <= 5) {
                safety_score += (kKingSafetyWeights.king_endgame_activity * (128 - phase)) / 128;
            }
        }

        return safety_score;
    };

    int white_safety = evaluate_color_king_safety(pieces::Color::White, king_sq[0]);
    int black_safety = evaluate_color_king_safety(pieces::Color::Black, king_sq[1]);

    return ((white_safety - black_safety) * phase) / 256;
}

} // namespace chess::search
