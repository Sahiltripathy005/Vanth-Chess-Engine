#include <gtest/gtest.h>
#include <fstream>
#include "chess/board/board.hpp"
#include "chess/search/evaluation.hpp"
#include "chess/validation/tuning.hpp"
#include "chess/validation/tournament.hpp"
#include "chess/validation/test_suite.hpp"
#include "chess/validation/benchmark.hpp"
#include "chess/validation/opening_book.hpp"
#include "chess/validation/endgame_framework.hpp"
#include "chess/validation/analysis.hpp"

using namespace chess::board;
using namespace chess::moves;
using namespace chess::validation;
using namespace chess::pieces;
using namespace chess::search;

TEST(Milestone7Test, ParameterLoadingAndSaving) {
    auto& tm = TuningManager::get();
    tm.reset_to_defaults();

    // Verify initial values
    int orig_val = 0;
    ASSERT_TRUE(tm.get_parameter("bishop_pair_bonus", orig_val));
    EXPECT_EQ(orig_val, 40);

    // Modify parameter
    EXPECT_TRUE(tm.set_parameter("bishop_pair_bonus", 55));
    int new_val = 0;
    EXPECT_TRUE(tm.get_parameter("bishop_pair_bonus", new_val));
    EXPECT_EQ(new_val, 55);

    // Save to temp file
    std::string temp_filepath = "temp_weights.txt";
    EXPECT_TRUE(tm.save_weights(temp_filepath));

    // Reset and load
    tm.reset_to_defaults();
    EXPECT_TRUE(tm.get_parameter("bishop_pair_bonus", new_val));
    EXPECT_EQ(new_val, 40); // Reset to default

    EXPECT_TRUE(tm.load_weights(temp_filepath));
    EXPECT_TRUE(tm.get_parameter("bishop_pair_bonus", new_val));
    EXPECT_EQ(new_val, 55); // Restored from file

    // Clean up
    tm.reset_to_defaults();
    std::remove(temp_filepath.c_str());
}

TEST(Milestone7Test, OpeningBookFramework) {
    OpeningBook book;

    // Load simple text book from string lines
    // We create a temporary text file
    std::string temp_book = "temp_book.txt";
    std::ofstream out(temp_book);
    out << "e2e4 e7e5 g1f3 b8c6\n";
    out << "d2d4 d7d5\n";
    out.close();

    EXPECT_TRUE(book.load_text_book(temp_book));

    Board board = Board::start_position();
    auto candidates = book.get_candidates(board);
    // Should have e2e4 and d2d4 as candidate moves
    EXPECT_FALSE(candidates.empty());

    Move chosen = book.select_move(board, true);
    EXPECT_NE(chosen.from(), Square::None());

    std::remove(temp_book.c_str());
}

TEST(Milestone7Test, BenchmarkFramework) {
    // Run depth 1 standard benchmark
    auto m = BenchmarkSuite::run_standard(1);
    EXPECT_GT(m.nodes, 0);
    EXPECT_GE(m.nps, 0);
    EXPECT_FALSE(m.best_move.empty());
    
    // Check report generation
    std::string report = BenchmarkSuite::generate_report(m);
    EXPECT_FALSE(report.empty());
}

TEST(Milestone7Test, TournamentFramework) {
    // Run a small 2-game tournament with shallow depth
    MatchPlayer p1{"P1", 1, std::chrono::milliseconds(5), ""};
    MatchPlayer p2{"P2", 1, std::chrono::milliseconds(5), ""};

    auto results = TournamentManager::run_match(p1, p2, 2);
    EXPECT_EQ(results.total_games, 2);
    EXPECT_GT(results.avg_game_length, 0.0);
    EXPECT_GT(results.avg_nodes_searched, 0);
}

TEST(Milestone7Test, AnalysisTools) {
    Board board = Board::start_position();
    auto breakdown = AnalysisTools::get_evaluation_breakdown(board);

    // Verify consistency
    EXPECT_EQ(breakdown.total, evaluate(board));

    std::string formatted = AnalysisTools::format_evaluation_breakdown(breakdown);
    EXPECT_FALSE(formatted.empty());
}

TEST(Milestone7Test, EndgameFramework) {
    Board board = Board::start_position();
    EXPECT_FALSE(EndgameFramework::is_endgame(board));

    // Generate material signature
    std::string sig = EndgameFramework::get_material_signature(board);
    // Starting position signature: KQQRRBBNNPPPPPPPPvKQQRRBBNNPPPPPPPP
    EXPECT_TRUE(sig.find("v") != std::string::npos);

    // Let's check a basic endgame position: K + P vs K
    Board eg_board;
    ASSERT_TRUE(eg_board.load_fen("8/k7/3P4/8/8/8/K7/8 w - - 0 1"));
    EXPECT_TRUE(EndgameFramework::is_endgame(eg_board));
    EXPECT_EQ(EndgameFramework::get_material_signature(eg_board), "KPvK");
}

TEST(Milestone7Test, EPDTestSuiteRunner) {
    std::string epd = "1k6/p7/8/8/8/8/P7/1K6 w - - bm a2a4; id \"pawn_push\";";
    std::string fen, expected_move, id;
    
    EXPECT_TRUE(TestSuiteRunner::parse_epd_line(epd, fen, expected_move, id));
    EXPECT_EQ(expected_move, "a2a4");
    EXPECT_EQ(id, "pawn_push");

    std::vector<std::string> lines = { epd };
    auto results = TestSuiteRunner::run_list(lines, 1, 100);
    EXPECT_EQ(results.total_positions, 1);
    EXPECT_EQ(results.detail_results[0].id, "pawn_push");
}
