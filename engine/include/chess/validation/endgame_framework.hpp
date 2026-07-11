#ifndef CHESS_ENDGAME_FRAMEWORK_HPP
#define CHESS_ENDGAME_FRAMEWORK_HPP

#include "chess/board/board.hpp"
#include <string>

namespace chess::validation {

enum class TablebaseResult {
    Win,
    Loss,
    Draw,
    Miss
};

class EndgameFramework {
public:
    EndgameFramework();

    // Detects if the position is an endgame (usually non-king material <= 1500 per side)
    static bool is_endgame(const board::Board& board);

    // Generates a canonical material signature, e.g. "KPK", "KQRK", "KRpkr" or "KRPKR"
    // Convention: White material (excluding K) sorted by value, then 'v', then Black material sorted by value
    // Example: White has K+P, Black has K -> "KPvK" or "KPK"
    static std::string get_material_signature(const board::Board& board);

    // Future Syzygy tablebase probing interfaces (stubs)
    static TablebaseResult probe_wdl(const board::Board& board, int& out_score);
    static TablebaseResult probe_dtz(const board::Board& board, moves::Move& out_best_move, int& out_dtz);
};

} // namespace chess::validation

#endif // CHESS_ENDGAME_FRAMEWORK_HPP
