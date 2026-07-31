#include "chess/board/board.hpp"
#include "chess/moves/move.hpp"
#include "chess/moves/move_generator.hpp"
#include "chess/search/search.hpp"
#include "chess/search/evaluation.hpp"
#include "chess/validation/tuning.hpp"
#include "chess/validation/tournament.hpp"
#include "chess/validation/test_suite.hpp"
#include "chess/validation/benchmark.hpp"
#include "chess/validation/analysis.hpp"
#include "chess/validation/opening_book.hpp"
#include "chess/validation/endgame_framework.hpp"
#include "chess/uci/uci.hpp"
#include "chess/validation/verifier.hpp"
#include "chess/validation/rating_lab.hpp"
#include "chess/validation/engine_validation.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>

using namespace chess::board;
using namespace chess::search;
using namespace chess::validation;
using namespace chess::uci;
using namespace chess::moves;

void print_usage() {
    std::cout << "Vanth Chess Engine Competitive & Testing Tool\n";
    std::cout << "Usage:\n";
    std::cout << "  chess_benchmarks.exe bench [depth]                      - Run standard search benchmark (default: 5)\n";
    std::cout << "  chess_benchmarks.exe eval [fen]                         - Show static evaluation breakdown\n";
    std::cout << "  chess_benchmarks.exe explain [move] [fen]               - Explain why a move is played\n";
    std::cout << "  chess_benchmarks.exe analyze [depth] [fen]              - Generate explainable chess AI analysis report in JSON\n";
    std::cout << "  chess_benchmarks.exe legalmoves [fen]                   - List all legal moves in the position\n";
    std::cout << "  chess_benchmarks.exe move [fen] [move]                  - Apply a move and output the resulting FEN\n";
    std::cout << "  chess_benchmarks.exe tournament [d1] [d2] [games]       - Run a match between two instances of the engine\n";
    std::cout << "  chess_benchmarks.exe selfplay [games] [depth] [time_ms]  - Run automated self-play game sessions\n";
    std::cout << "  chess_benchmarks.exe testsuite [epd_file] [depth] [ms]   - Run tactical test suite on an EPD file\n";
    std::cout << "  chess_benchmarks.exe tune [iterations] [param1] [param2] - Run Hill Climbing parameter optimization\n";
    std::cout << "\nRating & Tournament Laboratory Commands:\n";
    std::cout << "  chess_benchmarks.exe match [p1] [p2] [games] [depth]    - Run a color-balanced match\n";
    std::cout << "  chess_benchmarks.exe arena [rounds] [depth] [p1] [p2]... - Run a round robin arena tournament\n";
    std::cout << "  chess_benchmarks.exe elo [wins] [losses] [draws]        - Calculate Elo rating difference and error margin\n";
    std::cout << "  chess_benchmarks.exe sprt [wins] [losses] [draws] [e0] [e1] - Run Sequential Probability Ratio Test\n";
    std::cout << "  chess_benchmarks.exe compare [base] [patch] [games] [depth] - Run patch comparison between two weights files\n";
    std::cout << "  chess_benchmarks.exe ratings [db_file]                  - List all stored engine ratings\n";
    std::cout << "  chess_benchmarks.exe leaderboard [db_file]              - Print rating leaderboard\n";
    std::cout << "  chess_benchmarks.exe history [name] [db_file]           - View rating history of a specific engine\n";
    std::cout << "  chess_benchmarks.exe benchmark [depth]                  - Run benchmarks and output CSV/JSON metrics correlation\n";
    std::cout << "\nVerification & Debugging Commands:\n";
    std::cout << "  chess_benchmarks.exe validate [fen]                     - Validate position correctness & invariants\n";
    std::cout << "  chess_benchmarks.exe audit_material [fen]               - Count material balance and calculate static score\n";
    std::cout << "  chess_benchmarks.exe trace_search [depth] [fen]         - Run a search and output detailed execution trace\n";
    std::cout << "  chess_benchmarks.exe trace_eval [fen]                   - Print detailed subscore evaluation breakdown\n";
    std::cout << "  chess_benchmarks.exe trace_hash [fen]                   - Show step-by-step incremental Zobrist hash calculation\n";
    std::cout << "  chess_benchmarks.exe trace_movegen [fen]                - List all generated pseudo-legal and legal moves with filtering details\n";
    std::cout << "  chess_benchmarks.exe trace_tt [fen]                     - Show Transposition Table statistics & probe info\n";
    std::cout << "  chess_benchmarks.exe trace_history [depth] [fen]        - Output killer moves and history tables post-search\n";
    std::cout << "  chess_benchmarks.exe trace_makeundo [fen]               - Verify move execution/retraction consistency\n";
    std::cout << "  chess_benchmarks.exe selfcheck                          - Run engine diagnostic checks (perft, make/undo, board)\n";
    std::cout << "  chess_benchmarks.exe bugtest [id]                       - Run bug reproduction suite (all or specific bug ID)\n";
    std::cout << "  chess_benchmarks.exe stress [games]                     - Stress test moves generation & state invariants\n";
    std::cout << "  chess_benchmarks.exe validate_engine [subcmd] [arg]     - Run professional engine validation suite (CI-ready)\n";
    std::cout << "  chess_benchmarks.exe search_profile [depth] [fen]       - Run detailed search profiling on position\n";
    std::cout << "  chess_benchmarks.exe benchmark_compare [depth]          - Compare baseline vs optimized search efficiency\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        // Start the UCI engine loop by default for GUI compatibility
        UCIEngine engine;
        engine.run();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "uci") {
        UCIEngine engine;
        engine.run();
        return 0;
    }
    else if (cmd == "bench") {
        int max_depth = (argc >= 3) ? std::stoi(argv[2]) : 5;
        std::cout << "Running benchmark from depth 1 to " << max_depth << "...\n";
        
        std::ofstream txt_file("benchmark_results.txt");
        if (txt_file.is_open()) {
            txt_file << "Depth,Time(ms),Nodes,LeafNodes,Cutoffs,NPS,TTHits,TTLookups,FHF,OrderingLookups,BestMove,BestScore\n";
        }
        
        std::vector<BenchmarkMetrics> all_metrics;
        g_tt.clear(); // Clear TT before benchmark run
        
        for (int d = 1; d <= max_depth; ++d) {
            Board board = Board::start_position();
            
            auto start = std::chrono::high_resolution_clock::now();
            auto info = find_best_move(board, d);
            auto end = std::chrono::high_resolution_clock::now();
            long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            
            double seconds = duration / 1000.0;
            uint64_t nps = (seconds > 0.0) ? static_cast<uint64_t>(info.nodes / seconds) : 0;
            
            std::cout << "Depth " << d << ": " << duration << " ms, " << info.nodes << " nodes, " << nps << " NPS, best move: " << info.best_move.to_string() << "\n";
            
            if (txt_file.is_open()) {
                txt_file << d << ","
                         << duration << ","
                         << info.nodes << ","
                         << info.leaf_nodes << ","
                         << info.cutoffs << ","
                         << nps << ","
                         << info.tt_hits << ","
                         << info.tt_lookups << ","
                         << info.fhf << ","
                         << info.ordering_lookups << ","
                         << info.best_move.to_string() << ","
                         << info.best_score << "\n";
            }
            
            BenchmarkMetrics m;
            // Generate current date
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf;
#ifdef _WIN32
            localtime_s(&tm_buf, &now_time);
#else
            localtime_r(&now_time, &tm_buf);
#endif
            std::stringstream ss;
            ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
            
            m.date = ss.str();
            m.commit_hash = "local_dev";
            m.config_name = "default";
            m.depth = d;
            m.time_ms = duration;
            m.nodes = info.nodes;
            m.cutoffs = info.cutoffs;
            m.tt_hits = info.tt_hits;
            m.tt_lookups = info.tt_lookups;
            m.nps = nps;
            m.branching_factor = (d > 0) ? std::pow(static_cast<double>(info.nodes), 1.0 / d) : 0.0;
            if (info.ordering_lookups > 0) {
                m.move_ordering_efficiency = (static_cast<double>(info.fhf) / info.ordering_lookups) * 100.0;
            } else {
                m.move_ordering_efficiency = 0.0;
            }
            m.memory_usage_kb = BenchmarkSuite::get_memory_usage_kb();
            m.best_move = info.best_move.to_string();
            m.evaluation = info.best_score;
            
            all_metrics.push_back(m);
        }
        
        BenchmarkSuite::save_to_json("benchmark_results.json", all_metrics);
        BenchmarkSuite::save_to_csv("benchmark_results.csv", all_metrics);
        std::cout << "Saved results to benchmark_results.txt, JSON, and CSV.\n";
    } 
    else if (cmd == "eval") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        auto breakdown = AnalysisTools::get_evaluation_breakdown(board);
        std::cout << "Material signature: " << EndgameFramework::get_material_signature(board) << "\n";
        std::cout << "Is endgame: " << (EndgameFramework::is_endgame(board) ? "Yes" : "No") << "\n";
        std::cout << AnalysisTools::format_evaluation_breakdown(breakdown);
    } 
    else if (cmd == "tournament") {
        int d1 = (argc >= 3) ? std::stoi(argv[2]) : 4;
        int d2 = (argc >= 4) ? std::stoi(argv[3]) : 4;
        int games = (argc >= 5) ? std::stoi(argv[4]) : 10;

        std::cout << "Running tournament: Player1 (depth " << d1 << ") vs Player2 (depth " << d2 << ") over " << games << " games...\n";
        MatchPlayer p1{"Player1", d1, std::chrono::milliseconds(200), ""};
        MatchPlayer p2{"Player2", d2, std::chrono::milliseconds(200), ""};

        auto res = TournamentManager::run_match(p1, p2, games);
        std::cout << "\nMatch results:\n";
        std::cout << "  Player1 Wins:     " << res.player1_wins << "\n";
        std::cout << "  Player2 Wins:     " << res.player2_wins << "\n";
        std::cout << "  Draws:            " << res.draws << "\n";
        std::cout << "  Avg Game Length:  " << res.avg_game_length << " moves\n";
        std::cout << "  Avg Nodes Searched: " << res.avg_nodes_searched << "\n";
    } 
    else if (cmd == "selfplay") {
        int games = (argc >= 3) ? std::stoi(argv[2]) : 10;
        int depth = (argc >= 4) ? std::stoi(argv[3]) : 4;
        int time_ms = (argc >= 5) ? std::stoi(argv[4]) : 100;

        std::cout << "Running self-play: " << games << " games at depth " << depth << " / " << time_ms << "ms...\n";
        auto res = TournamentManager::run_self_play(games, depth, std::chrono::milliseconds(time_ms));
        std::cout << "\nSelf-play Results:\n";
        std::cout << "  White Wins:       " << res.player1_wins << "\n";
        std::cout << "  Black Wins:       " << res.player2_wins << "\n";
        std::cout << "  Draws:            " << res.draws << "\n";
        std::cout << "  Avg Game Length:  " << res.avg_game_length << " moves\n";
    } 
    else if (cmd == "testsuite") {
        if (argc < 3) {
            std::cerr << "Error: epd_file path required.\n";
            return 1;
        }
        std::string epd_file = argv[2];
        int depth = (argc >= 4) ? std::stoi(argv[3]) : 4;
        int time_ms = (argc >= 5) ? std::stoi(argv[4]) : 200;

        std::cout << "Running test suite on file: " << epd_file << " at depth " << depth << "...\n";
        auto res = TestSuiteRunner::run_file(epd_file, depth, time_ms);
        std::cout << "\nTest Suite Results:\n";
        std::cout << "  Total Positions:  " << res.total_positions << "\n";
        std::cout << "  Correct Solved:   " << res.correct_positions << "\n";
        std::cout << "  Accuracy:         " << std::fixed << std::setprecision(2) << res.accuracy_percentage << "%\n";
        std::cout << "  Avg Search Time:  " << res.avg_search_time_ms << " ms\n";
    } 
    else if (cmd == "tune") {
        if (argc < 4) {
            std::cerr << "Error: iterations and at least one parameter required.\n";
            return 1;
        }
        int iterations = std::stoi(argv[2]);
        std::vector<std::string> params;
        for (int i = 3; i < argc; ++i) {
            params.push_back(argv[i]);
        }

        // Define a simple evaluator that runs matches to evaluate parameter performance
        auto evaluator = [](const std::string& param_name, int val) {
            (void)param_name;
            (void)val;
            // For demonstration, we run a short 2-game self-play match to verify stability
            std::cout << "[Tuner] Evaluating candidate " << param_name << " = " << val << "...\n";
            MatchPlayer p1{"Baseline", 2, std::chrono::milliseconds(20), ""};
            MatchPlayer p2{"Candidate", 2, std::chrono::milliseconds(20), ""};
            auto res = TournamentManager::run_match(p1, p2, 2);
            // Returns net score: (wins + 0.5 * draws) / total_games
            double score = (res.player2_wins + 0.5 * res.draws) / res.total_games;
            return score - 0.5; // return delta against standard 50% expectations
        };

        TuningManager::get().run_hill_climbing(params, iterations, evaluator);
        std::cout << "Tuning finished. Saving optimized weights to 'optimized_weights.txt'...\n";
        TuningManager::get().save_weights("optimized_weights.txt");
    } 
    else if (cmd == "explain") {
        if (argc < 3) {
            std::cerr << "Error: explain requires a move in algebraic notation (e.g. e2e4).\n";
            return 1;
        }
        std::string move_str = argv[2];
        Board board;
        if (argc >= 4) {
            std::string fen = argv[3];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        std::vector<Move> legal_moves = MoveGenerator::generate_legal_moves(board);
        Move target_move;
        bool found = false;
        for (const auto& m : legal_moves) {
            if (m.to_string() == move_str) {
                target_move = m;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Error: move " << move_str << " is not legal in this position.\n";
            return 1;
        }
        auto explanation = AnalysisTools::explain_move(board, target_move);
        std::cout << "Explanation for move " << move_str << ":\n";
        for (const auto& r : explanation.reasons) {
            std::cout << "  - " << r << "\n";
        }
    }
    else if (cmd == "analyze") {
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 4;
        Board board;
        if (argc >= 4) {
            std::string fen = argv[3];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        std::cout << "Generating explainable AI analysis report (depth " << depth << ")...\n";
        auto report = AnalysisTools::generate_report(board, depth);
        std::cout << AnalysisTools::report_to_json(report) << "\n";
    }
    else if (cmd == "legalmoves") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        std::vector<Move> legal_moves = MoveGenerator::generate_legal_moves(board);
        for (size_t i = 0; i < legal_moves.size(); ++i) {
            std::cout << legal_moves[i].to_string() << (i + 1 < legal_moves.size() ? " " : "");
        }
        std::cout << "\n";
    }
    else if (cmd == "move") {
        if (argc < 4) {
            std::cerr << "Error: move requires FEN and move string.\n";
            return 1;
        }
        std::string fen = argv[2];
        std::string move_str = argv[3];
        Board board;
        if (!board.load_fen(fen)) {
            std::cerr << "Error: invalid FEN string.\n";
            return 1;
        }
        std::vector<Move> legal_moves = MoveGenerator::generate_legal_moves(board);
        Move target_move;
        bool found = false;
        for (const auto& m : legal_moves) {
            if (m.to_string() == move_str) {
                target_move = m;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Error: move " << move_str << " is not legal in this position.\n";
            return 1;
        }
        board.make_move(target_move);
        std::cout << board.to_fen() << "\n";
    }
    else if (cmd == "validate") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        auto res = chess::validation::Verifier::validate_position(board);
        std::cout << "Validation: " << (res.ok ? "PASS" : "FAIL") << "\n";
        for (const auto& err : res.errors) std::cout << "  [ERROR] " << err << "\n";
        for (const auto& warn : res.warnings) std::cout << "  [WARNING] " << warn << "\n";
        return res.ok ? 0 : 1;
    }
    else if (cmd == "audit_material") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        auto res = chess::validation::Verifier::audit_material(board);
        std::cout << "Material Audit:\n";
        std::cout << "  White Pawns:   " << res.white_pawns << "  Black Pawns:   " << res.black_pawns << "\n";
        std::cout << "  White Knights: " << res.white_knights << "  Black Knights: " << res.black_knights << "\n";
        std::cout << "  White Bishops: " << res.white_bishops << "  Black Bishops: " << res.black_bishops << "\n";
        std::cout << "  White Rooks:   " << res.white_rooks << "  Black Rooks:   " << res.black_rooks << "\n";
        std::cout << "  White Queens:  " << res.white_queens << "  Black Queens:  " << res.black_queens << "\n";
        std::cout << "  White Total:   " << res.white_total << " cp  Black Total:   " << res.black_total << " cp\n";
        std::cout << "  Expected Eval: " << res.expected_eval << " cp\n";
    }
    else if (cmd == "trace_search") {
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 4;
        Board board;
        if (argc >= 4) {
            std::string fen = argv[3];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        chess::validation::Verifier::trace_search(board, depth);
    }
    else if (cmd == "trace_eval") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        chess::validation::Verifier::trace_eval(board);
    }
    else if (cmd == "trace_hash") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        chess::validation::Verifier::trace_hash(board);
    }
    else if (cmd == "trace_movegen") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        chess::validation::Verifier::trace_movegen(board);
    }
    else if (cmd == "trace_tt") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        chess::validation::Verifier::trace_tt(board);
    }
    else if (cmd == "trace_history") {
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 4;
        Board board;
        if (argc >= 4) {
            std::string fen = argv[3];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        chess::validation::Verifier::trace_history(board, depth);
    }
    else if (cmd == "trace_makeundo") {
        Board board;
        if (argc >= 3) {
            std::string fen = argv[2];
            if (!board.load_fen(fen)) {
                std::cerr << "Error: invalid FEN string.\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        chess::validation::Verifier::trace_makeundo(board);
    }
    else if (cmd == "selfcheck") {
        bool ok = chess::validation::Verifier::run_selfcheck();
        return ok ? 0 : 1;
    }
    else if (cmd == "bugtest") {
        std::string id = (argc >= 3) ? argv[2] : "";
        bool ok = chess::validation::Verifier::run_bug_tests(id);
        return ok ? 0 : 1;
    }
    else if (cmd == "validate_engine") {
        std::string sub = (argc >= 3) ? argv[2] : "all";
        EngineValidationReport report;
        bool ok = true;
        if (sub == "all") {
            ok = EngineValidation::run_full_validation_lab();
        } else if (sub == "tactical") {
            ok = EngineValidation::run_tactical_suite(report);
        } else if (sub == "strategic") {
            ok = EngineValidation::run_strategic_suite(report);
        } else if (sub == "endgame") {
            ok = EngineValidation::run_endgame_suite(report);
        } else if (sub == "opening") {
            ok = EngineValidation::run_opening_suite(report);
        } else if (sub == "perft") {
            ok = EngineValidation::run_perft_suite(report);
        } else if (sub == "stability") {
            ok = EngineValidation::run_search_stability_tests(report);
        } else if (sub == "root") {
            std::string arg = (argc >= 4) ? argv[3] : "";
            ok = EngineValidation::run_root_move_analysis(arg);
        } else if (sub == "isolation") {
            std::string arg = (argc >= 4) ? argv[3] : "";
            ok = EngineValidation::run_search_heuristic_isolation(arg);
        } else if (sub == "symmetry") {
            ok = EngineValidation::run_evaluation_symmetry_tests(report);
        } else if (sub == "import") {
            if (argc < 4) {
                std::cerr << "Error: import requires an EPD file path.\n";
                return 1;
            }
            ok = EngineValidation::run_known_test_suite(argv[3]);
        } else if (sub == "stress") {
            int num = (argc >= 4) ? std::stoi(argv[3]) : 500;
            ok = EngineValidation::run_random_stress_testing(num, report);
        } else if (sub == "regression") {
            ok = EngineValidation::run_regression_framework(report);
        } else {
            std::cerr << "Unknown sub-command: " << sub << "\n";
            return 1;
        }
        return ok ? 0 : 1;
    }
    else if (cmd == "match") {
        if (argc < 6) {
            std::cerr << "Error: Usage: chess_benchmarks.exe match [p1] [p2] [games] [depth]\n";
            return 1;
        }
        std::string p1 = argv[2];
        std::string p2 = argv[3];
        int games = std::stoi(argv[4]);
        int depth = std::stoi(argv[5]);

        TournamentConfig config;
        config.type = "self_play";
        config.player_names = { p1, p2 };
        config.tc.mode = TimeControlMode::Depth;
        config.tc.limit = depth;
        config.rounds = games;
        config.alternate_colors = true;

        auto res = RatingLab::run_tournament(config);
        std::cout << "\nMatch Finished:\n";
        for (const auto& r : res.final_ratings) {
            std::cout << "  Player: " << r.second.name 
                      << " | Elo: " << r.second.elo << " +/- " << r.second.elo_error
                      << " | W:" << r.second.wins << " L:" << r.second.losses << " D:" << r.second.draws << "\n";
        }
        RatingDatabase::load_database();
        for (const auto& r : res.final_ratings) {
            RatingDatabase::add_or_update_rating(r.second);
        }
        RatingDatabase::save_database();
        RatingLab::export_to_pgn(res, "match_games.pgn");
        std::cout << "PGN exported to match_games.pgn\n";
    }
    else if (cmd == "arena") {
        if (argc < 6) {
            std::cerr << "Error: Usage: chess_benchmarks.exe arena [rounds] [depth] [p1] [p2] [p3]...\n";
            return 1;
        }
        int rounds = std::stoi(argv[2]);
        int depth = std::stoi(argv[3]);
        std::vector<std::string> players;
        for (int i = 4; i < argc; ++i) {
            players.push_back(argv[i]);
        }

        TournamentConfig config;
        config.type = "round_robin";
        config.player_names = players;
        config.tc.mode = TimeControlMode::Depth;
        config.tc.limit = depth;
        config.rounds = rounds;
        config.alternate_colors = true;

        auto res = RatingLab::run_tournament(config);
        std::cout << "\nArena Finished:\n";
        for (const auto& r : res.final_ratings) {
            std::cout << "  Player: " << r.second.name 
                      << " | Elo: " << r.second.elo << " +/- " << r.second.elo_error
                      << " | W:" << r.second.wins << " L:" << r.second.losses << " D:" << r.second.draws << "\n";
        }
        RatingDatabase::load_database();
        for (const auto& r : res.final_ratings) {
            RatingDatabase::add_or_update_rating(r.second);
        }
        RatingDatabase::save_database();
    }
    else if (cmd == "elo") {
        if (argc < 5) {
            std::cerr << "Error: Usage: chess_benchmarks.exe elo [wins] [losses] [draws]\n";
            return 1;
        }
        int wins = std::stoi(argv[2]);
        int losses = std::stoi(argv[3]);
        int draws = std::stoi(argv[4]);
        double diff = 0.0, err = 0.0;
        EloCalculator::calculate_elo(wins, losses, draws, diff, err);
        std::cout << "Elo Rating Difference: " << std::showpos << diff << " +/- " << err << "\n";
    }
    else if (cmd == "sprt") {
        if (argc < 7) {
            std::cerr << "Error: Usage: chess_benchmarks.exe sprt [wins] [losses] [draws] [elo0] [elo1]\n";
            return 1;
        }
        int wins = std::stoi(argv[2]);
        int losses = std::stoi(argv[3]);
        int draws = std::stoi(argv[4]);
        double elo0 = std::stod(argv[5]);
        double elo1 = std::stod(argv[6]);

        auto res = SPRT::check(wins, losses, draws, elo0, elo1);
        std::cout << "SPRT Result:\n"
                  << "  LLR:    " << res.llr << "\n"
                  << "  Bounds: [" << res.lBound << ", " << res.uBound << "]\n"
                  << "  Status: " << res.status << "\n";
    }
    else if (cmd == "compare") {
        if (argc < 6) {
            std::cerr << "Error: Usage: chess_benchmarks.exe compare [base_weights] [candidate_weights] [games] [depth]\n";
            return 1;
        }
        std::string base = argv[2];
        std::string patch = argv[3];
        int games = std::stoi(argv[4]);
        int depth = std::stoi(argv[5]);

        TimeControl tc;
        tc.mode = TimeControlMode::Depth;
        tc.limit = depth;

        RatingLab::run_patch_comparison(base, patch, games, tc);
    }
    else if (cmd == "ratings" || cmd == "leaderboard") {
        std::string db = (argc >= 3) ? argv[2] : "ratings_db.json";
        if (!RatingDatabase::load_database(db)) {
            std::cout << "No ratings database found at " << db << ". Creating default database.\n";
            RatingRecord r1{"Vanth_Baseline", "3.5", "dev", "default", "default", 1500.0, 0.0, 0, 0, 0, 0, "2026-07-11"};
            RatingDatabase::add_or_update_rating(r1);
            RatingDatabase::save_database(db);
        }
        auto leaderboard = RatingDatabase::get_leaderboard();
        std::cout << "========================================================\n"
                  << "                 VANTH RATING LEADERBOARD               \n"
                  << "========================================================\n"
                  << std::left << std::setw(20) << "Engine Name" 
                  << std::right << std::setw(10) << "Elo" 
                  << std::setw(12) << "Error" 
                  << std::setw(10) << "Games" << "\n"
                  << "--------------------------------------------------------\n";
        for (const auto& r : leaderboard) {
            std::cout << std::left << std::setw(20) << r.name 
                      << std::right << std::setw(10) << std::fixed << std::setprecision(1) << r.elo 
                      << " +/- " << std::setw(5) << r.elo_error 
                      << std::setw(10) << r.games << "\n";
        }
        std::cout << "========================================================\n";
    }
    else if (cmd == "history") {
        if (argc < 3) {
            std::cerr << "Error: Usage: chess_benchmarks.exe history [name] [db_file]\n";
            return 1;
        }
        std::string name = argv[2];
        std::string db = (argc >= 4) ? argv[3] : "ratings_db.json";
        RatingDatabase::load_database(db);
        auto hist = RatingDatabase::get_history(name);
        if (hist.empty()) {
            std::cout << "No history found for " << name << ".\n";
        } else {
            std::cout << "History for " << name << ":\n";
            for (const auto& r : hist) {
               std::cout << "  Date: " << r.date 
                         << " | Elo: " << r.elo 
                         << " | Wins: " << r.wins << " Losses: " << r.losses << " Draws: " << r.draws << "\n";
            }
        }
    }
    else if (cmd == "benchmark") {
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 5;
        std::cout << "Running laboratory benchmarks up to depth " << depth << "...\n";
        auto m = BenchmarkSuite::run_standard(depth);
        std::cout << "\nBenchmark Results Summary:\n"
                  << "  Depth:            " << m.depth << "\n"
                  << "  Nodes:            " << m.nodes << "\n"
                  << "  NPS:              " << m.nps << "\n"
                  << "  Branching Factor: " << std::fixed << std::setprecision(2) << m.branching_factor << "\n"
                  << "  Best Move:        " << m.best_move << "\n"
                  << "  Score:            " << m.evaluation << "\n";
    }
    else if (cmd == "stress") {
        int games = (argc >= 3) ? std::stoi(argv[2]) : 100;
        bool ok = chess::validation::Verifier::run_stress_test(games);
        return ok ? 0 : 1;
    }
    else if (cmd == "search_profile") {
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 5;
        Board board;
        if (argc >= 4) {
            if (!board.load_fen(argv[3])) {
                std::cerr << "Invalid FEN\n";
                return 1;
            }
        } else {
            board = Board::start_position();
        }
        
        g_tt.clear();
        std::cout << "Profiling search at depth " << depth << "...\n";
        auto start = std::chrono::high_resolution_clock::now();
        auto info = find_best_move(board, depth);
        auto end = std::chrono::high_resolution_clock::now();
        long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        double seconds = duration / 1000.0;
        uint64_t nps = (seconds > 0.0) ? static_cast<uint64_t>(info.nodes / seconds) : 0;
        
        std::cout << "========================================================\n"
                  << "                  SEARCH PROFILING REPORT               \n"
                  << "========================================================\n"
                  << "  Nodes:              " << info.nodes << "\n"
                  << "  QNodes:             " << info.q_nodes << "\n"
                  << "  Depth / SelDepth:   " << depth << " / " << depth << "\n"
                  << "  Time (ms):          " << duration << "\n"
                  << "  NPS:                " << nps << "\n"
                  << "  Best Move:          " << info.best_move.to_string() << "\n"
                  << "  Score:              " << info.best_score << "\n"
                  << "--------------------------------------------------------\n"
                  << "  Move Ordering Efficiency:\n"
                  << "    First Move Cutoff % (FHF): ";
        if (info.ordering_lookups > 0) {
            std::cout << std::fixed << std::setprecision(1) << (static_cast<double>(info.fhf) / info.ordering_lookups) * 100.0 << "%\n";
        } else {
            std::cout << "0.0%\n";
        }
        std::cout << "    Avg Cutoff Move Index:   ";
        if (info.cutoff_count_total > 0) {
            std::cout << std::fixed << std::setprecision(2) << (static_cast<double>(info.cutoff_index_sum) / info.cutoff_count_total) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "    Branching Factor:        ";
        if (info.nodes_visited > 0) {
            std::cout << std::fixed << std::setprecision(2) << (static_cast<double>(info.moves_searched_total) / info.nodes_visited) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "    Moves Generated/Node:    ";
        if (info.nodes_visited > 0) {
            std::cout << std::fixed << std::setprecision(2) << (static_cast<double>(info.moves_generated_total) / info.nodes_visited) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "    Moves Searched/Node:     ";
        if (info.nodes_visited > 0) {
            std::cout << std::fixed << std::setprecision(2) << (static_cast<double>(info.moves_searched_total) / info.nodes_visited) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "    Moves Skipped/Node:      ";
        if (info.nodes_visited > 0) {
            std::cout << std::fixed << std::setprecision(2) << (static_cast<double>(info.moves_generated_total - info.moves_searched_total) / info.nodes_visited) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "--------------------------------------------------------\n"
                  << "  Search Heuristics Breakdown:\n"
                  << "    Transposition Table (TT) Move:\n"
                  << "      Searched: " << info.tt_move_searched << ", Cutoffs: " << info.tt_move_cutoffs;
        if (info.tt_move_searched > 0) {
            std::cout << " (" << std::fixed << std::setprecision(1) << (static_cast<double>(info.tt_move_cutoffs) / info.tt_move_searched) * 100.0 << "% hit rate)\n";
        } else {
            std::cout << " (0.0%)\n";
        }
        std::cout << "    Killer Moves:\n"
                  << "      Searched: " << info.killer_searched << ", Cutoffs: " << info.killer_cutoffs;
        if (info.killer_searched > 0) {
            std::cout << " (" << std::fixed << std::setprecision(1) << (static_cast<double>(info.killer_cutoffs) / info.killer_searched) * 100.0 << "% hit rate)\n";
        } else {
            std::cout << " (0.0%)\n";
        }
        std::cout << "    Countermoves:\n"
                  << "      Searched: " << info.countermove_searched << ", Cutoffs: " << info.countermove_cutoffs;
        if (info.countermove_searched > 0) {
            std::cout << " (" << std::fixed << std::setprecision(1) << (static_cast<double>(info.countermove_cutoffs) / info.countermove_searched) * 100.0 << "% hit rate)\n";
        } else {
            std::cout << " (0.0%)\n";
        }
        std::cout << "    History Moves:\n"
                  << "      Searched: " << info.history_searched << ", Cutoffs: " << info.history_cutoffs;
        if (info.history_searched > 0) {
            std::cout << " (" << std::fixed << std::setprecision(1) << (static_cast<double>(info.history_cutoffs) / info.history_searched) * 100.0 << "% hit rate)\n";
        } else {
            std::cout << " (0.0%)\n";
        }
        std::cout << "      Avg History Score: ";
        if (info.history_score_count > 0) {
            std::cout << std::fixed << std::setprecision(1) << (static_cast<double>(info.history_score_sum) / info.history_score_count) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "    Static Exchange Evaluation (SEE):\n"
                  << "      Winning Captures (SEE > 0): Searched: " << info.see_winning_searched << ", Cutoffs: " << info.see_winning_cutoffs;
        if (info.see_winning_searched > 0) {
            std::cout << " (" << std::fixed << std::setprecision(1) << (static_cast<double>(info.see_winning_cutoffs) / info.see_winning_searched) * 100.0 << "%)\n";
        } else {
            std::cout << " (0.0%)\n";
        }
        std::cout << "      Neutral Captures (SEE == 0): Searched: " << info.see_neutral_searched << ", Cutoffs: " << info.see_neutral_cutoffs;
        if (info.see_neutral_searched > 0) {
            std::cout << " (" << std::fixed << std::setprecision(1) << (static_cast<double>(info.see_neutral_cutoffs) / info.see_neutral_searched) * 100.0 << "%)\n";
        } else {
            std::cout << " (0.0%)\n";
        }
        std::cout << "      Losing Captures (SEE < 0):  Searched: " << info.see_losing_searched << ", Cutoffs: " << info.see_losing_cutoffs;
        if (info.see_losing_searched > 0) {
            std::cout << " (" << std::fixed << std::setprecision(1) << (static_cast<double>(info.see_losing_cutoffs) / info.see_losing_searched) * 100.0 << "%)\n";
        } else {
            std::cout << " (0.0%)\n";
        }
        std::cout << "--------------------------------------------------------\n"
                  << "  Late Move / Reduction Statistics:\n"
                  << "    Late Moves Searched:      " << info.late_moves_searched << "\n"
                  << "    Avg Late Move Position:   ";
        if (info.late_moves_searched > 0) {
            std::cout << std::fixed << std::setprecision(2) << (static_cast<double>(info.late_moves_position_sum) / info.late_moves_searched) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "    LMR Reductions:           " << info.lmr_reductions << "\n"
                  << "    Avg LMR Reduction Depth:  ";
        if (info.lmr_reductions > 0) {
            std::cout << std::fixed << std::setprecision(2) << (static_cast<double>(info.lmr_reduction_depth_sum) / info.lmr_reductions) << "\n";
        } else {
            std::cout << "0.0\n";
        }
        std::cout << "========================================================\n";
    }
    else if (cmd == "benchmark_compare") {
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 5;
        Board board = Board::start_position();
        
        std::cout << "Starting side-by-side benchmark comparison (depth " << depth << ")..." << std::endl;
        
        // 1. Run Baseline (V3.6 optimizations and V3.7 selectivity disabled)
        g_enable_v36_ordering = false;
        g_enable_conthist = false;
        g_enable_countermoves = false;
        g_enable_lmr = false;
        g_enable_nmp = false;
        g_search_config.mdp_enable = false;
        int orig_iid_depth = g_search_config.iid_depth;
        g_search_config.iid_depth = 99;
        
        g_tt.clear();
        
        std::cout << "Running Baseline search..." << std::endl;
        auto start1 = std::chrono::high_resolution_clock::now();
        auto info_base = find_best_move(board, depth);
        auto end1 = std::chrono::high_resolution_clock::now();
        long long duration_base = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
        double seconds_base = duration_base / 1000.0;
        uint64_t nps_base = (seconds_base > 0.0) ? static_cast<uint64_t>(info_base.nodes / seconds_base) : 0;
        
        // 2. Run Optimized (V3.7 advanced selectivity enabled)
        g_enable_v36_ordering = true;
        g_enable_conthist = true;
        g_enable_countermoves = true;
        g_enable_lmr = true;
        g_enable_nmp = true;
        g_search_config.mdp_enable = true;
        g_search_config.iid_depth = orig_iid_depth;
        
        g_tt.clear();
        
        std::cout << "Running Optimized search..." << std::endl;
        auto start2 = std::chrono::high_resolution_clock::now();
        auto info_opt = find_best_move(board, depth);
        auto end2 = std::chrono::high_resolution_clock::now();
        long long duration_opt = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
        double seconds_opt = duration_opt / 1000.0;
        uint64_t nps_opt = (seconds_opt > 0.0) ? static_cast<uint64_t>(info_opt.nodes / seconds_opt) : 0;
        
        // Compute improvements
        double node_reduction = 0.0;
        if (info_base.nodes > 0) {
            node_reduction = (static_cast<double>(info_base.nodes) - info_opt.nodes) / info_base.nodes * 100.0;
        }
        double time_reduction = 0.0;
        if (duration_base > 0) {
            time_reduction = (static_cast<double>(duration_base) - duration_opt) / duration_base * 100.0;
        }
        
        double fhf_base = info_base.ordering_lookups > 0 ? (static_cast<double>(info_base.fhf) / info_base.ordering_lookups) * 100.0 : 0.0;
        double fhf_opt = info_opt.ordering_lookups > 0 ? (static_cast<double>(info_opt.fhf) / info_opt.ordering_lookups) * 100.0 : 0.0;
        
        double cutoff_idx_base = info_base.cutoff_count_total > 0 ? (static_cast<double>(info_base.cutoff_index_sum) / info_base.cutoff_count_total) : 0.0;
        double cutoff_idx_opt = info_opt.cutoff_count_total > 0 ? (static_cast<double>(info_opt.cutoff_index_sum) / info_opt.cutoff_count_total) : 0.0;
        
        double bf_base = depth > 0 ? std::pow(static_cast<double>(info_base.nodes), 1.0 / depth) : 0.0;
        double bf_opt = depth > 0 ? std::pow(static_cast<double>(info_opt.nodes), 1.0 / depth) : 0.0;
        
        double tt_hit_base = info_base.tt_lookups > 0 ? (static_cast<double>(info_base.tt_hits) / info_base.tt_lookups) * 100.0 : 0.0;
        double tt_hit_opt = info_opt.tt_lookups > 0 ? (static_cast<double>(info_opt.tt_hits) / info_opt.tt_lookups) * 100.0 : 0.0;
        
        double lmr_pct_base = info_base.nodes > 0 ? (static_cast<double>(info_base.lmr_usage) / info_base.nodes) * 100.0 : 0.0;
        double lmr_pct_opt = info_opt.nodes > 0 ? (static_cast<double>(info_opt.lmr_usage) / info_opt.nodes) * 100.0 : 0.0;
        
        double nmp_pct_base = info_base.nodes > 0 ? (static_cast<double>(info_base.nmp_usage) / info_base.nodes) * 100.0 : 0.0;
        double nmp_pct_opt = info_opt.nodes > 0 ? (static_cast<double>(info_opt.nmp_usage) / info_opt.nodes) * 100.0 : 0.0;
        
        double iid_pct_base = info_base.nodes > 0 ? (static_cast<double>(info_base.iid_usage) / info_base.nodes) * 100.0 : 0.0;
        double iid_pct_opt = info_opt.nodes > 0 ? (static_cast<double>(info_opt.iid_usage) / info_opt.nodes) * 100.0 : 0.0;
        
        double mdp_pct_base = info_base.nodes > 0 ? (static_cast<double>(info_base.mdp_usage) / info_base.nodes) * 100.0 : 0.0;
        double mdp_pct_opt = info_opt.nodes > 0 ? (static_cast<double>(info_opt.mdp_usage) / info_opt.nodes) * 100.0 : 0.0;

        std::string move_stability = (info_base.best_move == info_opt.best_move) ? "STABLE" : "UNSTABLE";
        
        std::cout << "\n====================================================================\n"
                  << "                     BENCHMARK COMPARISON REPORT                    \n"
                  << "====================================================================\n"
                  << std::left << std::setw(30) << "Metric" 
                  << std::right << std::setw(15) << "Baseline" 
                  << std::setw(15) << "Optimized" 
                  << std::setw(15) << "Change %" << "\n"
                  << "--------------------------------------------------------------------\n"
                  << std::left << std::setw(30) << "Nodes Searched" 
                  << std::right << std::setw(15) << info_base.nodes 
                  << std::setw(15) << info_opt.nodes 
                  << std::setw(14) << std::showpos << std::fixed << std::setprecision(1) << -node_reduction << "%" << "\n"
                  << std::left << std::setw(30) << "Time (ms)" 
                  << std::right << std::setw(15) << duration_base 
                  << std::setw(15) << duration_opt 
                  << std::setw(14) << -time_reduction << "%" << "\n"
                  << std::noshowpos
                  << std::left << std::setw(30) << "NPS" 
                  << std::right << std::setw(15) << nps_base 
                  << std::setw(15) << nps_opt 
                  << std::setw(14) << std::fixed << std::setprecision(1) << (nps_base > 0 ? (static_cast<double>(nps_opt) - nps_base) / nps_base * 100.0 : 0.0) << "%" << "\n"
                  << std::left << std::setw(30) << "Branching Factor" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << bf_base 
                  << std::setw(15) << bf_opt 
                  << std::setw(14) << (bf_base > 0 ? (bf_opt - bf_base) / bf_base * 100.0 : 0.0) << "%" << "\n"
                  << std::left << std::setw(30) << "TT Hit Rate" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(1) << tt_hit_base << "%" 
                  << std::setw(14) << tt_hit_opt << "%" 
                  << std::setw(14) << std::showpos << (tt_hit_opt - tt_hit_base) << "%" << "\n"
                  << std::left << std::setw(30) << "LMR %" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << lmr_pct_base << "%" 
                  << std::setw(14) << lmr_pct_opt << "%" 
                  << std::setw(14) << std::showpos << (lmr_pct_opt - lmr_pct_base) << "%" << "\n"
                  << std::left << std::setw(30) << "NMP %" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << nmp_pct_base << "%" 
                  << std::setw(14) << nmp_pct_opt << "%" 
                  << std::setw(14) << std::showpos << (nmp_pct_opt - nmp_pct_base) << "%" << "\n"
                  << std::left << std::setw(30) << "IID %" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << iid_pct_base << "%" 
                  << std::setw(14) << iid_pct_opt << "%" 
                  << std::setw(14) << std::showpos << (iid_pct_opt - iid_pct_base) << "%" << "\n"
                  << std::left << std::setw(30) << "MDP %" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << mdp_pct_base << "%" 
                  << std::setw(14) << mdp_pct_opt << "%" 
                  << std::setw(14) << std::showpos << (mdp_pct_opt - mdp_pct_base) << "%" << "\n"
                  << std::left << std::setw(30) << "First Move Cutoff %" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(1) << fhf_base 
                  << std::setw(15) << fhf_opt 
                  << std::setw(14) << std::showpos << (fhf_opt - fhf_base) << "%" << "\n"
                  << std::left << std::setw(30) << "Avg Cutoff Move Index" 
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << cutoff_idx_base 
                  << std::setw(15) << cutoff_idx_opt 
                  << std::setw(14) << (cutoff_idx_opt - cutoff_idx_base) << "\n"
                  << std::noshowpos
                  << std::left << std::setw(30) << "Best Move" 
                  << std::right << std::setw(15) << info_base.best_move.to_string() 
                  << std::setw(15) << info_opt.best_move.to_string() 
                  << std::setw(15) << move_stability << "\n"
                  << std::left << std::setw(30) << "Evaluation Score" 
                  << std::right << std::setw(15) << info_base.best_score 
                  << std::setw(15) << info_opt.best_score 
                  << std::setw(15) << "-" << "\n"
                  << "--------------------------------------------------------------------\n"
                  << "  Net Search Node Reduction: " << std::fixed << std::setprecision(1) << node_reduction << "%\n"
                  << "====================================================================\n" << std::endl;

        // Export JSON Heatmap Telemetry (Module 7 & 14)
        std::ofstream json_file("search_heatmap.json");
        if (json_file.is_open()) {
            json_file << "{\n"
                      << "  \"depth_distribution\": {\n"
                      << "    \"target_depth\": " << depth << ",\n"
                      << "    \"selective_depth\": " << depth << "\n"
                      << "  },\n"
                      << "  \"reductions\": {\n"
                      << "    \"lmr_usage\": " << info_opt.lmr_usage << ",\n"
                      << "    \"lmr_reductions\": " << info_opt.lmr_reductions << ",\n"
                      << "    \"total_reduction_depth\": " << info_opt.total_reduction_depth << "\n"
                      << "  },\n"
                      << "  \"extensions\": {\n"
                      << "    \"total_extension_depth\": " << info_opt.total_extension_depth << "\n"
                      << "  },\n"
                      << "  \"tt_usage\": {\n"
                      << "    \"tt_lookups\": " << info_opt.tt_lookups << ",\n"
                      << "    \"tt_hits\": " << info_opt.tt_hits << ",\n"
                      << "    \"tt_exact_hits\": " << info_opt.tt_exact_hits << ",\n"
                      << "    \"tt_lower_hits\": " << info_opt.tt_lower_hits << ",\n"
                      << "    \"tt_upper_hits\": " << info_opt.tt_upper_hits << "\n"
                      << "  },\n"
                      << "  \"cutoffs\": {\n"
                      << "    \"cutoffs\": " << info_opt.cutoffs << ",\n"
                      << "    \"fhf\": " << info_opt.fhf << ",\n"
                      << "    \"fhf_percentage\": " << fhf_opt << "\n"
                      << "  },\n"
                      << "  \"node_types\": {\n"
                      << "    \"pv_nodes\": " << info_opt.pv_nodes_count << ",\n"
                      << "    \"cut_nodes\": " << info_opt.cut_nodes_count << ",\n"
                      << "    \"all_nodes\": " << info_opt.all_nodes_count << "\n"
                      << "  },\n"
                      << "  \"selectivity\": {\n"
                      << "    \"nmp_usage\": " << info_opt.nmp_usage << ",\n"
                      << "    \"iid_usage\": " << info_opt.iid_usage << ",\n"
                      << "    \"mdp_usage\": " << info_opt.mdp_usage << ",\n"
                      << "    \"mdp_prunings\": " << info_opt.mdp_prunings << ",\n"
                      << "    \"iid_calls\": " << info_opt.iid_calls << ",\n"
                      << "    \"iid_successes\": " << info_opt.iid_successes << "\n"
                      << "  }\n"
                      << "}\n";
            json_file.close();
            std::cout << "Exported search heatmap JSON to search_heatmap.json\n";
        }
    }
    else {
        print_usage();
        return 1;
    }

    return 0;
}
