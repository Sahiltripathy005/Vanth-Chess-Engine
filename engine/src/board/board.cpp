#include "chess/board/board.hpp"
#include "chess/board/zobrist.hpp"
#include <sstream>

namespace chess::board {

Board::Board() {
    // Standard initialization: clear all squares
    pieces_.fill(pieces::Piece());
    state_.zobrist_hash = compute_zobrist_hash();
}

Board Board::start_position() {
    Board board;
    board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    return board;
}

pieces::Piece Board::piece_at(pieces::Square square) const {
    if (!square.is_valid()) {
        return pieces::Piece();
    }
    return pieces_[square.index()];
}

void Board::set_piece(pieces::Square square, pieces::Piece piece) {
    if (square.is_valid()) {
        pieces_[square.index()] = piece;
    }
}

moves::BoardState Board::make_move(const moves::Move& move) {
    moves::BoardState old_state = state_;

    pieces::Piece moving_piece = piece_at(move.from());
    uint64_t hash = state_.zobrist_hash;

    // XOR out moving piece from source
    hash ^= Zobrist::piece_key(state_.active_color, moving_piece.type(), move.from());

    // 1. Identify captured piece and record it
    if (move.type() == moves::MoveType::EnPassant) {
        pieces::Square capture_sq(move.to().file(), move.from().rank());
        old_state.captured_piece = piece_at(capture_sq);
        // XOR out captured pawn
        hash ^= Zobrist::piece_key(pieces::opposite_color(state_.active_color), pieces::PieceType::Pawn, capture_sq);
        set_piece(capture_sq, pieces::Piece()); // Remove captured pawn
    } else {
        old_state.captured_piece = piece_at(move.to());
        if (old_state.captured_piece.is_present()) {
            // XOR out captured piece
            hash ^= Zobrist::piece_key(pieces::opposite_color(state_.active_color), old_state.captured_piece.type(), move.to());
        }
    }

    // 2. Clear from square
    set_piece(move.from(), pieces::Piece());

    // 3. Set to square
    if (move.type() == moves::MoveType::Promotion) {
        set_piece(move.to(), pieces::Piece(move.promotion(), state_.active_color));
        // XOR in promoted piece
        hash ^= Zobrist::piece_key(state_.active_color, move.promotion(), move.to());
    } else {
        set_piece(move.to(), moving_piece);
        // XOR in moving piece
        hash ^= Zobrist::piece_key(state_.active_color, moving_piece.type(), move.to());
    }

    // 4. Handle Castling rook movement
    if (move.type() == moves::MoveType::Castling) {
        uint8_t to_idx = move.to().index();
        if (to_idx == 6) { // g1 (White Kingside)
            pieces::Square r_from(7), r_to(5);
            set_piece(r_to, piece_at(r_from));
            set_piece(r_from, pieces::Piece());
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_from);
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_to);
        } else if (to_idx == 2) { // c1 (White Queenside)
            pieces::Square r_from(0), r_to(3);
            set_piece(r_to, piece_at(r_from));
            set_piece(r_from, pieces::Piece());
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_from);
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_to);
        } else if (to_idx == 62) { // g8 (Black Kingside)
            pieces::Square r_from(63), r_to(61);
            set_piece(r_to, piece_at(r_from));
            set_piece(r_from, pieces::Piece());
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_from);
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_to);
        } else if (to_idx == 58) { // c8 (Black Queenside)
            pieces::Square r_from(56), r_to(59);
            set_piece(r_to, piece_at(r_from));
            set_piece(r_from, pieces::Piece());
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_from);
            hash ^= Zobrist::piece_key(state_.active_color, pieces::PieceType::Rook, r_to);
        }
    }

    // XOR out old castling rights and old en passant
    hash ^= Zobrist::castling_key(state_.castling_rights);
    if (state_.en_passant_square.is_valid()) {
        hash ^= Zobrist::en_passant_key(state_.en_passant_square);
    }

    // 5. Update en passant target square
    if (move.type() == moves::MoveType::DoublePawnPush) {
        uint8_t ep_index = (move.from().index() + move.to().index()) / 2;
        state_.en_passant_square = pieces::Square(ep_index);
    } else {
        state_.en_passant_square = pieces::Square::None();
    }

    // 6. Update Castling Rights
    update_castling_rights_for_square(move.from());
    update_castling_rights_for_square(move.to());

    // XOR in new castling rights and new en passant
    hash ^= Zobrist::castling_key(state_.castling_rights);
    if (state_.en_passant_square.is_valid()) {
        hash ^= Zobrist::en_passant_key(state_.en_passant_square);
    }

    // 7. Update Clocks
    if (moving_piece.type() == pieces::PieceType::Pawn || old_state.captured_piece.is_present()) {
        state_.halfmove_clock = 0;
    } else {
        state_.halfmove_clock++;
    }

    if (state_.active_color == pieces::Color::Black) {
        state_.fullmove_clock++;
    }

    // Toggle active color in hash
    hash ^= Zobrist::side_key();

    // 8. Toggle Active Color
    state_.active_color = pieces::opposite_color(state_.active_color);

    // Save hash
    state_.zobrist_hash = hash;

    return old_state;
}

