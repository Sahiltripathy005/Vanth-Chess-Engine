#ifndef CHESS_BOARD_HPP
#define CHESS_BOARD_HPP

#include "chess/pieces/piece.hpp"
#include "chess/pieces/square.hpp"
#include "chess/moves/move.hpp"
#include "chess/moves/board_state.hpp"
#include <array>
#include <string>
#include <string_view>
#include <ostream>

namespace chess::board {

class Board {
public:
    Board(); // Initializes an empty board

    static Board start_position();

    // Board state accessors
    pieces::Piece piece_at(pieces::Square square) const;
    void set_piece(pieces::Square square, pieces::Piece piece);

    pieces::Color active_color() const { return state_.active_color; }
    void set_active_color(pieces::Color color) { state_.active_color = color; }

    uint8_t castling_rights() const { return state_.castling_rights; }
    void set_castling_rights(uint8_t rights) { state_.castling_rights = rights; }

    pieces::Square en_passant_square() const { return state_.en_passant_square; }
    void set_en_passant_square(pieces::Square square) { state_.en_passant_square = square; }

    uint16_t halfmove_clock() const { return state_.halfmove_clock; }
    void set_halfmove_clock(uint16_t clock) { state_.halfmove_clock = clock; }

    uint16_t fullmove_clock() const { return state_.fullmove_clock; }
    void set_fullmove_clock(uint16_t clock) { state_.fullmove_clock = clock; }

    const moves::BoardState& state() const { return state_; }

    uint64_t zobrist_hash() const { return state_.zobrist_hash; }
    uint64_t compute_zobrist_hash() const;

    // Make and Undo moves
    moves::BoardState make_move(const moves::Move& move);
    void undo_move(const moves::Move& move, const moves::BoardState& previous_state);

    // FEN support
    bool load_fen(std::string_view fen);
    std::string to_fen() const;

    // Printing
    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& os, const Board& board);

private:
    std::array<pieces::Piece, 64> pieces_;
    moves::BoardState state_;

    void update_castling_rights_for_square(pieces::Square square);
};

} // namespace chess::board

#endif // CHESS_BOARD_HPP
