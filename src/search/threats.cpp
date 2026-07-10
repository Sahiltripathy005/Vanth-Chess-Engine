#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>
#include <cmath>

namespace chess::search {

static int get_piece_value_threat(pieces::PieceType type) {
    switch (type) {
        case pieces::PieceType::Pawn:   return 100;
        case pieces::PieceType::Knight: return 320;
        case pieces::PieceType::Bishop: return 330;
        case pieces::PieceType::Rook:   return 500;
        case pieces::PieceType::Queen:  return 900;
        default: return 0;
    }
}

// Helper to check if a piece is defended by color C
static bool is_piece_defended(const board::Board& board, pieces::Square sq, pieces::Color color) {
    return moves::MoveGenerator::is_square_attacked(board, sq, color);
}

int evaluate_threats(const board::Board& board) {
    int white_score = 0;
    int black_score = 0;

    // We need list of attackers for each square to determine lower-value attacker
    // For simplicity, we can inspect each piece on the board
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_none()) continue;

        pieces::Color color = p.color();
        pieces::Color opp_color = pieces::opposite_color(color);
        int p_val = get_piece_value_threat(p.type());

        if (p_val == 0) continue; // King is handled by king safety

        // Check if attacked by opponent
        bool is_attacked = moves::MoveGenerator::is_square_attacked(board, sq, opp_color);

        if (is_attacked) {
            bool is_defended = is_piece_defended(board, sq, color);
            if (!is_defended) {
                // Hanging piece (undefended)
                if (color == pieces::Color::White) white_score -= p_val / 2;
                else black_score -= p_val / 2;
            } else {
                // Defended but check if attacked by a lower-value piece
                // Check all squares to see who is attacking
                bool lower_value_attacker = false;
                for (uint8_t a = 0; a < 64; ++a) {
                    pieces::Square a_sq(a);
                    pieces::Piece att = board.piece_at(a_sq);
                    if (att.is_present() && att.color() == opp_color) {
                        int att_val = get_piece_value_threat(att.type());
                        if (att_val > 0 && att_val < p_val) {
                            // Check if this specific piece attacks the square
                            // We can use pseudo-legal check for attacks
                            if (moves::MoveGenerator::is_square_attacked(board, sq, opp_color)) {
                                lower_value_attacker = true;
                                break;
                            }
                        }
                    }
                }
                if (lower_value_attacker) {
                    if (color == pieces::Color::White) white_score -= p_val / 3;
                    else black_score -= p_val / 3;
                }
            }
        }

