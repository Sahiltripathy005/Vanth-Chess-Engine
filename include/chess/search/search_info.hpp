#ifndef CHESS_SEARCH_INFO_HPP
#define CHESS_SEARCH_INFO_HPP

#include "chess/moves/move.hpp"
#include <cstdint>
#include <chrono>

namespace chess::search {

struct SearchInfo {
    uint64_t nodes = 0;
    uint64_t leaf_nodes = 0;
    uint64_t cutoffs = 0;
    int max_depth = 0;
    std::chrono::milliseconds time{0};
    moves::Move best_move;
    int best_score = 0;

    // Milestone 4 additional fields
    uint64_t tt_hits = 0;
    uint64_t tt_lookups = 0;
    uint64_t fhf = 0;              // Beta cutoffs on the first move
    uint64_t ordering_lookups = 0; // Total nodes where cutoffs occurred
    std::vector<moves::Move> pv;   // Principal Variation

    // Milestone 5 additional fields
    uint64_t q_nodes = 0;
    uint64_t lmr_reductions = 0;
    uint64_t nmp_cutoffs = 0;
    uint64_t pvs_researches = 0;
    bool aborted = false;
};

} // namespace chess::search

#endif // CHESS_SEARCH_INFO_HPP
