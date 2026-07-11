#ifndef CHESS_TRANSPOSITION_TABLE_HPP
#define CHESS_TRANSPOSITION_TABLE_HPP

#include "chess/moves/move.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>

namespace chess::validation {
class Verifier;
}

namespace chess::search {

enum class TTBound : uint8_t {
    Exact,
    Upper, // Fail-low (Alpha)
    Lower  // Fail-high (Beta)
};

struct TTEntry {
    uint64_t key = 0;
    int depth = -1;
    int score = 0;
    TTBound bound = TTBound::Exact;
    moves::Move best_move = moves::Move();
    uint8_t age = 0;
};

class TranspositionTable {
    friend class chess::validation::Verifier;
public:
    explicit TranspositionTable(std::size_t size_mb = 16);

    void clear();
    void resize(std::size_t size_mb);

    bool probe(uint64_t key, int depth, int ply, int alpha, int beta, int& score, moves::Move& best_move);
    void store(uint64_t key, int depth, int ply, int score, TTBound bound, const moves::Move& best_move);

    void increment_age() { age_++; }

    std::size_t size() const { return entries_.size(); }
    uint64_t hits() const { return hits_; }
    uint64_t lookups() const { return lookups_; }

private:
    std::vector<TTEntry> entries_;
    uint8_t age_ = 0;
    uint64_t hits_ = 0;
    uint64_t lookups_ = 0;
};

extern TranspositionTable g_tt;

} // namespace chess::search

#endif // CHESS_TRANSPOSITION_TABLE_HPP
