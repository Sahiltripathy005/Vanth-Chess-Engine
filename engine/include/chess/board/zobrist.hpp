#ifndef CHESS_ZOBRIST_HPP
#define CHESS_ZOBRIST_HPP

#include "chess/pieces/piece.hpp"
#include "chess/pieces/square.hpp"
#include "chess/pieces/color.hpp"
#include "chess/pieces/piece_type.hpp"
#include <cstdint>

namespace chess::board {

class Zobrist {
public:
    // Initialize Zobrist keys (thread-safe, lazy initialization on first use)
    static void init();

    // Key lookup functions
    static uint64_t piece_key(pieces::Color color, pieces::PieceType type, pieces::Square sq);
    static uint64_t side_key();
    static uint64_t castling_key(uint8_t castling_rights);
    static uint64_t en_passant_key(pieces::Square ep_square);

private:
    static uint64_t piece_keys[2][6][64];
    static uint64_t side_to_move_key;
    static uint64_t castling_keys[16];
    static uint64_t en_passant_keys[8];
    static bool initialized;
};

} // namespace chess::board

#endif // CHESS_ZOBRIST_HPP
