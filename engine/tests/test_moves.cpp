#include <gtest/gtest.h>
#include "chess/moves/move.hpp"
#include "chess/moves/board_state.hpp"

using namespace chess::pieces;
using namespace chess::moves;

TEST(MovesTest, MoveConstructorAndToString) {
    Move default_move;
    EXPECT_EQ(default_move.to_string(), "0000");

    Move normal_move(Square(12), Square(28)); // e2 to e4
    EXPECT_EQ(normal_move.from(), Square(12));
    EXPECT_EQ(normal_move.to(), Square(28));
    EXPECT_EQ(normal_move.type(), MoveType::Normal);
    EXPECT_EQ(normal_move.promotion(), PieceType::None);
    EXPECT_EQ(normal_move.to_string(), "e2e4");

    Move promotion_move(Square(52), Square(60), MoveType::Promotion, PieceType::Queen);
    EXPECT_EQ(promotion_move.from(), Square(52));
    EXPECT_EQ(promotion_move.to(), Square(60));
    EXPECT_EQ(promotion_move.type(), MoveType::Promotion);
    EXPECT_EQ(promotion_move.promotion(), PieceType::Queen);
    EXPECT_EQ(promotion_move.to_string(), "e7e8q");
}

TEST(MovesTest, MoveEquality) {
    Move m1(Square(12), Square(28));
    Move m2(Square(12), Square(28));
    Move m3(Square(12), Square(20));

    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 == m3);
    EXPECT_TRUE(m1 != m3);
}

TEST(MovesTest, BoardStateDefaultValues) {
    BoardState state;
    EXPECT_EQ(state.castling_rights, CastlingRights::kAll);
    EXPECT_TRUE(state.en_passant_square.is_none());
    EXPECT_EQ(state.halfmove_clock, 0);
    EXPECT_EQ(state.fullmove_clock, 1);
    EXPECT_TRUE(state.captured_piece.is_none());
    EXPECT_EQ(state.active_color, Color::White);
}
