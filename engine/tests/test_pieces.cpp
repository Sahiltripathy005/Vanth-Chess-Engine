#include <gtest/gtest.h>
#include "chess/pieces/color.hpp"
#include "chess/pieces/piece_type.hpp"
#include "chess/pieces/piece.hpp"
#include "chess/pieces/square.hpp"

using namespace chess::pieces;

TEST(PiecesTest, ColorHelpers) {
    EXPECT_EQ(opposite_color(Color::White), Color::Black);
    EXPECT_EQ(opposite_color(Color::Black), Color::White);
    EXPECT_EQ(opposite_color(Color::None), Color::None);

    EXPECT_EQ(to_string(Color::White), "White");
    EXPECT_EQ(to_string(Color::Black), "Black");
    EXPECT_EQ(to_string(Color::None), "None");
}

TEST(PiecesTest, PieceTypeHelpers) {
    EXPECT_EQ(to_string(PieceType::Pawn), "Pawn");
    EXPECT_EQ(to_string(PieceType::King), "King");
    EXPECT_EQ(to_string(PieceType::None), "None");

    EXPECT_EQ(to_char(PieceType::Pawn), 'p');
    EXPECT_EQ(to_char(PieceType::King), 'k');
    EXPECT_EQ(to_char(PieceType::None), ' ');
}

TEST(PiecesTest, PieceClass) {
    Piece p1;
    EXPECT_TRUE(p1.is_none());
    EXPECT_FALSE(p1.is_present());
    EXPECT_EQ(p1.to_char(), '.');

    Piece p2(PieceType::Pawn, Color::White);
    EXPECT_FALSE(p2.is_none());
    EXPECT_TRUE(p2.is_present());
    EXPECT_EQ(p2.to_char(), 'P');

    Piece p3(PieceType::Knight, Color::Black);
    EXPECT_EQ(p3.to_char(), 'n');

    auto parsed_opt = Piece::from_char('Q');
    ASSERT_TRUE(parsed_opt.has_value());
    EXPECT_EQ(parsed_opt->type(), PieceType::Queen);
    EXPECT_EQ(parsed_opt->color(), Color::White);

    auto parsed_opt2 = Piece::from_char('r');
    ASSERT_TRUE(parsed_opt2.has_value());
    EXPECT_EQ(parsed_opt2->type(), PieceType::Rook);
    EXPECT_EQ(parsed_opt2->color(), Color::Black);

    auto invalid_opt = Piece::from_char('x');
    EXPECT_FALSE(invalid_opt.has_value());
}

TEST(PiecesTest, SquareClass) {
    Square sq1;
    EXPECT_TRUE(sq1.is_none());
    EXPECT_FALSE(sq1.is_valid());
    EXPECT_EQ(sq1.to_string(), "-");

    Square a1(0);
    EXPECT_TRUE(a1.is_valid());
    EXPECT_EQ(a1.file(), 0);
    EXPECT_EQ(a1.rank(), 0);
    EXPECT_EQ(a1.to_string(), "a1");

    Square h8(63);
    EXPECT_TRUE(h8.is_valid());
    EXPECT_EQ(h8.file(), 7);
    EXPECT_EQ(h8.rank(), 7);
    EXPECT_EQ(h8.to_string(), "h8");

    Square e4(4, 3);
    EXPECT_EQ(e4.index(), 28);
    EXPECT_EQ(e4.to_string(), "e4");

    auto sq_opt = Square::from_string("e4");
    ASSERT_TRUE(sq_opt.has_value());
    EXPECT_EQ(sq_opt->index(), 28);

    auto sq_invalid = Square::from_string("z9");
    EXPECT_FALSE(sq_invalid.has_value());

    auto sq_short = Square::from_string("e");
    EXPECT_FALSE(sq_short.has_value());
}
