#ifndef CHESS_MOVE_GENERATOR_HPP
#define CHESS_MOVE_GENERATOR_HPP

#include "chess/board/board.hpp"
#include "chess/moves/move.hpp"
#include <vector>

namespace chess::moves {

class MoveGenerator {
public:
    // Generates all legal moves for the active side on the board.
    static std::vector<Move> generate_legal_moves(board::Board& board);

    // Generates all pseudo-legal moves for the active side on the board.
    static std::vector<Move> generate_pseudo_legal_moves(const board::Board& board);

    // Determines if a square is attacked by any piece of the specified attacker color.
    static bool is_square_attacked(const board::Board& board, pieces::Square square, pieces::Color attacker_color);

    // Determines if the specified player's king is currently in check.
    static bool is_in_check(const board::Board& board, pieces::Color color);

    // Simple perft calculator (returns number of leaf nodes at depth)
    static uint64_t perft(board::Board& board, int depth);

private:
    static void generate_pawn_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves);
    static void generate_knight_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves);
    static void generate_bishop_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves);
    static void generate_rook_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves);
    static void generate_queen_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves);
    static void generate_king_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves);
    static void generate_castling_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves);
};

} // namespace chess::moves

#endif // CHESS_MOVE_GENERATOR_HPP
