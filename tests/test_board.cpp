#include <gtest/gtest.h>
#include "chess/board/board.hpp"

using namespace chess::pieces;
using namespace chess::moves;
using namespace chess::board;

TEST(BoardTest, DefaultInitialization) {
    Board board;
    // Every square should be empty
    for (uint8_t i = 0; i < 64; ++i) {
        EXPECT_TRUE(board.piece_at(Square(i)).is_none());
    }
    EXPECT_EQ(board.active_color(), Color::White);
    EXPECT_EQ(board.castling_rights(), CastlingRights::kAll);
    EXPECT_TRUE(board.en_passant_square().is_none());
    EXPECT_EQ(board.halfmove_clock(), 0);
    EXPECT_EQ(board.fullmove_clock(), 1);
}

TEST(BoardTest, StartPositionFEN) {
    Board board = Board::start_position();
    
    // Check key pieces
    EXPECT_EQ(board.piece_at(Square(0)), Piece(PieceType::Rook, Color::White));   // a1
    EXPECT_EQ(board.piece_at(Square(4)), Piece(PieceType::King, Color::White));   // e1
    EXPECT_EQ(board.piece_at(Square(7)), Piece(PieceType::Rook, Color::White));   // h1
    EXPECT_EQ(board.piece_at(Square(60)), Piece(PieceType::King, Color::Black));  // e8
    
    EXPECT_EQ(board.active_color(), Color::White);
    EXPECT_EQ(board.castling_rights(), CastlingRights::kAll);
    EXPECT_TRUE(board.en_passant_square().is_none());
    EXPECT_EQ(board.halfmove_clock(), 0);
    EXPECT_EQ(board.fullmove_clock(), 1);

    // Export back to FEN should match start pos
    EXPECT_EQ(board.to_fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

TEST(BoardTest, CustomFEN) {
    Board board;
    // FEN representing a position after 1. e4 e5 2. Nf3 Nc6
    std::string fen = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3";
    ASSERT_TRUE(board.load_fen(fen));

    EXPECT_EQ(board.piece_at(*Square::from_string("e4")), Piece(PieceType::Pawn, Color::White));
    EXPECT_EQ(board.piece_at(*Square::from_string("e5")), Piece(PieceType::Pawn, Color::Black));
    EXPECT_EQ(board.piece_at(*Square::from_string("f3")), Piece(PieceType::Knight, Color::White));
    EXPECT_EQ(board.piece_at(*Square::from_string("c6")), Piece(PieceType::Knight, Color::Black));

    EXPECT_EQ(board.active_color(), Color::White);
    EXPECT_EQ(board.castling_rights(), CastlingRights::kAll);
    EXPECT_TRUE(board.en_passant_square().is_none());
    EXPECT_EQ(board.halfmove_clock(), 2);
    EXPECT_EQ(board.fullmove_clock(), 3);

    EXPECT_EQ(board.to_fen(), fen);
}

TEST(BoardTest, MakeUndoNormalMove) {
    Board board = Board::start_position();
    Square e2(12);
    Square e3(20);
    Move move(e2, e3, MoveType::Normal);

    // Make move
    BoardState prev = board.make_move(move);

    EXPECT_TRUE(board.piece_at(e2).is_none());
    EXPECT_EQ(board.piece_at(e3), Piece(PieceType::Pawn, Color::White));
    EXPECT_EQ(board.active_color(), Color::Black);
    EXPECT_EQ(board.halfmove_clock(), 0); // pawn move resets clock
    EXPECT_EQ(board.fullmove_clock(), 1); // incremented on black move, not white

    // Undo move
    board.undo_move(move, prev);

    EXPECT_EQ(board.piece_at(e2), Piece(PieceType::Pawn, Color::White));
    EXPECT_TRUE(board.piece_at(e3).is_none());
    EXPECT_EQ(board.active_color(), Color::White);
    EXPECT_EQ(board.halfmove_clock(), 0);
    EXPECT_EQ(board.fullmove_clock(), 1);
}

TEST(BoardTest, MakeUndoDoublePawnPush) {
    Board board = Board::start_position();
    Square e2(12);
    Square e4(28);
    Move move(e2, e4, MoveType::DoublePawnPush);

    BoardState prev = board.make_move(move);

    EXPECT_EQ(board.en_passant_square(), Square(20)); // e3 is EP square

    board.undo_move(move, prev);

    EXPECT_TRUE(board.en_passant_square().is_none());
}

TEST(BoardTest, MakeUndoCastling) {
    // Setup a board where castling is possible immediately (for test simplification)
    Board board;
    // White king at e1, White rook at h1. Other pieces empty
    ASSERT_TRUE(board.load_fen("4k3/8/8/8/8/8/8/4K2R w K - 0 1"));

    Square e1(4);
    Square g1(6);
    Move castling_move(e1, g1, MoveType::Castling);

    BoardState prev = board.make_move(castling_move);

    // King should be at g1, Rook should be at f1
    EXPECT_EQ(board.piece_at(g1), Piece(PieceType::King, Color::White));
    EXPECT_EQ(board.piece_at(Square(5)), Piece(PieceType::Rook, Color::White)); // f1
    EXPECT_TRUE(board.piece_at(e1).is_none());
    EXPECT_TRUE(board.piece_at(Square(7)).is_none()); // h1
    EXPECT_EQ(board.castling_rights() & CastlingRights::kWhiteKingside, 0); // lost white kingside

    board.undo_move(castling_move, prev);

    EXPECT_EQ(board.piece_at(e1), Piece(PieceType::King, Color::White));
    EXPECT_EQ(board.piece_at(Square(7)), Piece(PieceType::Rook, Color::White));
    EXPECT_TRUE(board.piece_at(g1).is_none());
    EXPECT_TRUE(board.piece_at(Square(5)).is_none());
    EXPECT_EQ(board.castling_rights() & CastlingRights::kWhiteKingside, CastlingRights::kWhiteKingside);
}

TEST(BoardTest, MakeUndoEnPassant) {
    Board board;
    // Position with black pawn on e4, white pawn double pushed to d4 (so ep square is d3)
    ASSERT_TRUE(board.load_fen("8/8/8/8/3pP3/8/8/8 b - e3 0 1"));

    Square d4(*Square::from_string("d4"));
    Square e3(*Square::from_string("e3"));
    Move ep_move(d4, e3, MoveType::EnPassant);

    BoardState prev = board.make_move(ep_move);

    // The captured pawn was at e4
    EXPECT_TRUE(board.piece_at(*Square::from_string("e4")).is_none());
    EXPECT_EQ(board.piece_at(e3), Piece(PieceType::Pawn, Color::Black));

    board.undo_move(ep_move, prev);

    EXPECT_EQ(board.piece_at(*Square::from_string("e4")), Piece(PieceType::Pawn, Color::White));
    EXPECT_EQ(board.piece_at(d4), Piece(PieceType::Pawn, Color::Black));
    EXPECT_TRUE(board.piece_at(e3).is_none());
}

TEST(BoardTest, MakeUndoPromotion) {
    Board board;
    // White pawn on a7 ready to promote on a8
    ASSERT_TRUE(board.load_fen("1k6/P7/8/8/8/8/8/K7 w - - 0 1"));

    Square a7(*Square::from_string("a7"));
    Square a8(*Square::from_string("a8"));
    Move promo_move(a7, a8, MoveType::Promotion, PieceType::Queen);

    BoardState prev = board.make_move(promo_move);

    EXPECT_TRUE(board.piece_at(a7).is_none());
    EXPECT_EQ(board.piece_at(a8), Piece(PieceType::Queen, Color::White));

    board.undo_move(promo_move, prev);

    EXPECT_EQ(board.piece_at(a7), Piece(PieceType::Pawn, Color::White));
    EXPECT_TRUE(board.piece_at(a8).is_none());
}
