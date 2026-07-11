#ifndef CHESS_SEARCH_HPP
#define CHESS_SEARCH_HPP

#include "chess/board/board.hpp"
#include "chess/search/search_info.hpp"
#include "chess/search/negamax.hpp"

namespace chess::search {

SearchInfo find_best_move(
    board::Board& board,
    int depth,
    int limit_ms = 0,
    int time_left_ms = 0,
    int increment_ms = 0,
    int moves_to_go = 0,
    bool is_infinite = false
);

} // namespace chess::search

#endif // CHESS_SEARCH_HPP
