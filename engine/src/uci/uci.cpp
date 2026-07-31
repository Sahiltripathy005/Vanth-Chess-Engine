#include "chess/uci/uci.hpp"
#include "chess/uci/option_manager.hpp"
#include "chess/moves/move_generator.hpp"
#include "chess/search/search.hpp"
#include "chess/search/time_manager.hpp"
#include "chess/search/transposition_table.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>
#include <cmath>

namespace chess::uci {

// Forward declarations of parser/dispatcher helpers
std::vector<std::string> split_string(const std::string& str);
void dispatch_command(UCIEngine* engine, const std::string& line);
void parse_go_params(const std::string& args, int& depth, int& limit_ms, int& wtime, int& btime, int& winc, int& binc, int& movestogo, bool& is_infinite);

UCIEngine::UCIEngine() : board_(board::Board::start_position()), searching_(false) {
}

UCIEngine::~UCIEngine() {
    handle_stop();
}

void UCIEngine::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") {
            handle_stop();
            break;
        }
        dispatch_command(this, line);
    }
}

void UCIEngine::handle_uci() {
    std::cout << "id name Vanth Chess Engine\n";
    std::cout << "id author Google Deepmind team\n";
    OptionManager::get().print_options();
    std::cout << "uciok\n" << std::endl;
}

void UCIEngine::handle_isready() {
    std::cout << "readyok\n" << std::endl;
}

void UCIEngine::handle_ucinewgame() {
    handle_stop();
    board_ = board::Board::start_position();
    search::g_tt.clear();
}

void UCIEngine::handle_position(const std::string& args) {
    std::string fen;
    std::vector<std::string> move_strings;

    size_t moves_pos = args.find("moves");
    std::string pos_part = (moves_pos != std::string::npos) ? args.substr(0, moves_pos) : args;
    std::string moves_part = (moves_pos != std::string::npos) ? args.substr(moves_pos + 5) : "";

    if (pos_part.find("startpos") != std::string::npos) {
        board_ = board::Board::start_position();
    } else {
        size_t fen_idx = pos_part.find("fen");
        if (fen_idx != std::string::npos) {
            fen = pos_part.substr(fen_idx + 4);
            // Trim trailing spaces
            while (!fen.empty() && std::isspace(fen.back())) {
                fen.pop_back();
            }
            // Trim leading spaces
            while (!fen.empty() && std::isspace(fen.front())) {
                fen.erase(fen.begin());
            }
            if (!board_.load_fen(fen)) {
                board_ = board::Board::start_position();
            }
        }
    }

    if (!moves_part.empty()) {
        std::stringstream ss(moves_part);
        std::string mv;
        while (ss >> mv) {
            move_strings.push_back(mv);
        }
        play_moves(move_strings);
    }
}

void UCIEngine::handle_go(const std::string& args) {
    handle_stop();

    int depth = 0;
    int limit_ms = 0;
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    bool is_infinite = false;

    // Check for ponder flag in go command
    if (args.find("ponder") != std::string::npos) {
        is_infinite = true;
    }

    parse_go_params(args, depth, limit_ms, wtime, btime, winc, binc, movestogo, is_infinite);

    search_thread_ = std::thread(&UCIEngine::async_search, this, depth, limit_ms, wtime, btime, winc, binc, movestogo, is_infinite);
}

void UCIEngine::handle_setoption(const std::string& args) {
    // Expected format: setoption name <name> [value <val>]
    size_t name_pos = args.find("name");
    if (name_pos == std::string::npos) return;

    size_t val_pos = args.find("value");
    std::string name_str;
    std::string val_str;

    if (val_pos != std::string::npos) {
        name_str = args.substr(name_pos + 5, val_pos - (name_pos + 5));
        val_str = args.substr(val_pos + 6);
    } else {
        name_str = args.substr(name_pos + 5);
    }

    // Trim spaces
    while (!name_str.empty() && std::isspace(name_str.back())) name_str.pop_back();
    while (!name_str.empty() && std::isspace(name_str.front())) name_str.erase(name_str.begin());
    while (!val_str.empty() && std::isspace(val_str.back())) val_str.pop_back();
    while (!val_str.empty() && std::isspace(val_str.front())) val_str.erase(val_str.begin());

    OptionManager::get().set_option(name_str, val_str);
}

void UCIEngine::handle_stop() {
    search::g_stop_search = true;
    if (search_thread_.joinable()) {
        search_thread_.join();
    }
}

