#include <gtest/gtest.h>
#include "chess/board/board.hpp"
#include "chess/search/search.hpp"
#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/search/see.hpp"
#include "chess/moves/move_generator.hpp"

using namespace chess::pieces;
using namespace chess::moves;
using namespace chess::board;
using namespace chess::search;

TEST(SearchTest, StaticEvaluation) {
    Board board = Board::start_position();
    // Starting position: White has tempo bonus + slight positional features
    EXPECT_NEAR(evaluate(board), kTempoBonus, 10);

    // If White has an extra Queen
    ASSERT_TRUE(board.load_fen("k7/8/8/8/8/8/8/K3Q3 w - - 0 1"));
    EXPECT_GT(evaluate(board), 800);

    // If Black has an extra Rook
    ASSERT_TRUE(board.load_fen("k3r3/8/8/8/8/8/8/K7 w - - 0 1"));
    EXPECT_LT(evaluate(board), -400);
}

TEST(SearchTest, StalemateDetection) {
    Board board;
    // Stalemate position (Black to move, no legal moves, not in check)
    ASSERT_TRUE(board.load_fen("k7/P7/1Q6/8/8/8/8/4K3 b - - 0 1"));
    
    auto info = find_best_move(board, 1);
    EXPECT_EQ(info.best_score, kDrawScore);
    EXPECT_EQ(info.best_move, Move());
}

