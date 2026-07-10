#ifndef CHESS_ANALYSIS_HPP
#define CHESS_ANALYSIS_HPP

#include "chess/board/board.hpp"
#include "chess/search/search_info.hpp"
#include "chess/search/transposition_table.hpp"
#include <string>
#include <vector>

namespace chess::validation {

struct EvalBreakdown {
    int material = 0;
    int pst = 0;
    int pawn_structure = 0;
    int piece_activity = 0;
    int king_safety = 0;
    int space = 0;
    int threats = 0;
    int endgame = 0;
    int tempo = 0;
    int total = 0;
};

struct SearchTimelinePoint {
    int depth = 0;
    long long duration_ms = 0;
    uint64_t nodes = 0;
    int score = 0;
    std::string best_move;
};

class AnalysisTools {
public:
    // Returns static evaluation scores broken down by category
    static EvalBreakdown get_evaluation_breakdown(const board::Board& board);

    // Formats evaluation breakdown into a beautiful, aligned string
    static std::string format_evaluation_breakdown(const EvalBreakdown& breakdown);

    // Prints transposition table statistics (e.g. usage, size, hit rates)
    static std::string get_hash_table_statistics(const search::TranspositionTable& tt);

    // Prints move ordering efficiency metrics (First-Move Beta Cutoffs, lookup counts)
    static std::string get_move_ordering_efficiency(const search::SearchInfo& info);

    // Generates a search timeline string from a list of iterative deepening points
    static std::string format_search_timeline(const std::vector<SearchTimelinePoint>& timeline);
};

} // namespace chess::validation

#endif // CHESS_ANALYSIS_HPP
