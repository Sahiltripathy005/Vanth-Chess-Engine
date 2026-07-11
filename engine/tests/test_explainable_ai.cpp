#include <gtest/gtest.h>
#include "chess/board/board.hpp"
#include "chess/validation/analysis.hpp"
#include "chess/moves/move.hpp"
#include "chess/moves/move_generator.hpp"
#include "chess/pieces/square.hpp"
#include "chess/pieces/piece.hpp"
#include <string>
#include <vector>
#include <algorithm>

using namespace chess::board;
using namespace chess::validation;
using namespace chess::moves;
using namespace chess::pieces;

TEST(ExplainableAITest, EvaluationBreakdownMobility) {
    Board board = Board::start_position();
    auto breakdown = AnalysisTools::get_evaluation_breakdown(board);

    // Verify mobility is populated (should not be zero, or at least exists)
    // and that the total score equals the sum of the components
    int sum = breakdown.material + breakdown.pst + breakdown.pawn_structure + 
              breakdown.mobility + breakdown.piece_activity + breakdown.king_safety + 
              breakdown.space + breakdown.threats + breakdown.endgame + breakdown.tempo;
    
    EXPECT_EQ(breakdown.total, sum);
}

TEST(ExplainableAITest, ExplainMove) {
    Board board = Board::start_position();
    
    // Test pawn push e2e4
    Move e2e4(*Square::from_string("e2"), *Square::from_string("e4"), MoveType::Normal);
    auto exp = AnalysisTools::explain_move(board, e2e4);

    EXPECT_EQ(exp.move, "e2e4");
    EXPECT_FALSE(exp.reasons.empty());

    // Should include central presence / control reasons
    bool controls_center = false;
    for (const auto& r : exp.reasons) {
        if (r.find("central") != std::string::npos || r.find("center") != std::string::npos) {
            controls_center = true;
        }
    }
    EXPECT_TRUE(controls_center);
}

TEST(ExplainableAITest, DetectTacticalThemes) {
    // 1. Setup a pin: White Rook on d1, Black Rook on d5, Black King on d8
    // Black Rook is pinned to the King on d8.
    Board board;
    ASSERT_TRUE(board.load_fen("3k4/8/8/3r4/8/8/8/3R2K1 w - - 0 1"));

    auto themes = AnalysisTools::detect_tactical_themes(board);
    EXPECT_TRUE(themes.pin);
    EXPECT_FALSE(themes.descriptions.empty());

    // Verify description contains "pinned"
    bool contains_pinned = false;
    for (const auto& desc : themes.descriptions) {
        if (desc.find("pinned") != std::string::npos || desc.find("Pin") != std::string::npos) {
            contains_pinned = true;
        }
    }
    EXPECT_TRUE(contains_pinned);
}

TEST(ExplainableAITest, PositionClassification) {
    // Start position should be opening
    Board board = Board::start_position();
    auto classif = AnalysisTools::classify_position(board);

    EXPECT_TRUE(classif.opening);
    EXPECT_FALSE(classif.endgame);
    EXPECT_TRUE(std::find(classif.labels.begin(), classif.labels.end(), "Opening") != classif.labels.end());

    // Setup an endgame position
    Board endgame_board;
    ASSERT_TRUE(endgame_board.load_fen("k7/8/8/8/8/8/8/K7 w - - 0 1"));
    auto classif_endgame = AnalysisTools::classify_position(endgame_board);

    EXPECT_TRUE(classif_endgame.endgame);
    EXPECT_FALSE(classif_endgame.opening);
    EXPECT_TRUE(std::find(classif_endgame.labels.begin(), classif_endgame.labels.end(), "Endgame") != classif_endgame.labels.end());
}

TEST(ExplainableAITest, GenerateReportAndJSON) {
    Board board = Board::start_position();
    auto report = AnalysisTools::generate_report(board, 3);

    EXPECT_EQ(report.fen, board.to_fen());
    EXPECT_FALSE(report.best_move.empty());
    EXPECT_FALSE(report.explanation.reasons.empty());

    // Convert to JSON and verify key JSON tokens
    std::string json = AnalysisTools::report_to_json(report);
    EXPECT_TRUE(json.find("\"fen\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"best_move\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"breakdown\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"alternatives\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"tactical_themes\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"classifications\"") != std::string::npos);
}

TEST(ExplainableAITest, BlunderAnalysis) {
    // White King on g1, White Queen on e1, Black Rook on e8
    // If White plays Qf1, it is a blunder (evaluation drops as black can win the Queen)
    Board board;
    ASSERT_TRUE(board.load_fen("4r1k1/8/8/8/8/8/8/4Q1K1 w - - 0 1"));

    // played move is Qf1
    Move qf1(*Square::from_string("e1"), *Square::from_string("f1"), MoveType::Normal);
    auto blunder = AnalysisTools::analyze_blunder(board, qf1, 4);

    EXPECT_TRUE(blunder.is_blunder);
    EXPECT_GE(blunder.score_drop, 150);
    EXPECT_FALSE(blunder.explanation.empty());
}
