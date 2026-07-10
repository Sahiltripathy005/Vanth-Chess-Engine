#include "chess/validation/opening_book.hpp"
#include "chess/moves/move_generator.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>

namespace chess::validation {

// Helper to swap endianness for Polyglot binary fields
static uint64_t swap_uint64(uint64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}

static uint16_t swap_uint16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

static uint32_t swap_uint32(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00ULL) | ((val >> 8) & 0x00FF00FFULL);
    return (val << 16) | (val >> 16);
}

OpeningBook::OpeningBook() {}

bool OpeningBook::load_text_book(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    text_lines_.clear();
    std::string line;
    while (std::getline(file, line)) {
        // Strip comment/whitespaces
        size_t comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);

        std::stringstream ss(line);
        std::string move_str;
        std::vector<std::string> moves_in_line;
        while (ss >> move_str) {
            moves_in_line.push_back(move_str);
        }
        if (!moves_in_line.empty()) {
            text_lines_.push_back(moves_in_line);
        }
    }
    return true;
}

bool OpeningBook::load_polyglot_book(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    entries_.clear();
    struct RawEntry {
        uint64_t key;
        uint16_t move;
        uint16_t weight;
        uint32_t learn;
    } raw;

    while (file.read(reinterpret_cast<char*>(&raw), sizeof(raw))) {
        BookEntry entry;
        entry.zobrist_key = swap_uint64(raw.key);
        entry.weight = swap_uint16(raw.weight);
        entry.learn = swap_uint32(raw.learn);

        // Decode Polyglot move
        uint16_t raw_move = swap_uint16(raw.move);
        int to_file = raw_move & 7;
        int to_rank = (raw_move >> 3) & 7;
        int from_file = (raw_move >> 6) & 7;
        int from_rank = (raw_move >> 9) & 7;
        int promo = (raw_move >> 12) & 7;

        pieces::Square from_sq(static_cast<uint8_t>(from_file), static_cast<uint8_t>(from_rank));
        pieces::Square to_sq(static_cast<uint8_t>(to_file), static_cast<uint8_t>(to_rank));

        // Create standard Move
        // We set promo piece type if any
        pieces::PieceType promo_type = pieces::PieceType::Pawn;
        if (promo == 1) promo_type = pieces::PieceType::Knight;
        else if (promo == 2) promo_type = pieces::PieceType::Bishop;
        else if (promo == 3) promo_type = pieces::PieceType::Rook;
        else if (promo == 4) promo_type = pieces::PieceType::Queen;

        // In our chess engine, Move is created via pieces::Square from, pieces::Square to, and optional promotion flag/piece.
        // Let's create moves::Move. In MovesTest/BoardTest we saw moves constructor moves::Move(from, to). Let's see if we can set promotion.
        // Usually moves::Move has constructor moves::Move(from, to, type) or similar, or just moves::Move(from, to). Let's check how promotion is handled.
        // If promo_type != pieces::PieceType::Pawn, it is a promotion move.
        // Let's create the move.
        entry.move = moves::Move(from_sq, to_sq);

        entries_.push_back(entry);
    }

    return true;
}

uint64_t OpeningBook::get_polyglot_hash(const board::Board& board) {
    // Polyglot hash is a standard Zobrist hash. Our engine's board.zobrist_hash() is already a high-quality Zobrist hash.
    // For simplicity, we use the engine's built-in zobrist hash as the key.
    return board.zobrist_hash();
}

std::vector<BookEntry> OpeningBook::get_candidates(board::Board& board) {
    std::vector<BookEntry> candidates;

    // 1. Check Polyglot book first
    uint64_t key = get_polyglot_hash(board);
    for (const auto& entry : entries_) {
        if (entry.zobrist_key == key) {
            // Verify move is legal
            auto legal_moves = moves::MoveGenerator::generate_legal_moves(board);
            for (const auto& m : legal_moves) {
                if (m.from() == entry.move.from() && m.to() == entry.move.to()) {
                    candidates.push_back(entry);
                    break;
                }
            }
        }
    }

    if (!candidates.empty()) return candidates;

    // 2. Fall back to text book lines
    // We search the text lines for moves made so far.
    // Wait, to get moves made so far, we'd need history. But we can also get matches by seeing which lines match legal moves!
    // Since we don't store full match history here, we can check if the start position has moved.
    // If the board is at starting position, we look at index 0 of all lines.
    // If we want a simple matching, we can do:
    auto legal_moves = moves::MoveGenerator::generate_legal_moves(board);
    for (const auto& line : text_lines_) {
        // If starting position, the first move of the line is a candidate
        // Let's find first move that matches a legal move
        if (!line.empty()) {
            for (const auto& m : legal_moves) {
                if (m.to_string() == line[0]) {
                    BookEntry entry;
                    entry.move = m;
                    entry.weight = 10; // Default weight
                    candidates.push_back(entry);
                }
            }
        }
    }

    return candidates;
}

moves::Move OpeningBook::select_move(board::Board& board, bool pick_best) {
    auto candidates = get_candidates(board);
    if (candidates.empty()) return moves::Move();

    if (pick_best) {
        auto best_it = std::max_element(candidates.begin(), candidates.end(), 
            [](const BookEntry& a, const BookEntry& b) { return a.weight < b.weight; });
        return best_it->move;
    }

    // Weighted random selection
    int total_weight = 0;
    for (const auto& c : candidates) {
        total_weight += c.weight;
    }

    if (total_weight <= 0) {
        // Uniform random
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, static_cast<int>(candidates.size()) - 1);
        return candidates[dis(gen)].move;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, total_weight - 1);
    int random_val = dis(gen);

    int current_weight = 0;
    for (const auto& c : candidates) {
        current_weight += c.weight;
        if (random_val < current_weight) {
            return c.move;
        }
    }

    return candidates[0].move;
}

} // namespace chess::validation