void UCIEngine::handle_debug(const std::string& args) {
    (void)args;
}

void UCIEngine::handle_register() {
    std::cout << "registration ok\n" << std::endl;
}

void UCIEngine::play_moves(const std::vector<std::string>& move_strings) {
    for (const auto& mv_str : move_strings) {
        std::vector<moves::Move> legal = moves::MoveGenerator::generate_legal_moves(board_);
        bool found = false;
        for (const auto& m : legal) {
            if (m.to_string() == mv_str) {
                board_.make_move(m);
                found = true;
                break;
            }
        }
        if (!found) {
            break; // Stop if move is illegal
        }
    }
}

void UCIEngine::async_search(int depth, int limit_ms, int wtime, int btime, int winc, int binc, int movestogo, bool is_infinite) {
    searching_ = true;
    search::g_stop_search = false;

    if (depth <= 0) {
        depth = 64; // Default depth limit
    }

    int time_left = 0;
    int inc = 0;
    if (board_.active_color() == pieces::Color::White) {
        time_left = wtime;
        inc = winc;
    } else {
        time_left = btime;
        inc = binc;
    }

    auto info = search::find_best_move(board_, depth, limit_ms, time_left, inc, movestogo, is_infinite);
    last_search_info_ = info;

    if (!info.best_move.from().is_none()) {
        std::cout << "bestmove " << info.best_move.to_string();
        if (info.pv.size() >= 2) {
            std::cout << " ponder " << info.pv[1].to_string();
        }
        std::cout << std::endl;
    } else {
        auto legal = moves::MoveGenerator::generate_legal_moves(board_);
        if (!legal.empty()) {
            std::cout << "bestmove " << legal[0].to_string() << std::endl;
        } else {
            std::cout << "bestmove 0000" << std::endl;
        }
    }

    searching_ = false;
}

void UCIEngine::handle_search_profile() {
    if (last_search_info_.nodes == 0) {
        std::cout << "No search profile data available. Run a search (e.g. 'go depth 5') first." << std::endl;
        return;
    }

    const auto& info = last_search_info_;
    std::cout << "========================================================\n";
    std::cout << "               VANTH SEARCH PROFILE REPORT              \n";
    std::cout << "========================================================\n\n";

    std::cout << "--- General Search Metrics ---\n";
    std::cout << "Nodes Searched:       " << info.nodes << "\n";
    std::cout << "Q-Nodes Searched:     " << info.q_nodes << "\n";
    std::cout << "Total Time:           " << info.time.count() << " ms\n";
    double seconds = info.time.count() / 1000.0;
    uint64_t nps = (seconds > 0.0) ? static_cast<uint64_t>(info.nodes / seconds) : 0;
    std::cout << "NPS:                  " << nps << "\n";
    std::cout << "Max Depth Reached:    " << info.max_depth << "\n\n";

    std::cout << "--- Move Ordering & Branching Statistics ---\n";
    std::cout << "Nodes Visited:        " << info.nodes_visited << "\n";
    std::cout << "Total Moves Generated:" << info.moves_generated_total << "\n";
    std::cout << "Total Moves Searched: " << info.moves_searched_total << "\n";
    double avg_gen = info.nodes_visited > 0 ? (double)info.moves_generated_total / info.nodes_visited : 0.0;
    double avg_searched = info.nodes_visited > 0 ? (double)info.moves_searched_total / info.nodes_visited : 0.0;
    std::cout << "Avg Generated/Node:   " << std::fixed << std::setprecision(2) << avg_gen << "\n";
    std::cout << "Avg Searched/Node:    " << avg_searched << "\n";
    double fhf_pct = info.ordering_lookups > 0 ? (double)info.fhf / info.ordering_lookups * 100.0 : 0.0;
    std::cout << "First Move Cutoff %:  " << fhf_pct << "%\n";
    double avg_cutoff_idx = info.cutoff_count_total > 0 ? (double)info.cutoff_index_sum / info.cutoff_count_total : 0.0;
    std::cout << "Avg Cutoff Move Index:" << avg_cutoff_idx << "\n\n";

    std::cout << "--- Cutoff & Heuristic Statistics ---\n";
    std::cout << "Total Cutoffs:        " << info.cutoffs << "\n";
    std::cout << "NMP Cutoffs:          " << info.nmp_cutoffs << "\n";
    std::cout << "PVS Researches:       " << info.pvs_researches << "\n\n";

    auto print_heur = [](const std::string& name, uint64_t searched, uint64_t cutoffs) {
        double rate = searched > 0 ? (double)cutoffs / searched * 100.0 : 0.0;
        std::cout << std::left << std::setw(22) << (name + ":")
                  << "Searched: " << std::right << std::setw(8) << searched
                  << " | Cutoffs: " << std::setw(8) << cutoffs
                  << " | Cutoff Rate: " << std::fixed << std::setprecision(2) << std::setw(6) << rate << "%\n";
    };

    print_heur("TT Move", info.tt_move_searched, info.tt_move_cutoffs);
    print_heur("Killer Move", info.killer_searched, info.killer_cutoffs);
    print_heur("Countermove", info.countermove_searched, info.countermove_cutoffs);
    print_heur("History Move", info.history_searched, info.history_cutoffs);
    print_heur("Winning Captures (SEE)", info.see_winning_searched, info.see_winning_cutoffs);
    print_heur("Neutral Captures (SEE)", info.see_neutral_searched, info.see_neutral_cutoffs);
    print_heur("Losing Captures (SEE)", info.see_losing_searched, info.see_losing_cutoffs);
    std::cout << "\n";

    std::cout << "--- History Heuristic Details ---\n";
    double avg_hist = info.history_score_count > 0 ? (double)info.history_score_sum / info.history_score_count : 0.0;
    std::cout << "Avg Quiet Move History Score: " << std::fixed << std::setprecision(1) << avg_hist << "\n\n";

    std::cout << "--- Late Move Reduction (LMR) Statistics ---\n";
    std::cout << "Late Moves Searched:  " << info.late_moves_searched << "\n";
    double avg_late_pos = info.late_moves_searched > 0 ? (double)info.late_moves_position_sum / info.late_moves_searched : 0.0;
    std::cout << "Avg Late Move Index:  " << std::fixed << std::setprecision(1) << avg_late_pos << "\n";
    std::cout << "LMR Reductions:       " << info.lmr_reductions << "\n";
    double avg_reduction = info.lmr_reductions > 0 ? (double)info.lmr_reduction_depth_sum / info.lmr_reductions : 0.0;
    std::cout << "Avg Reduction Depth:  " << std::fixed << std::setprecision(1) << avg_reduction << "\n";

    std::cout << "========================================================\n" << std::endl;
}