        // 2. Trapped Pieces
        // Check mobility: generate moves and see how many options this piece has.
        // A piece is trapped if it has 0 or 1 destination square in enemy territory and is attacked.
        int rank = sq.rank();
        bool in_enemy_territory = (color == pieces::Color::White && rank >= 4) || 
                                  (color == pieces::Color::Black && rank <= 3);
        if (in_enemy_territory) {
            // Count attacks/moves
            int escape_squares = 0;
            // Generate legal moves and see if this piece has moves
            std::vector<moves::Move> legal_moves = moves::MoveGenerator::generate_pseudo_legal_moves(board);
            for (const auto& m : legal_moves) {
                if (m.from() == sq) {
                    // Check if target is not attacked by opponent pawn or lower piece
                    escape_squares++;
                }
            }
            if (escape_squares <= 1 && is_attacked) {
                if (color == pieces::Color::White) white_score -= 80;
                else black_score -= 80;
            }
        }
    }

    // 3. Pins, Skewers and Discovered Attacks
    // We check alignment of pieces along files/ranks/diagonals
    pieces::Square white_king_sq = pieces::Square::None();
    pieces::Square black_king_sq = pieces::Square::None();
    pieces::Square white_queen_sq = pieces::Square::None();
    pieces::Square black_queen_sq = pieces::Square::None();

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Piece p = board.piece_at(pieces::Square(i));
        if (p.is_present()) {
            if (p.type() == pieces::PieceType::King) {
                if (p.color() == pieces::Color::White) white_king_sq = pieces::Square(i);
                else black_king_sq = pieces::Square(i);
            } else if (p.type() == pieces::PieceType::Queen) {
                if (p.color() == pieces::Color::White) white_queen_sq = pieces::Square(i);
                else black_queen_sq = pieces::Square(i);
            }
        }
    }

    auto check_ray_alignment = [&](pieces::Square start, pieces::Square target, int step_f, int step_r) {
        int sf = start.file();
        int sr = start.rank();
        int tf = target.file();
        int tr = target.rank();

        int df = tf - sf;
        int dr = tr - sr;

        // Check if aligned
        if (step_f == 0 && df != 0) return false;
        if (step_r == 0 && dr != 0) return false;
        if (step_f != 0 && step_r != 0 && std::abs(df) != std::abs(dr)) return false;
        if (step_f != 0 && df * step_f < 0) return false;
        if (step_r != 0 && dr * step_r < 0) return false;

        return true;
    };

    // Scan for pins/discovered attacks
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square attacker_sq(i);
        pieces::Piece att = board.piece_at(attacker_sq);
        if (att.is_none()) continue;

        pieces::Color att_color = att.color();
        pieces::Color victim_color = pieces::opposite_color(att_color);

        pieces::Square target_king = (victim_color == pieces::Color::White) ? white_king_sq : black_king_sq;
        pieces::Square target_queen = (victim_color == pieces::Color::White) ? white_queen_sq : black_queen_sq;

        if (att.type() == pieces::PieceType::Bishop || att.type() == pieces::PieceType::Rook || att.type() == pieces::PieceType::Queen) {
            int steps[8][2] = { {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
            for (int s = 0; s < 8; ++s) {
                int df = steps[s][0];
                int dr = steps[s][1];

                // Check bishop/rook directions
                if (att.type() == pieces::PieceType::Bishop && (df == 0 || dr == 0)) continue;
                if (att.type() == pieces::PieceType::Rook && (df != 0 && dr != 0)) continue;

                // Check alignment to King
                if (!target_king.is_none() && check_ray_alignment(attacker_sq, target_king, df, dr)) {
                    // Count pieces in between
                    int f = attacker_sq.file() + df;
                    int r = attacker_sq.rank() + dr;
                    int pieces_between = 0;
                    pieces::Square between_sq = pieces::Square::None();

                    while (f != target_king.file() || r != target_king.rank()) {
                        pieces::Square sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
                        if (board.piece_at(sq).is_present()) {
                            pieces_between++;
                            between_sq = sq;
                        }
                        f += df;
                        r += dr;
                    }

                    if (pieces_between == 1) {
                        pieces::Piece p_between = board.piece_at(between_sq);
                        if (p_between.color() == victim_color) {
                            // PIN: enemy piece is pinned to their king
                            if (att_color == pieces::Color::White) white_score += 25;
                            else black_score += 25;
                        } else if (p_between.color() == att_color) {
                            // DISCOVERED ATTACK setup: friendly piece blocks attack on enemy king
                            if (att_color == pieces::Color::White) white_score += 20;
                            else black_score += 20;
                        }
                    }
                }

                // Check alignment to Queen
                if (!target_queen.is_none() && check_ray_alignment(attacker_sq, target_queen, df, dr)) {
                    int f = attacker_sq.file() + df;
                    int r = attacker_sq.rank() + dr;
                    int pieces_between = 0;
                    pieces::Square between_sq = pieces::Square::None();

                    while (f != target_queen.file() || r != target_queen.rank()) {
                        pieces::Square sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
                        if (board.piece_at(sq).is_present()) {
                            pieces_between++;
                            between_sq = sq;
                        }
                        f += df;
                        r += dr;
                    }

                    if (pieces_between == 1) {
                        pieces::Piece p_between = board.piece_at(between_sq);
                        if (p_between.color() == victim_color) {
                            // Pin to Queen
                            if (att_color == pieces::Color::White) white_score += 15;
                            else black_score += 15;
                        }
                    }
                }
            }
        }
    }

    return white_score - black_score;
}

} // namespace chess::search
