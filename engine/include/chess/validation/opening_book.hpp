#ifndef CHESS_OPENING_BOOK_HPP
#define CHESS_OPENING_BOOK_HPP

#include "chess/board/board.hpp"
#include <string>
#include <vector>

namespace chess::validation {

struct BookEntry {
    uint64_t zobrist_key;
    moves::Move move;
    uint16_t weight;
    uint32_t learn;
};

class OpeningBook {
public:
    OpeningBook();

    // Loads a simple text-based opening book where each line is a sequence of moves (in LAN/coordinate format, e.g. "e2e4 e7e5 g1f3")
    bool load_text_book(const std::string& filepath);

    // Loads a binary Polyglot opening book (.bin file)
    bool load_polyglot_book(const std::string& filepath);

    // Returns a list of candidate moves matching the current board state
    std::vector<BookEntry> get_candidates(board::Board& board);

    // Selects a move from candidate moves (weighted selection or random)
    moves::Move select_move(board::Board& board, bool pick_best = false);

private:
    std::vector<BookEntry> entries_;
    std::vector<std::vector<std::string>> text_lines_;

    // Generates Polyglot-compatible Zobrist hash for the board (if different from our Zobrist)
    uint64_t get_polyglot_hash(const board::Board& board);
};

} // namespace chess::validation

#endif // CHESS_OPENING_BOOK_HPP
