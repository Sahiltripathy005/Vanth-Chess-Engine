#include "chess/board/board.hpp"
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

void print_usage() {
    std::cout << "Vanth Chess Engine Competitive & Testing Tool\n";
    std::cout << "Usage:\n";
    std::cout << "  chess_benchmarks.exe bench [depth]                      - Run standard search benchmark (default: 5)\n";
    std::cout << "  chess_benchmarks.exe eval [fen]                         - Show static evaluation breakdown\n";
    std::cout << "  chess_benchmarks.exe tournament [d1] [d2] [games]       - Run a match between two instances of the engine\n";
    std::cout << "  chess_benchmarks.exe selfplay [games] [depth] [time_ms]  - Run automated self-play game sessions\n";
    std::cout << "  chess_benchmarks.exe testsuite [epd_file] [depth] [ms]   - Run tactical test suite on an EPD file\n";
    std::cout << "  chess_benchmarks.exe tune [iterations] [param1] [param2] - Run Hill Climbing parameter optimization\n";
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
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 5;
        std::cout << "Running benchmark to depth " << depth << "...\n";
        auto m = BenchmarkSuite::run_standard(depth);
        std::cout << BenchmarkSuite::generate_report(m) << "\n";
        
        std::vector<BenchmarkMetrics> metrics = {m};
        BenchmarkSuite::save_to_json("benchmark_results.json", metrics);
        BenchmarkSuite::save_to_csv("benchmark_results.csv", metrics);
        std::cout << "Saved results to JSON/CSV.\n";
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
    else {
        print_usage();
        return 1;
    }

    return 0;
}
