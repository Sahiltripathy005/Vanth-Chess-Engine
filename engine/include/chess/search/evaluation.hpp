#ifndef CHESS_EVALUATION_HPP
#define CHESS_EVALUATION_HPP

#include "chess/board/board.hpp"

namespace chess::search {

// Piece values as constants
constexpr int kPawnValue   = 100;
constexpr int kKnightValue = 320;
constexpr int kBishopValue = 330;
constexpr int kRookValue   = 500;
constexpr int kQueenValue  = 900;
constexpr int kKingValue   = 0;

int evaluate(const board::Board& board);

// Sub-evaluation modules for Milestone 4 and 6
int get_game_phase(const board::Board& board);
int evaluate_material(const board::Board& board);
int evaluate_piece_square_tables(const board::Board& board, int phase);
int evaluate_pawn_structure(const board::Board& board);
int evaluate_piece_activity(const board::Board& board, int phase);
int evaluate_king_safety(const board::Board& board, int phase);
int evaluate_space(const board::Board& board);
int evaluate_threats(const board::Board& board);
int evaluate_endgame(const board::Board& board, int phase);
int evaluate_tempo(const board::Board& board);

// Under-the-hood helpers
int evaluate_bishop_pair(const board::Board& board);
int evaluate_rook_activity(const board::Board& board);
int evaluate_mobility(const board::Board& board);

} // namespace chess::search

#endif // CHESS_EVALUATION_HPP