void UCIEngine::handle_benchmark_compare() {
    std::cout << "Running benchmark comparison at depth 5 (start position)..." << std::endl;

    // Save current board
    board::Board saved_board = board_;

    // Reset to start position
    board_ = board::Board::start_position();

    // Clear transposition table to ensure clean search
    search::g_tt.clear();

    // Run search
    auto start_time = std::chrono::high_resolution_clock::now();
    search::SearchInfo opt = search::find_best_move(board_, 5);
    auto end_time = std::chrono::high_resolution_clock::now();
    long long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Restore board
    board_ = saved_board;

    // Default Baseline metrics (from benchmark_results.json)
    uint64_t base_nodes = 3395;
    int base_depth = 5;
    (void)base_depth;
    long long base_time = 76;
    uint64_t base_nps = 44671;
    double base_tt_hit_pct = 5.51;
    double base_cutoff_pct = 84.51; // move ordering efficiency
    std::string base_pv = "g1f3";

    // Attempt to read from benchmark_results.json
    std::ifstream f("benchmark_results.json");
    if (f.is_open()) {
        std::stringstream ss;
        ss << f.rdbuf();
        std::string content = ss.str();
        // Look for the block with "depth": 5
        size_t d5_pos = content.find("\"depth\": 5");
        if (d5_pos != std::string::npos) {
            // Find nodes
            size_t nodes_pos = content.find("\"nodes\":", d5_pos);
            if (nodes_pos != std::string::npos) {
                base_nodes = std::stoull(content.substr(nodes_pos + 8));
            }
            // Find time_ms
            size_t time_pos = content.find("\"time_ms\":", d5_pos);
            if (time_pos != std::string::npos) {
                base_time = std::stoll(content.substr(time_pos + 10));
            }
            // Find nps
            size_t nps_pos = content.find("\"nps\":", d5_pos);
            if (nps_pos != std::string::npos) {
                base_nps = std::stoull(content.substr(nps_pos + 6));
            }
            // Find move_ordering_efficiency
            size_t mo_pos = content.find("\"move_ordering_efficiency\":", d5_pos);
            if (mo_pos != std::string::npos) {
                base_cutoff_pct = std::stod(content.substr(mo_pos + 27));
            }
            // Find best_move
            size_t bm_pos = content.find("\"best_move\":", d5_pos);
            if (bm_pos != std::string::npos) {
                size_t start_quote = content.find("\"", bm_pos + 12);
                size_t end_quote = content.find("\"", start_quote + 1);
                if (start_quote != std::string::npos && end_quote != std::string::npos) {
                    base_pv = content.substr(start_quote + 1, end_quote - start_quote - 1);
                }
            }
            // Find tt_hits and tt_lookups
            size_t tth_pos = content.find("\"tt_hits\":", d5_pos);
            size_t ttl_pos = content.find("\"tt_lookups\":", d5_pos);
            if (tth_pos != std::string::npos && ttl_pos != std::string::npos) {
                uint64_t tth = std::stoull(content.substr(tth_pos + 10));
                uint64_t ttl = std::stoull(content.substr(ttl_pos + 13));
                base_tt_hit_pct = ttl > 0 ? (double)tth / ttl * 100.0 : 0.0;
            }
        }
    }

    // Calculated optimized values
    double opt_tt_hit_pct = opt.tt_lookups > 0 ? (double)opt.tt_hits / opt.tt_lookups * 100.0 : 0.0;
    double opt_cutoff_pct = opt.ordering_lookups > 0 ? (double)opt.fhf / opt.ordering_lookups * 100.0 : 0.0;
    double opt_seconds = duration_ms / 1000.0;
    uint64_t opt_nps = (opt_seconds > 0.0) ? static_cast<uint64_t>(opt.nodes / opt_seconds) : 0;
    std::string opt_pv = opt.best_move.to_string();

    // Print comparison table
    std::cout << "\n====================================================================\n";
    std::cout << "                 BENCHMARK PERFORMANCE COMPARISON\n";
    std::cout << "====================================================================\n";
    std::cout << std::left << std::setw(25) << "Metric"
              << std::right << std::setw(15) << "Baseline"
              << std::right << std::setw(15) << "Optimized"
              << std::right << std::setw(13) << "Change" << "\n";
    std::cout << "--------------------------------------------------------------------\n";

    auto print_row_int = [](const std::string& label, uint64_t base, uint64_t opt_val, bool lower_is_better) {
        double pct_change = base > 0 ? (double)(static_cast<double>(opt_val) - static_cast<double>(base)) / base * 100.0 : 0.0;
        std::cout << std::left << std::setw(25) << label
                  << std::right << std::setw(15) << base
                  << std::right << std::setw(15) << opt_val;
        std::cout << std::right << std::setw(12) << std::fixed << std::setprecision(1) << std::showpos << pct_change << "%";
        
        // Show status indicator
        if (std::abs(pct_change) < 0.1) {
            std::cout << " =";
        } else {
            bool improved = (pct_change < 0 && lower_is_better) || (pct_change > 0 && !lower_is_better);
            std::cout << (improved ? " +" : " -");
        }
        std::cout << "\n" << std::noshowpos;
    };

    auto print_row_double = [](const std::string& label, double base, double opt_val, bool lower_is_better) {
        double diff = opt_val - base;
        std::cout << std::left << std::setw(25) << label
                  << std::right << std::setw(14) << std::fixed << std::setprecision(2) << base << "%"
                  << std::right << std::setw(14) << opt_val << "%";
        std::cout << std::right << std::setw(12) << std::fixed << std::setprecision(2) << std::showpos << diff << "%";
        
        if (std::abs(diff) < 0.01) {
            std::cout << " =";
        } else {
            bool improved = (diff < 0 && lower_is_better) || (diff > 0 && !lower_is_better);
            std::cout << (improved ? " +" : " -");
        }
        std::cout << "\n" << std::noshowpos;
    };

    print_row_int("Nodes Searched", base_nodes, opt.nodes, true);
    print_row_int("Search Time (ms)", base_time, duration_ms, true);
    print_row_int("NPS", base_nps, opt_nps, false);
    print_row_double("TT Hit Rate", base_tt_hit_pct, opt_tt_hit_pct, false);
    print_row_double("First Move Cutoff Rate", base_cutoff_pct, opt_cutoff_pct, false);
    
    std::cout << "--------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(25) << "Best Move (PV)"
              << std::right << std::setw(15) << base_pv
              << std::right << std::setw(15) << opt_pv
              << std::right << std::setw(13) << (base_pv == opt_pv ? "Stable" : "Changed") << "\n";
    std::cout << "====================================================================\n" << std::endl;
}

} // namespace chess::uci
