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
    else {
        print_usage();
        return 1;
    }

    return 0;
}
