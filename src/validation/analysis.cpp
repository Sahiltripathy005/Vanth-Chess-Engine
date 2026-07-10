#include "chess/validation/analysis.hpp"
#include "chess/search/evaluation.hpp"
#include <sstream>
#include <iomanip>

namespace chess::validation {

EvalBreakdown AnalysisTools::get_evaluation_breakdown(const board::Board& board) {
    int phase = search::get_game_phase(board);
    
    EvalBreakdown b;
    b.material = search::evaluate_material(board);
    b.pst = search::evaluate_piece_square_tables(board, phase);
    b.pawn_structure = search::evaluate_pawn_structure(board);
    b.piece_activity = search::evaluate_piece_activity(board, phase);
    b.king_safety = search::evaluate_king_safety(board, phase);
    b.space = search::evaluate_space(board);
    b.threats = search::evaluate_threats(board);
    b.endgame = search::evaluate_endgame(board, phase);
    b.tempo = search::evaluate_tempo(board);
    b.total = search::evaluate(board);
    
    return b;
}

std::string AnalysisTools::format_evaluation_breakdown(const EvalBreakdown& b) {
    std::stringstream ss;
    ss << "--------------------------------------------------\n"
       << "         STATIC EVALUATION BREAKDOWN              \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "Material:" << std::right << std::setw(10) << b.material << " cp\n"
       << std::left << std::setw(25) << "Piece-Square Tables:" << std::right << std::setw(10) << b.pst << " cp\n"
       << std::left << std::setw(25) << "Pawn Structure:" << std::right << std::setw(10) << b.pawn_structure << " cp\n"
       << std::left << std::setw(25) << "Piece Activity:" << std::right << std::setw(10) << b.piece_activity << " cp\n"
       << std::left << std::setw(25) << "King Safety:" << std::right << std::setw(10) << b.king_safety << " cp\n"
       << std::left << std::setw(25) << "Space Control:" << std::right << std::setw(10) << b.space << " cp\n"
       << std::left << std::setw(25) << "Tactical Threats:" << std::right << std::setw(10) << b.threats << " cp\n"
       << std::left << std::setw(25) << "Endgame Knowledge:" << std::right << std::setw(10) << b.endgame << " cp\n"
       << std::left << std::setw(25) << "Tempo Bonus:" << std::right << std::setw(10) << b.tempo << " cp\n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "TOTAL SCORE:" << std::right << std::setw(10) << b.total << " cp\n"
       << "--------------------------------------------------\n";
    return ss.str();
}

std::string AnalysisTools::get_hash_table_statistics(const search::TranspositionTable& tt) {
    std::stringstream ss;
    size_t entry_bytes = 24; // size of TTEntry
    size_t size_bytes = tt.size() * entry_bytes;
    double size_mb = static_cast<double>(size_bytes) / (1024.0 * 1024.0);
    
    double hit_rate = 0.0;
    if (tt.lookups() > 0) {
        hit_rate = (static_cast<double>(tt.hits()) / tt.lookups()) * 100.0;
    }
    
    ss << "--------------------------------------------------\n"
       << "         TRANSPOSITION TABLE STATISTICS           \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "Total Entries:" << std::right << std::setw(10) << tt.size() << "\n"
       << std::left << std::setw(25) << "Allocated Memory:" << std::right << std::fixed << std::setprecision(2) << std::setw(8) << size_mb << " MB\n"
       << std::left << std::setw(25) << "Total Lookups:" << std::right << std::setw(10) << tt.lookups() << "\n"
       << std::left << std::setw(25) << "Total Hits:" << std::right << std::setw(10) << tt.hits() << "\n"
       << std::left << std::setw(25) << "TT Hit Rate:" << std::right << std::fixed << std::setprecision(2) << std::setw(8) << hit_rate << "%\n"
       << "--------------------------------------------------\n";
    return ss.str();
}

std::string AnalysisTools::get_move_ordering_efficiency(const search::SearchInfo& info) {
    std::stringstream ss;
    double ordering_eff = 0.0;
    if (info.ordering_lookups > 0) {
        ordering_eff = (static_cast<double>(info.fhf) / info.ordering_lookups) * 100.0;
    }
    
    ss << "--------------------------------------------------\n"
       << "         MOVE ORDERING EFFICIENCY                 \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "Ordering Lookups:" << std::right << std::setw(10) << info.ordering_lookups << "\n"
       << std::left << std::setw(25) << "First-Move Beta Cutoffs (FHF):" << std::right << std::setw(10) << info.fhf << "\n"
       << std::left << std::setw(25) << "Ordering Efficiency:" << std::right << std::fixed << std::setprecision(2) << std::setw(8) << ordering_eff << "%\n"
       << "--------------------------------------------------\n";
    return ss.str();
}

std::string AnalysisTools::format_search_timeline(const std::vector<SearchTimelinePoint>& timeline) {
    std::stringstream ss;
    ss << "--------------------------------------------------\n"
       << "         SEARCH TIMELINE STATISTICS               \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(8) << "Depth" 
       << std::left << std::setw(12) << "Time (ms)" 
       << std::left << std::setw(12) << "Nodes" 
       << std::left << std::setw(10) << "Score" 
       << "Best Move\n"
       << "--------------------------------------------------\n";
    for (const auto& pt : timeline) {
        ss << std::left << std::setw(8) << pt.depth
           << std::left << std::setw(12) << pt.duration_ms
           << std::left << std::setw(12) << pt.nodes
           << std::left << std::setw(10) << pt.score
           << pt.best_move << "\n";
    }
    ss << "--------------------------------------------------\n";
    return ss.str();
}

} // namespace chess::validation
