#include <gtest/gtest.h>
#include "chess/uci/uci.hpp"
#include "chess/uci/option_manager.hpp"
#include "chess/board/board.hpp"
#include "chess/search/time_manager.hpp"
#include "chess/validation/rating_lab.hpp"
#include <sstream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cmath>

using namespace chess::uci;
using namespace chess::board;
using namespace chess::validation;

TEST(Milestone8Test, OptionRegistryAndSet) {
    // Retrieve the OptionManager singleton
    auto& mgr = OptionManager::get();

    // Verify default standard options exist
    EXPECT_EQ(mgr.get_spin("Hash"), 16);
    EXPECT_EQ(mgr.get_spin("Threads"), 1);
    EXPECT_EQ(mgr.get_spin("MultiPV"), 1);
    EXPECT_TRUE(mgr.get_check("OwnBook"));
    EXPECT_FALSE(mgr.get_check("Ponder"));

    // Modify option values and verify change
    EXPECT_TRUE(mgr.set_option("Hash", "64"));
    EXPECT_EQ(mgr.get_spin("Hash"), 64);

    EXPECT_TRUE(mgr.set_option("MultiPV", "4"));
    EXPECT_EQ(mgr.get_spin("MultiPV"), 4);

    EXPECT_TRUE(mgr.set_option("OwnBook", "false"));
    EXPECT_FALSE(mgr.get_check("OwnBook"));

    // Restore defaults for future runs
    mgr.set_option("Hash", "16");
    mgr.set_option("MultiPV", "1");
    mgr.set_option("OwnBook", "true");
}

TEST(Milestone8Test, PositionFENLoading) {
    UCIEngine uci_engine;
    
    // Load standard starting position with some moves
    // 1. e4 e5 2. Nf3 Nc6
    uci_engine.handle_position("startpos moves e2e4 e7e5 g1f3 b8c6");
}

TEST(Milestone8Test, GoCommandParsing) {
    chess::search::TimeManager tm;
    tm.start(1000, 300000, 8000, 40, false);

    EXPECT_EQ(tm.hard_limit_ms(), 1000);
    EXPECT_EQ(tm.soft_limit_ms(), 1000);
}

TEST(Milestone8Test, EloCalculatorMath) {
    // Expected score for identical rating is 0.5
    EXPECT_NEAR(EloCalculator::expected_score(0.0), 0.5, 0.001);

    // Expected score for +400 rating diff is ~0.909
    EXPECT_NEAR(EloCalculator::expected_score(400.0), 1.0 / (1.0 + std::pow(10.0, -1.0)), 0.001);

    // Expected score for -400 rating diff is ~0.0909
    EXPECT_NEAR(EloCalculator::expected_score(-400.0), 1.0 / (1.0 + std::pow(10.0, 1.0)), 0.001);

    // Score of 0.5 corresponds to 0 difference
    EXPECT_NEAR(EloCalculator::rating_difference(0.5), 0.0, 0.001);

    // Elo difference and error bounds
    double elo_diff = 0.0;
    double elo_error = 0.0;
    EloCalculator::calculate_elo(10, 10, 20, elo_diff, elo_error);
    EXPECT_NEAR(elo_diff, 0.0, 0.1);
    EXPECT_GT(elo_error, 0.0);

    // 10 wins, 0 losses, 0 draws should be high score and positive difference
    EloCalculator::calculate_elo(10, 0, 0, elo_diff, elo_error);
    EXPECT_GT(elo_diff, 200.0);
}

TEST(Milestone8Test, SPRTLogic) {
    // 100 wins, 0 losses should accept H1
    auto res_accept = SPRT::check(100, 0, 0, 0.0, 5.0);
    EXPECT_EQ(res_accept.status, "ACCEPT");

    // 0 wins, 100 losses should reject H1
    auto res_reject = SPRT::check(0, 100, 0, 0.0, 5.0);
    EXPECT_EQ(res_reject.status, "REJECT");

    // Equal wins and losses should continue testing
    auto res_continue = SPRT::check(10, 10, 10, 0.0, 5.0);
    EXPECT_EQ(res_continue.status, "CONTINUE");
}

TEST(Milestone8Test, RatingDatabaseSerialization) {
    std::string temp_db = "temp_test_db.json";
    
    // Clear database
    RatingDatabase::clear();

    RatingRecord rec1{"Vanth_v1", "1.0", "hash1", "s_cfg", "e_cfg", 1550.0, 12.5, 100, 45, 15, 40, "2026-07-11"};
    RatingRecord rec2{"Vanth_v2", "2.0", "hash2", "s_cfg", "e_cfg", 1620.0, 15.0, 120, 60, 20, 40, "2026-07-11"};

    RatingDatabase::add_or_update_rating(rec1);
    RatingDatabase::add_or_update_rating(rec2);

    // Save database
    EXPECT_TRUE(RatingDatabase::save_database(temp_db));

    // Clear and reload
    RatingDatabase::clear();
    EXPECT_TRUE(RatingDatabase::load_database(temp_db));

    auto leaderboard = RatingDatabase::get_leaderboard();
    ASSERT_EQ(leaderboard.size(), 2);
    
    // First in leaderboard should be Vanth_v2 (highest Elo)
    EXPECT_EQ(leaderboard[0].name, "Vanth_v2");
    EXPECT_NEAR(leaderboard[0].elo, 1620.0, 0.1);
    EXPECT_EQ(leaderboard[1].name, "Vanth_v1");

    // Cleanup temp file
    std::filesystem::remove(temp_db);
}

TEST(Milestone8Test, TournamentSelfPlayAndExports) {
    TournamentConfig config;
    config.type = "self_play";
    config.player_names = { "Vanth_TestA", "Vanth_TestB" };
    config.tc.mode = TimeControlMode::Depth;
    config.tc.limit = 1; // Super fast depth 1 search
    config.rounds = 1;
    config.alternate_colors = true;

    auto res = RatingLab::run_tournament(config);
    
    // Since alternate_colors is true and 1 round is played, it should run 2 games (A vs B, B vs A)
    EXPECT_EQ(res.total_games, 2);
    EXPECT_GT(res.avg_game_length, 0.0);
    EXPECT_EQ(res.games.size(), 2);

    // Test Export
    std::string test_pgn = "temp_test_games.pgn";
    std::string test_csv = "temp_test_games.csv";
    
    RatingLab::export_to_pgn(res, test_pgn);
    RatingLab::export_to_csv(res, test_csv);
    std::string json_str = RatingLab::export_to_json(res);

    EXPECT_TRUE(std::filesystem::exists(test_pgn));
    EXPECT_TRUE(std::filesystem::exists(test_csv));
    EXPECT_FALSE(json_str.empty());

    // Cleanup
    std::filesystem::remove(test_pgn);
    std::filesystem::remove(test_csv);
}
