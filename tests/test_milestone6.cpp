#include <gtest/gtest.h>
#include "chess/board/board.hpp"
#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"

using namespace chess::pieces;
using namespace chess::moves;
using namespace chess::board;
using namespace chess::search;

TEST(Milestone6Test, AdvancedPawnStructure) {
    Board board;
    // Connected passed pawns vs isolated pawn
    // White: connected passed pawns on e5, d5
    // Black: isolated pawn on a7
    ASSERT_TRUE(board.load_fen("k7/p7/8/3PP3/8/8/8/K7 w - - 0 1"));
    int score = evaluate_pawn_structure(board);
    // Should favor White
    EXPECT_GT(score, 40);

    // Rule of the square (Endgame passed pawn race)
    // White passed pawn on a5, Black King on h8 (too far to catch it)
    // Black has no non-king pieces
    board = Board();
    ASSERT_TRUE(board.load_fen("7k/8/8/P7/8/8/8/K7 w - - 0 1"));
    int race_score = evaluate_pawn_structure(board);
    // Should give a huge bonus since Black King cannot catch the pawn
    EXPECT_GT(race_score, 200);
}

TEST(Milestone6Test, PieceActivityAndOutposts) {
    Board board;
    // Knight on an outpost (e.g. d5, defended by pawn on e4, no black pawns can attack it)
    ASSERT_TRUE(board.load_fen("k7/8/3p4/3N4/4P3/8/8/K7 w - - 0 1"));
    int phase = get_game_phase(board);
    int score = evaluate_piece_activity(board, phase);
    // Should favor White knight significantly
    EXPECT_GT(score, 30);

    // Good vs Bad Bishop
    // White bishop on light squares, White pawns on dark squares (Good bishop)
    // Black bishop on dark squares, Black pawns on dark squares (Bad bishop)
    board = Board();
    ASSERT_TRUE(board.load_fen("k7/1p6/p7/2b5/8/3P4/PP6/K2B4 w - - 0 1"));
    int GoodBadScore = evaluate_piece_activity(board, phase);
    // Black has 0 bad pawns, White has 2 bad pawns, so Black has the better bishop in this FEN.
    EXPECT_LT(GoodBadScore, 0);

    // Rook on 7th rank vs Rook on home rank
    board = Board();
    ASSERT_TRUE(board.load_fen("k7/4R3/8/8/8/8/8/K4r2 w - - 0 1"));
    int rook_score = evaluate_piece_activity(board, phase);
    // White rook on 7th rank should be highly active
    EXPECT_GT(rook_score, 10);
}

TEST(Milestone6Test, ExtendedKingSafety) {
    Board board;
    // Safe King behind a solid pawn shield (Black)
    // vs Uncastled King (White)
    ASSERT_TRUE(board.load_fen("rnbq1rk1/ppppppbp/5np1/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1"));
    int safety = evaluate_king_safety(board, 240);
    // Should favor Black (negative score) due to Black being castled
    EXPECT_LT(safety, 0);
}

TEST(Milestone6Test, SpaceAndControl) {
    Board board;
    // White has space advantage in center d4/e4
    ASSERT_TRUE(board.load_fen("rnbqkbnr/ppp1pppp/8/3p4/3PP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 2"));
    int space_score = evaluate_space(board);
    EXPECT_GT(space_score, 0);
}

TEST(Milestone6Test, ThreatDetection) {
    Board board;
    // Fork opportunity / hanging pieces
    // Black queen is hanging, attacked by White bishop
    ASSERT_TRUE(board.load_fen("k7/8/3q4/8/5B2/8/8/K7 w - - 0 1"));
    int threats_score = evaluate_threats(board);
    // White threats should be highly positive
    EXPECT_GT(threats_score, 40);
}

TEST(Milestone6Test, EndgameHeuristics) {
    Board board;
    // Active White King in center (d4) close to pawn (e3) vs inactive Black King (a8)
    ASSERT_TRUE(board.load_fen("k7/8/8/8/3K4/4P3/8/8 w - - 0 1"));
    int endgame_score = evaluate_endgame(board, 10);
    EXPECT_GT(endgame_score, 10);
}