void Board::undo_move(const moves::Move& move, const moves::BoardState& previous_state) {
    pieces::Color mover_color = previous_state.active_color;

    // 1. Restore the moving piece to the source square
    if (move.type() == moves::MoveType::Promotion) {
        set_piece(move.from(), pieces::Piece(pieces::PieceType::Pawn, mover_color));
    } else {
        set_piece(move.from(), piece_at(move.to()));
    }

    // 2. Restore the target square or the captured piece
    if (move.type() == moves::MoveType::EnPassant) {
        set_piece(move.to(), pieces::Piece());
        pieces::Square capture_sq(move.to().file(), move.from().rank());
        set_piece(capture_sq, previous_state.captured_piece);
    } else {
        set_piece(move.to(), previous_state.captured_piece);
    }

    // 3. Undo Castling Rook movement
    if (move.type() == moves::MoveType::Castling) {
        uint8_t to_idx = move.to().index();
        if (to_idx == 6) { // g1
            set_piece(pieces::Square(7), piece_at(pieces::Square(5)));
            set_piece(pieces::Square(5), pieces::Piece());
        } else if (to_idx == 2) { // c1
            set_piece(pieces::Square(0), piece_at(pieces::Square(3)));
            set_piece(pieces::Square(3), pieces::Piece());
        } else if (to_idx == 62) { // g8
            set_piece(pieces::Square(63), piece_at(pieces::Square(61)));
            set_piece(pieces::Square(61), pieces::Piece());
        } else if (to_idx == 58) { // c8
            set_piece(pieces::Square(56), piece_at(pieces::Square(59)));
            set_piece(pieces::Square(59), pieces::Piece());
        }
    }

    // 4. Restore the saved board state
    state_ = previous_state;
}

moves::BoardState Board::make_null_move() {
    moves::BoardState old_state = state_;

    // XOR out old en passant from hash
    if (state_.en_passant_square.is_valid()) {
        state_.zobrist_hash ^= Zobrist::en_passant_key(state_.en_passant_square);
    }

    // Clear en passant square
    state_.en_passant_square = pieces::Square::None();

    // Toggle active color in hash
    state_.zobrist_hash ^= Zobrist::side_key();

    // Toggle active color
    state_.active_color = pieces::opposite_color(state_.active_color);

    // Update clocks
    state_.halfmove_clock++;
    if (state_.active_color == pieces::Color::Black) {
        state_.fullmove_clock++;
    }

    return old_state;
}

void Board::undo_null_move(const moves::BoardState& previous_state) {
    state_ = previous_state;
}

void Board::update_castling_rights_for_square(pieces::Square square) {
    if (!square.is_valid()) return;
    uint8_t index = square.index();
    switch (index) {
        case 4: // e1
            state_.castling_rights &= ~(moves::CastlingRights::kWhiteKingside | moves::CastlingRights::kWhiteQueenside);
            break;
        case 60: // e8
            state_.castling_rights &= ~(moves::CastlingRights::kBlackKingside | moves::CastlingRights::kBlackQueenside);
            break;
        case 7: // h1
            state_.castling_rights &= ~moves::CastlingRights::kWhiteKingside;
            break;
        case 0: // a1
            state_.castling_rights &= ~moves::CastlingRights::kWhiteQueenside;
            break;
        case 63: // h8
            state_.castling_rights &= ~moves::CastlingRights::kBlackKingside;
            break;
        case 56: // a8
            state_.castling_rights &= ~moves::CastlingRights::kBlackQueenside;
            break;
    }
}

std::string Board::to_string() const {
    std::string result;
    for (int rank = 7; rank >= 0; --rank) {
        result += std::to_string(rank + 1) + " ";
        for (int file = 0; file < 8; ++file) {
            pieces::Square sq(static_cast<uint8_t>(file), static_cast<uint8_t>(rank));
            result += piece_at(sq).to_char();
            result += ' ';
        }
        result += '\n';
    }
    result += "  a b c d e f g h\n";
    return result;
}

std::ostream& operator<<(std::ostream& os, const Board& board) {
    os << board.to_string();
    return os;
}

uint64_t Board::compute_zobrist_hash() const {
    uint64_t hash = 0;
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = piece_at(sq);
        if (p.is_present()) {
            hash ^= Zobrist::piece_key(p.color(), p.type(), sq);
        }
    }
    if (state_.active_color == pieces::Color::Black) {
        hash ^= Zobrist::side_key();
    }
    hash ^= Zobrist::castling_key(state_.castling_rights);
    if (state_.en_passant_square.is_valid()) {
        hash ^= Zobrist::en_passant_key(state_.en_passant_square);
    }
    return hash;
}

} // namespace chess::board
