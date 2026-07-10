#ifndef CHESS_BOARD_STATE_HPP
#define CHESS_BOARD_STATE_HPP

#include "chess/pieces/color.hpp"
#include "chess/pieces/piece.hpp"
#include "chess/pieces/square.hpp"
#include <cstdint>

namespace chess::moves {

struct CastlingRights {
    static constexpr uint8_t kWhiteKingside  = 1 << 0;
    static constexpr uint8_t kWhiteQueenside = 1 << 1;
    static constexpr uint8_t kBlackKingside  = 1 << 2;
    static constexpr uint8_t kBlackQueenside = 1 << 3;
    static constexpr uint8_t kNone           = 0;
    static constexpr uint8_t kAll            = kWhiteKingside | kWhiteQueenside | kBlackKingside | kBlackQueenside;
};

struct BoardState {
    uint8_t castling_rights = CastlingRights::kAll;
    pieces::Square en_passant_square = pieces::Square::None();
    uint16_t halfmove_clock = 0;
    uint16_t fullmove_clock = 1;
    pieces::Piece captured_piece = pieces::Piece();
    pieces::Color active_color = pieces::Color::White;
    uint64_t zobrist_hash = 0;
};

} // namespace chess::moves

#endif // CHESS_BOARD_STATE_HPP
