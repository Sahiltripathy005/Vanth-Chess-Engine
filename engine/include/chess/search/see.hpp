#ifndef CHESS_SEE_HPP
#define CHESS_SEE_HPP

#include "chess/board/board.hpp"
#include "chess/moves/move.hpp"

namespace chess::search {

// Returns the material balance (in centipawns) resulting from a series of captures on the target square of the move.
// Returns >= 0 for favorable/equal exchanges, and < 0 for unfavorable ones.
int see(const board::Board& board, const moves::Move& move);

} // namespace chess::search

#endif // CHESS_SEE_HPP
