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
    uint64_t tt_exact_hits = 0;
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

    // Search optimization profiling fields (Version 3.6)
    uint64_t moves_generated_total = 0;
    uint64_t moves_searched_total = 0;
    uint64_t nodes_visited = 0;
    uint64_t cutoff_index_sum = 0;
    uint64_t cutoff_count_total = 0;

    uint64_t tt_move_searched = 0;
    uint64_t tt_move_cutoffs = 0;
    uint64_t killer_searched = 0;
    uint64_t killer_cutoffs = 0;
    uint64_t countermove_searched = 0;
    uint64_t countermove_cutoffs = 0;
    uint64_t history_searched = 0;
    uint64_t history_cutoffs = 0;
    uint64_t see_winning_searched = 0;
    uint64_t see_winning_cutoffs = 0;
    uint64_t see_neutral_searched = 0;
    uint64_t see_neutral_cutoffs = 0;
    uint64_t see_losing_searched = 0;
    uint64_t see_losing_cutoffs = 0;

    // History scores accumulation for average reporting
    uint64_t history_score_sum = 0;
    uint64_t history_score_count = 0;

    // Late move / LMR profiling fields (Version 3.6)
    uint64_t lmr_reduction_depth_sum = 0;
    uint64_t late_moves_searched = 0;
    uint64_t late_moves_position_sum = 0;

    // Advanced selectivity and search tree metrics (Version 3.7)
    uint64_t pv_nodes_count = 0;
    uint64_t cut_nodes_count = 0;
    uint64_t all_nodes_count = 0;
    uint64_t tt_lower_hits = 0;
    uint64_t tt_upper_hits = 0;
    uint64_t lmr_usage = 0;
    uint64_t nmp_usage = 0;
    uint64_t iid_usage = 0;
    uint64_t mdp_usage = 0;
    uint64_t total_reduction_depth = 0;
    uint64_t total_extension_depth = 0;
    uint64_t iid_calls = 0;
    uint64_t iid_successes = 0;
    uint64_t aspiration_expansions = 0;
    uint64_t aspiration_researches = 0;
    uint64_t mdp_prunings = 0;
    uint64_t delta_prunings = 0;
};

} // namespace chess::search

#endif // CHESS_SEARCH_INFO_HPP

