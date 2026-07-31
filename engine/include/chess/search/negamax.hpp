#pragma once

#include "chess/board/board.hpp"
#include "chess/search/search_info.hpp"
#include "chess/search/transposition_table.hpp"
#include "chess/search/time_manager.hpp"
#include <cstring>
#include <vector>
#include <memory>

namespace chess::search {

enum class CaptureOrderingMode {
    MvvLvaOnly,
    SeeOnly,
    Hybrid
};

constexpr int kMateScore = 100000;
constexpr int kInfiniteScore = 200000;
constexpr int kDrawScore = 0;

// Centralized Search Configuration for Tunability (Module 6)
struct SearchConfig {
    double lmr_base = 0.75;
    double lmr_divisor = 2.25;
    int null_move_base = 3;
    int null_move_max = 4;
    int iid_depth = 5;
    int aspiration_initial = 15;
    int aspiration_expansion = 15;
    int futility_margin = 150;
    int rfp_margin = 80;
    bool mdp_enable = true;
    bool nmp_verification = true;
};

extern SearchConfig g_search_config;
extern int g_reductions[64][64];

void init_search_heuristics();

extern bool g_enable_tt;
extern bool g_enable_qsearch;
extern bool g_enable_nmp;
extern bool g_enable_lmr;
extern bool g_enable_rfp;
extern bool g_enable_pvs;
extern bool g_enable_futility;
extern bool g_enable_razoring;
extern bool g_enable_aspiration;
extern CaptureOrderingMode g_capture_ordering_mode;

extern bool g_enable_v36_ordering;
extern bool g_enable_conthist;
extern bool g_enable_countermoves;

struct ContinuationHistoryTable {
    int data[6][64][6][64];
};

struct SearchContext {
    TranspositionTable& tt;
    TimeManager& time_manager;
    moves::Move killer_moves[128][2];
    int history_moves[2][6][64];
    moves::Move counter_moves[64][64];
    std::unique_ptr<ContinuationHistoryTable> continuation_history_1; // [prev_piece_type][prev_to_sq][curr_piece_type][curr_to_sq] (1 ply ago)
    std::unique_ptr<ContinuationHistoryTable> continuation_history_2; // [prev_prev_piece_type][prev_prev_to_sq][curr_piece_type][curr_to_sq] (2 plies ago)
    std::vector<uint64_t> board_history;
    std::vector<moves::Move> move_history; // Tracks move history down the search branch
    moves::Move previous_pv_move = moves::Move();
    int static_eval_history[128];

    SearchContext(TranspositionTable& tt_ref, TimeManager& tm_ref)
        : tt(tt_ref), time_manager(tm_ref),
          continuation_history_1(std::make_unique<ContinuationHistoryTable>()),
          continuation_history_2(std::make_unique<ContinuationHistoryTable>()) {
        std::memset(static_eval_history, 0, sizeof(static_eval_history));
    }

    void clear_heuristics() {
        std::memset(killer_moves, 0, sizeof(killer_moves));
        std::memset(history_moves, 0, sizeof(history_moves));
        std::memset(counter_moves, 0, sizeof(counter_moves));
        std::memset(continuation_history_1->data, 0, sizeof(continuation_history_1->data));
        std::memset(continuation_history_2->data, 0, sizeof(continuation_history_2->data));
        std::memset(static_eval_history, 0, sizeof(static_eval_history));
        previous_pv_move = moves::Move();
        move_history.clear();
    }
};

int negamax(board::Board& board, int depth, int ply, int alpha, int beta, SearchInfo& info, SearchContext& context, const moves::Move& prev_move = moves::Move());
int qsearch(board::Board& board, int alpha, int beta, SearchInfo& info, SearchContext& context);

} // namespace chess::search
