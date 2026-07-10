#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include <vector>
#include <algorithm>

namespace chess::search {

int evaluate_rook_activity(const board::Board& board) {
    int score = 0;

    int white_pawns[8] = {0};
    int black_pawns[8] = {0};
    bool has_pawn[2][8][8] = {false};

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present() && p.type() == pieces::PieceType::Pawn) {
            if (p.color() == pieces::Color::White) {
                white_pawns[sq.file()]++;
                has_pawn[0][sq.file()][sq.rank()] = true;
            } else {
                black_pawns[sq.file()]++;
                has_pawn[1][sq.file()][sq.rank()] = true;
            }
        }
    }

    // Helper to check if a pawn on file, rank is passed
    auto is_passed_pawn = [&](int color_idx, int file, int rank) {
        int opp = 1 - color_idx;
        int pawn_dir = (color_idx == 0) ? 1 : -1;
        // Check same file ahead
        for (int r = rank + pawn_dir; r >= 0 && r < 8; r += pawn_dir) {
            if (has_pawn[opp][file][r]) return false;
        }
        // Check adjacent files ahead
        if (file > 0) {
            for (int r = rank + pawn_dir; r >= 0 && r < 8; r += pawn_dir) {
                if (has_pawn[opp][file - 1][r]) return false;
            }
        }
        if (file < 7) {
            for (int r = rank + pawn_dir; r >= 0 && r < 8; r += pawn_dir) {
                if (has_pawn[opp][file + 1][r]) return false;
            }
        }
        return true;
    };

    // Find all rooks
    std::vector<pieces::Square> white_rooks;
    std::vector<pieces::Square> black_rooks;

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present() && p.type() == pieces::PieceType::Rook) {
            if (p.color() == pieces::Color::White) {
                white_rooks.push_back(sq);
            } else {
                black_rooks.push_back(sq);
            }
        }
    }

    auto evaluate_rooks = [&](int color_idx, const std::vector<pieces::Square>& rooks) {
        int rook_score = 0;
        int own_pawns[8];
        int opp_pawns[8];
        if (color_idx == 0) {
            std::copy(std::begin(white_pawns), std::end(white_pawns), own_pawns);
            std::copy(std::begin(black_pawns), std::end(black_pawns), opp_pawns);
        } else {
            std::copy(std::begin(black_pawns), std::end(black_pawns), own_pawns);
            std::copy(std::begin(white_pawns), std::end(white_pawns), opp_pawns);
        }

        for (size_t i = 0; i < rooks.size(); ++i) {
            pieces::Square r_sq = rooks[i];
            int file = r_sq.file();
            int rank = r_sq.rank();

            // 1. Open / Semi-open files
            if (own_pawns[file] == 0 && opp_pawns[file] == 0) {
                rook_score += kRookWeights.open_file;
            } else if (own_pawns[file] == 0 && opp_pawns[file] > 0) {
                rook_score += kRookWeights.semi_open_file;
            }

            // 2. Seventh Rank
            int seventh_rank = (color_idx == 0) ? 6 : 1;
            if (rank == seventh_rank) {
                rook_score += kRookWeights.seventh_rank;
            }

            // 3. Connected rooks
            for (size_t j = i + 1; j < rooks.size(); ++j) {
                pieces::Square other_r = rooks[j];
                if (other_r.file() == file || other_r.rank() == rank) {
                    rook_score += kRookWeights.connected_rooks;
                }
            }

            // 4. Rook behind passed pawn
            // Check if there is a friendly passed pawn on this file, and the rook is behind it
            for (int r = 0; r < 8; ++r) {
                if (has_pawn[color_idx][file][r]) {
                    if (is_passed_pawn(color_idx, file, r)) {
                        bool is_behind = (color_idx == 0) ? (rank < r) : (rank > r);
                        if (is_behind) {
                            rook_score += kRookWeights.behind_passed_pawn;
                        }
                    }
                }
            }
        }
        return rook_score;
    };

    score += evaluate_rooks(0, white_rooks);
    score -= evaluate_rooks(1, black_rooks);

    return score;
}

} // namespace chess::search
