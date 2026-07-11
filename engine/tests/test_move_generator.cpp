#include <gtest/gtest.h>
#include "chess/board/board.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>

using namespace chess::pieces;
using namespace chess::moves;
using namespace chess::board;

TEST(MoveGeneratorTest, AttackDetection) {
    Board board;
    // Set up a board with a white rook at e4, white knight at c3, black pawn at f5
    ASSERT_TRUE(board.load_fen("8/8/8/5p2/4R3/2N5/8/8 w - - 0 1"));

    // Check rook attacks: e4 attacks e1-e8 and a4-h4 (unless blocked)
    EXPECT_TRUE(MoveGenerator::is_square_attacked(board, Square::from_string("e8").value(), Color::White));
    EXPECT_TRUE(MoveGenerator::is_square_attacked(board, Square::from_string("a4").value(), Color::White));
    EXPECT_FALSE(MoveGenerator::is_square_attacked(board, Square::from_string("f3").value(), Color::White));

    // Check knight attacks: c3 attacks a2, b1, d1, e2, e4, d5, b5, a4
    EXPECT_TRUE(MoveGenerator::is_square_attacked(board, Square::from_string("d5").value(), Color::White));
    EXPECT_TRUE(MoveGenerator::is_square_attacked(board, Square::from_string("a4").value(), Color::White));
    EXPECT_FALSE(MoveGenerator::is_square_attacked(board, Square::from_string("c5").value(), Color::White));

    // Check black pawn attacks: f5 attacks e4 and g4
    EXPECT_TRUE(MoveGenerator::is_square_attacked(board, Square::from_string("e4").value(), Color::Black));
    EXPECT_TRUE(MoveGenerator::is_square_attacked(board, Square::from_string("g4").value(), Color::Black));
    EXPECT_FALSE(MoveGenerator::is_square_attacked(board, Square::from_string("f4").value(), Color::Black));
}

TEST(MoveGeneratorTest, PawnMoves) {
    Board board;
    // Pawn at e2 (start), pawn at d7 (start)
    ASSERT_TRUE(board.load_fen("8/3p4/8/8/8/8/4P3/8 w - - 0 1"));

    // White turn
    auto moves = MoveGenerator::generate_legal_moves(board);
    // e2 should have e3, e4 (2 moves)
    EXPECT_EQ(moves.size(), 2);
    
    // Change to Black turn
    board.set_active_color(Color::Black);
    moves = MoveGenerator::generate_legal_moves(board);
    // d7 should have d6, d5 (2 moves)
    EXPECT_EQ(moves.size(), 2);
}

TEST(MoveGeneratorTest, PinnedPieces) {
    Board board;
    // White king at e1, White knight at e2 (pinned by Black rook at e8)
    ASSERT_TRUE(board.load_fen("4r3/8/8/8/8/8/4N3/4K3 w - - 0 1"));

    auto moves = MoveGenerator::generate_legal_moves(board);
    // Knight at e2 cannot move because it is pinned
    for (const auto& move : moves) {
        EXPECT_NE(board.piece_at(move.from()).type(), PieceType::Knight);
    }
}

TEST(MoveGeneratorTest, CastlingLegalityChecks) {
    {
        Board board;
        // White castling: king at e1, rook at h1. Black rook at f8 attacks f1
        ASSERT_TRUE(board.load_fen("4kr2/8/8/8/8/8/8/4K2R w K - 0 1"));
        auto moves = MoveGenerator::generate_legal_moves(board);
        // Castling (e1g1) should be illegal because f1 is attacked by Black rook
        bool can_castle = false;
        for (const auto& move : moves) {
            if (move.type() == MoveType::Castling) {
                can_castle = true;
            }
        }
        EXPECT_FALSE(can_castle);
    }

    {
        Board board;
        // King is in check (White king at e1 checked by Black rook at e8)
        ASSERT_TRUE(board.load_fen("4r3/8/8/8/8/8/8/4K2R w K - 0 1"));
        auto moves = MoveGenerator::generate_legal_moves(board);
        // Castling (e1g1) should be illegal because King is in check
        bool can_castle = false;
        for (const auto& move : moves) {
            if (move.type() == MoveType::Castling) {
                can_castle = true;
            }
        }
        EXPECT_FALSE(can_castle);
    }
}

TEST(MoveGeneratorTest, PerftValues) {
    Board board = Board::start_position();

    EXPECT_EQ(MoveGenerator::perft(board, 1), 20ULL);
    EXPECT_EQ(MoveGenerator::perft(board, 2), 400ULL);
    EXPECT_EQ(MoveGenerator::perft(board, 3), 8902ULL);
    EXPECT_EQ(MoveGenerator::perft(board, 4), 197281ULL);
    
    // Depth 5 test is run separately to avoid long debug builds or can be included
    // Since we want to verify it, we can run it here as it takes ~1 second in release and few seconds in debug.
    EXPECT_EQ(MoveGenerator::perft(board, 5), 4865609ULL);
}