TEST(SearchTest, DrawBy50MoveRule) {
    Board board = Board::start_position();
    // Make 100 half-moves (50 full moves) without pawn push or capture
    // Setting halfmove clock directly to 100 via FEN
    ASSERT_TRUE(board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 100 51"));
    
    auto info = find_best_move(board, 1);
    EXPECT_EQ(info.best_score, kDrawScore);
}

TEST(SearchTest, ForcedCapture) {
    Board board;
    // Bishop on c3 attacks Black Queen on d4. Capturing it is the best move.
    ASSERT_TRUE(board.load_fen("7k/8/8/8/3q4/2B5/8/4K3 w - - 0 1"));

    auto info = find_best_move(board, 2);
    EXPECT_EQ(info.best_move.from().to_string(), "c3");
    EXPECT_EQ(info.best_move.to().to_string(), "d4");
}

TEST(SearchTest, MateInOne) {
    Board board;
    // White Queen at h5, Pawn at g6, ready to checkmate Black King on h8 via Qh7#
    ASSERT_TRUE(board.load_fen("7k/8/6P1/7Q/8/8/8/4K3 w - - 0 1"));

    // Depth 2 search is needed to detect checkmate at ply 1
    auto info = find_best_move(board, 2);
    EXPECT_EQ(info.best_move.from().to_string(), "h5");
    EXPECT_EQ(info.best_move.to().to_string(), "h7");
}

TEST(SearchTest, MateInTwo) {
    Board board;
    // White Queen at f3, White Rook at g1. Black King at h8, pawn at g7.
    // Qf8+ Kh7, then Qg7# (Mate in 2)
    ASSERT_TRUE(board.load_fen("7k/6p1/8/8/8/5Q2/8/4K1R1 w - - 0 1"));

    // Depth 4 search is required to detect mate in 2
    auto info = find_best_move(board, 4);
    EXPECT_EQ(info.best_score, kMateScore - 3);
    std::string move_str = info.best_move.from().to_string() + info.best_move.to().to_string();
    EXPECT_TRUE(move_str == "f3c3" || move_str == "f3f7" || move_str == "f3f8");
}

TEST(SearchTest, StatisticsCollection) {
    Board board = Board::start_position();
    auto info = find_best_move(board, 2);

    EXPECT_GT(info.nodes, 0ULL);
    EXPECT_GT(info.leaf_nodes, 0ULL);
    EXPECT_EQ(info.max_depth, 2);
    EXPECT_GE(info.time.count(), 0);
    EXPECT_GT(info.tt_lookups, 0ULL);
}

TEST(SearchTest, ZobristHashing) {
    Board board = Board::start_position();
    uint64_t start_hash = board.zobrist_hash();
    EXPECT_NE(start_hash, 0ULL);

    // Verify compute matches incremental hash
    EXPECT_EQ(start_hash, board.compute_zobrist_hash());

    // Make a move and check incremental hash
    std::vector<Move> moves = MoveGenerator::generate_legal_moves(board);
    ASSERT_FALSE(moves.empty());
    Move m = moves[0];
    
    auto state = board.make_move(m);
    EXPECT_EQ(board.zobrist_hash(), board.compute_zobrist_hash());
    EXPECT_NE(board.zobrist_hash(), start_hash);

    board.undo_move(m, state);
    EXPECT_EQ(board.zobrist_hash(), start_hash);
}

TEST(SearchTest, ZobristUniqueness) {
    Board board1;
    Board board2;
    board1.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board2.load_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_NE(board1.zobrist_hash(), board2.zobrist_hash());
}

TEST(SearchTest, GamePhaseInterpolation) {
    Board board = Board::start_position();
    // Starting position phase should be maximum (256)
    EXPECT_EQ(get_game_phase(board), 256);

    // Board with only kings and pawns should be 0 (endgame)
    board.load_fen("k7/8/8/8/8/8/8/K7 w - - 0 1");
    EXPECT_EQ(get_game_phase(board), 0);
}

TEST(SearchTest, PieceSquareTables) {
    Board board1;
    Board board2;
    // White king active in center vs corner in middlegame
    board1.load_fen("8/8/8/3K4/8/8/8/k7 w - - 0 1");
    board2.load_fen("8/8/8/8/8/8/8/kK6 w - - 0 1");
    
    // King is penalized in center for MG, rewarded in EG. Let's test the evaluation function.
    int eval1 = evaluate(board1);
    int eval2 = evaluate(board2);
    EXPECT_NE(eval1, eval2);
}

TEST(SearchTest, BishopPairBonus) {
    Board board1;
    Board board2;
    // board1 has two bishops, board2 has one
    board1.load_fen("k7/8/8/8/8/8/8/K2BB3 w - - 0 1");
    board2.load_fen("k7/8/8/8/8/8/8/K2B4 w - - 0 1");
    
    // The difference should reflect piece value (330) + bishop pair bonus (40) with tolerance
    int diff = evaluate(board1) - evaluate(board2);
    EXPECT_NEAR(diff, 370, 30);
}

TEST(SearchTest, RookActivity) {
    Board board1;
    Board board2;
    // board1 has rook on open file, board2 has rook on semi-open file (blocked by enemy pawn on e7)
    board1.load_fen("k7/8/8/8/8/8/8/K3R3 w - - 0 1");
    board2.load_fen("k7/4p3/8/8/8/8/8/K3R3 w - - 0 1");
    
    int act1 = evaluate_rook_activity(board1);
    int act2 = evaluate_rook_activity(board2);
    EXPECT_GT(act1, act2);
}

TEST(SearchTest, TranspositionTable) {
    TranspositionTable tt(1);
    uint64_t test_key = 0x123456789ABCDEF0ULL;
    Move test_move = Move(Square(12), Square(28), MoveType::DoublePawnPush);
    
    // Store exact score
    tt.store(test_key, 4, 1, 150, TTBound::Exact, test_move);
    
    int score = 0;
    Move best_move;
    // Probe at same depth
    bool found = tt.probe(test_key, 4, 1, -1000, 1000, score, best_move);
    EXPECT_TRUE(found);
    EXPECT_EQ(score, 150);
    EXPECT_EQ(best_move, test_move);
}

TEST(SearchTest, RepetitionDetection) {
    Board board = Board::start_position();
    TranspositionTable tt(1);
    TimeManager tm;
    SearchContext context{tt, tm};
    SearchInfo info;
    
    // Setup history to simulate that the starting position has occurred twice already
    context.board_history.push_back(board.zobrist_hash());
    context.board_history.push_back(0x1111ULL); // Dummy hash
    context.board_history.push_back(board.zobrist_hash()); // Second occurrence
    
    // Now, calling negamax at depth 1, ply 1 with the same starting position hash should return kDrawScore (0)
    int score = negamax(board, 1, 1, -kInfiniteScore, kInfiniteScore, info, context);
    EXPECT_EQ(score, kDrawScore);
}

TEST(SearchTest, SEEEvaluation) {
    Board board;
    // 1. Favorable capture: undefended pawn
    // White Rook on e1, Black Pawn on e4
    ASSERT_TRUE(board.load_fen("4k3/8/8/8/4p3/8/8/4R1K1 w - - 0 1"));
    Move favorable_move(Square::from_string("e1").value(), Square::from_string("e4").value(), MoveType::Normal);
    EXPECT_EQ(see(board, favorable_move), 100);

    // 2. Unfavorable capture: defended pawn
    // White Rook on e1, Black Pawn on e4 defended by Black pawn on d5
    ASSERT_TRUE(board.load_fen("4k3/8/8/3p4/4p3/8/8/4R1K1 w - - 0 1"));
    Move unfavorable_move(Square::from_string("e1").value(), Square::from_string("e4").value(), MoveType::Normal);
    EXPECT_EQ(see(board, unfavorable_move), 100 - 500); // Net loss of 400

    // 3. Favorable exchange of high value piece
    // White Rook on e1, Black Queen on e4 defended by Black Pawn on d5
    ASSERT_TRUE(board.load_fen("4k3/8/8/3p4/4q3/8/8/4R1K1 w - - 0 1"));
    Move high_val_move(Square::from_string("e1").value(), Square::from_string("e4").value(), MoveType::Normal);
    EXPECT_EQ(see(board, high_val_move), 900 - 500); // Net gain of 400
}

