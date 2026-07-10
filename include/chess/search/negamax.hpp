#ifndef CHESS_NEGAMAX_HPP
#define CHESS_NEGAMAX_HPP

#include "chess/board/board.hpp"
#include "chess/search/search_info.hpp"
#include "chess/search/transposition_table.hpp"
#include "chess/search/time_manager.hpp"
#include <cstring>
#include <vector>

namespace chess::search {

constexpr int kMateScore = 100000;
constexpr int kInfiniteScore = 200000;
constexpr int kDrawScore = 0;

struct SearchContext {
    TranspositionTable& tt;
    TimeManager& time_manager;
    moves::Move killer_moves[128][2];
    int history_moves[2][6][64];
    moves::Move counter_moves[64][64];
    int continuation_history[64][64];
    std::vector<uint64_t> board_history;

    void clear_heuristics() {
        std::memset(killer_moves, 0, sizeof(killer_moves));
        std::memset(history_moves, 0, sizeof(history_moves));
        std::memset(counter_moves, 0, sizeof(counter_moves));
        std::memset(continuation_history, 0, sizeof(continuation_history));
    }
};

int negamax(board::Board& board, int depth, int ply, int alpha, int beta, SearchInfo& info, SearchContext& context, const moves::Move& prev_move = moves::Move());
int qsearch(board::Board& board, int alpha, int beta, SearchInfo& info, SearchContext& context);

} // namespace chess::search

#endif // CHESS_NEGAMAX_HPP
