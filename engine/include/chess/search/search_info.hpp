#ifndef CHESS_SEARCH_INFO_HPP
#define CHESS_SEARCH_INFO_HPP

#include "chess/moves/move.hpp"
#include <cstdint>
#include <chrono>
#include <vector>

namespace chess::search {

struct SearchTimelinePoint {
    int depth = 0;
    long long duration_ms = 0;
    uint64_t nodes = 0;
    int score = 0;
    std::string best_move;
};

struct AlternativeMove {
    moves::Move move;
    int score = 0;
    std::vector<moves::Move> pv;
};

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

    // Explainable AI fields
    std::vector<SearchTimelinePoint> timeline;
    std::vector<AlternativeMove> alternatives;

    // Capturing search heuristics
    moves::Move killer_moves[128][2];
    int history_moves[2][6][64];
};

} // namespace chess::search

#endif // CHESS_SEARCH_INFO_HPP
