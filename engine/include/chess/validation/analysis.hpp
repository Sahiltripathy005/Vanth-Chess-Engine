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
    int mobility = 0;
    int piece_activity = 0;
    int king_safety = 0;
    int space = 0;
    int threats = 0;
    int endgame = 0;
    int tempo = 0;
    int total = 0;
};

using SearchTimelinePoint = search::SearchTimelinePoint;

struct MoveExplanation {
    std::string move;
    std::vector<std::string> reasons;
};

struct BlunderAnalysis {
    std::string move_played;
    int score_drop = 0;
    bool is_blunder = false;
    bool material_loss = false;
    bool positional_loss = false;
    bool tactical_mistake = false;
    bool king_safety_mistake = false;
    bool missed_opportunity = false;
    std::string explanation;
};

struct TacticalThemes {
    bool fork = false;
    bool pin = false;
    bool skewer = false;
    bool discovered_attack = false;
    bool double_attack = false;
    bool deflection = false;
    bool decoy = false;
    bool x_ray = false;
    bool back_rank_weakness = false;
    bool remove_defender = false;
    bool zwischenzug = false;
    std::vector<std::string> descriptions;
};

struct PositionClassifications {
    bool opening = false;
    bool middlegame = false;
    bool endgame = false;
    bool closed_position = false;
    bool open_position = false;
    bool semi_open = false;
    bool tactical = false;
    bool strategic = false;
    bool king_attack = false;
    bool pawn_race = false;
    std::vector<std::string> labels;
};

struct FullAnalysisReport {
    std::string fen;
    std::string best_move;
    int evaluation = 0;
    EvalBreakdown breakdown;
    std::vector<search::AlternativeMove> alternatives;
    MoveExplanation explanation;
    TacticalThemes tactical_themes;
    PositionClassifications classifications;
    std::vector<std::string> strengths;
    std::vector<std::string> weaknesses;
    search::SearchInfo search_stats;
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

    // Explain the strategic/tactical reasons for a move
    static MoveExplanation explain_move(const board::Board& board, const moves::Move& move);

    // Analyze if a move was a blunder and why
    static BlunderAnalysis analyze_blunder(const board::Board& board, const moves::Move& move_played, int depth = 5);

    // Detect tactical themes present in the position
    static TacticalThemes detect_tactical_themes(const board::Board& board);

    // Classify the position type
    static PositionClassifications classify_position(const board::Board& board);

    // Generate a complete analysis report
    static FullAnalysisReport generate_report(board::Board& board, int depth = 5);

    // Format the full analysis report into JSON string
    static std::string report_to_json(const FullAnalysisReport& report);
};

} // namespace chess::validation

#endif // CHESS_ANALYSIS_HPP
