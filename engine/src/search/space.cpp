#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>
#include <cmath>

namespace chess::search {

static bool is_friendly_pawn_attacking(const board::Board& board, pieces::Square sq, pieces::Color color) {
    int pawn_dir = (color == pieces::Color::White) ? 1 : -1;
    // Check files
    int f = sq.file();
    int r = sq.rank() - pawn_dir; // Row where friendly pawn would be to attack sq
    if (r >= 0 && r < 8) {
        if (f > 0) {
            pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(f - 1), static_cast<uint8_t>(r)));
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == color) {
                return true;
            }
        }
        if (f < 7) {
            pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(f + 1), static_cast<uint8_t>(r)));
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == color) {
                return true;
            }
        }
    }
    return false;
}

int evaluate_space(const board::Board& board) {
    int white_score = 0;
    int black_score = 0;

    // 1. Space Advantage (ranks 5-8 for White, 1-4 for Black)
    // Count squares not occupied/attacked by enemy pawns, controlled by us
    for (int file = 0; file < 8; ++file) {
        // White Space (ranks 4, 5, 6 - i.e. index 3, 4, 5)
        for (int rank = 4; rank < 7; ++rank) {
            pieces::Square sq(static_cast<uint8_t>(file), static_cast<uint8_t>(rank));
            // Not occupied by Black pawn
            pieces::Piece p = board.piece_at(sq);
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == pieces::Color::Black) {
                continue;
            }
            // Not attacked by Black pawn
            if (is_friendly_pawn_attacking(board, sq, pieces::Color::Black)) {
                continue;
            }
            // Controlled by White (either occupied by White piece or attacked by White)
            if ((p.is_present() && p.color() == pieces::Color::White) || 
                moves::MoveGenerator::is_square_attacked(board, sq, pieces::Color::White)) {
                white_score += 2;
            }
        }

        // Black Space (ranks 2, 3, 4 - i.e. index 1, 2, 3)
        for (int rank = 1; rank < 4; ++rank) {
            pieces::Square sq(static_cast<uint8_t>(file), static_cast<uint8_t>(rank));
            // Not occupied by White pawn
            pieces::Piece p = board.piece_at(sq);
            if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == pieces::Color::White) {
                continue;
            }
            // Not attacked by White pawn
            if (is_friendly_pawn_attacking(board, sq, pieces::Color::White)) {
                continue;
            }
            // Controlled by Black
            if ((p.is_present() && p.color() == pieces::Color::Black) || 
                moves::MoveGenerator::is_square_attacked(board, sq, pieces::Color::Black)) {
                black_score += 2;
            }
        }
    }

    // 2. Central Control (d4, e4, d5, e5)
    pieces::Square inner_center[] = {
        pieces::Square(3, 3), pieces::Square(4, 3), // d4, e4
        pieces::Square(3, 4), pieces::Square(4, 4)  // d5, e5
    };
    for (const auto& sq : inner_center) {
        if (moves::MoveGenerator::is_square_attacked(board, sq, pieces::Color::White)) white_score += 5;
        if (moves::MoveGenerator::is_square_attacked(board, sq, pieces::Color::Black)) black_score += 5;
    }

    // 3. Piece Coordination / Harmony (mutual defense)
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present()) {
            if (moves::MoveGenerator::is_square_attacked(board, sq, p.color())) {
                if (p.color() == pieces::Color::White) white_score += 3;
                else black_score += 3;
            }
        }
    }

    // 4. Development Lead
    int phase = get_game_phase(board);
    if (phase > 120) {
        // Minor pieces still on home row penalty (-15 each)
        // White minor pieces: b1, c1, f1, g1 (indices 1, 2, 5, 6)
        uint8_t white_home[] = {1, 2, 5, 6};
        for (uint8_t idx : white_home) {
            pieces::Piece p = board.piece_at(pieces::Square(idx));
            if (p.is_present() && p.color() == pieces::Color::White && 
                (p.type() == pieces::PieceType::Knight || p.type() == pieces::PieceType::Bishop)) {
                white_score -= 15;
            }
        }
        // Black minor pieces: b8, c8, f8, g8 (indices 57, 58, 61, 62)
        uint8_t black_home[] = {57, 58, 61, 62};
        for (uint8_t idx : black_home) {
            pieces::Piece p = board.piece_at(pieces::Square(idx));
            if (p.is_present() && p.color() == pieces::Color::Black && 
                (p.type() == pieces::PieceType::Knight || p.type() == pieces::PieceType::Bishop)) {
                black_score -= 15;
            }
        }

        // King castling status / rights
        // If not castled and lost castling rights
        // Check if White king is not on g1 or c1 and has no castling rights
        pieces::Square w_king_sq = pieces::Square::None();
        pieces::Square b_king_sq = pieces::Square::None();
        for (uint8_t i = 0; i < 64; ++i) {
            pieces::Piece p = board.piece_at(pieces::Square(i));
            if (p.type() == pieces::PieceType::King) {
                if (p.color() == pieces::Color::White) w_king_sq = pieces::Square(i);
                else b_king_sq = pieces::Square(i);
            }
        }
        if (!w_king_sq.is_none()) {
            if (w_king_sq.index() != 2 && w_king_sq.index() != 6) { // not castled
                if (!(board.state().castling_rights & moves::CastlingRights::kWhiteKingside) && 
                    !(board.state().castling_rights & moves::CastlingRights::kWhiteQueenside)) {
                    white_score -= 25;
                }
            }
        }
        if (!b_king_sq.is_none()) {
            if (b_king_sq.index() != 58 && b_king_sq.index() != 62) {
                if (!(board.state().castling_rights & moves::CastlingRights::kBlackKingside) && 
                    !(board.state().castling_rights & moves::CastlingRights::kBlackQueenside)) {
                    black_score -= 25;
                }
            }
        }
    }

    return white_score - black_score;
}

} // namespace chess::search
