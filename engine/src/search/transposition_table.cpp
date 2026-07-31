#include "chess/search/transposition_table.hpp"
#include "chess/search/negamax.hpp"

namespace chess::search {

TranspositionTable g_tt(16);

TranspositionTable::TranspositionTable(std::size_t size_mb) {
    resize(size_mb);
}

void TranspositionTable::clear() {
    for (auto& entry : entries_) {
        entry = TTEntry();
    }
    hits_ = 0;
    lookups_ = 0;
}

void TranspositionTable::resize(std::size_t size_mb) {
    std::size_t size_in_bytes = size_mb * 1024 * 1024;
    std::size_t num_entries = size_in_bytes / sizeof(TTEntry);
    if (num_entries < 1024) {
        num_entries = 1024; // Minimum size
    }
    entries_.resize(num_entries);
    clear();
}

bool TranspositionTable::probe(uint64_t key, int depth, int ply, int alpha, int beta, int& score, moves::Move& best_move, TTBound* out_bound) {
    if (entries_.empty()) return false;
    lookups_++;
    size_t index = key % entries_.size();
    const TTEntry& entry = entries_[index];

    if (entry.key == key) {
        best_move = entry.best_move;

        if (entry.depth >= depth) {
            int entry_score = entry.score;
            // Adjust mate scores from position-relative to root-relative
            if (entry_score > kMateScore - 100) {
                entry_score -= ply;
            } else if (entry_score < -kMateScore + 100) {
                entry_score += ply;
            }

            if (entry.bound == TTBound::Exact) {
                score = entry_score;
                hits_++;
                if (out_bound) *out_bound = entry.bound;
                return true;
            }
            if (entry.bound == TTBound::Upper && entry_score <= alpha) {
                score = entry_score;
                hits_++;
                if (out_bound) *out_bound = entry.bound;
                return true;
            }
            if (entry.bound == TTBound::Lower && entry_score >= beta) {
                score = entry_score;
                hits_++;
                if (out_bound) *out_bound = entry.bound;
                return true;
            }
        }
    }
    return false;
}

void TranspositionTable::store(uint64_t key, int depth, int ply, int score, TTBound bound, const moves::Move& best_move) {
    if (entries_.empty()) return;
    size_t index = key % entries_.size();
    TTEntry& entry = entries_[index];

    // Adjust mate scores from root-relative to position-relative
    int store_score = score;
    if (score > kMateScore - 100) {
        store_score += ply;
    } else if (score < -kMateScore + 100) {
        store_score -= ply;
    }

    // Replacement policy: replace if slot is empty, belongs to an older search, or if the new depth is >= the stored depth
    bool replace = (entry.key == 0) || 
                   (entry.age != age_) || 
                   (depth >= entry.depth);

    if (replace) {
        entry.key = key;
        entry.depth = depth;
        entry.score = store_score;
        entry.bound = bound;
        // Keep the previous best move if the new move is none/null
        if (best_move.from().is_none() && entry.key == key) {
            // Keep existing best move
        } else {
            entry.best_move = best_move;
        }
        entry.age = age_;
    }
}

} // namespace chess::search
