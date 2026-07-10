#include "chess/moves/move_generator.hpp"
#include <utility>

namespace chess::moves {

// Shared sliding move generator helper
static void generate_sliding_moves(
    const board::Board& board,
    pieces::Square sq,
    const std::vector<std::pair<int, int>>& directions,
    std::vector<Move>& moves) 
{
    pieces::Color active_color = board.active_color();
    int start_file = sq.file();
    int start_rank = sq.rank();

    for (const auto& [df, dr] : directions) {
        int f = start_file + df;
        int r = start_rank + dr;
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            pieces::Square target(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
            pieces::Piece piece = board.piece_at(target);
            if (piece.is_none()) {
                moves.emplace_back(sq, target, MoveType::Normal);
            } else {
                if (piece.color() != active_color) {
                    moves.emplace_back(sq, target, MoveType::Normal);
                }
                break; // Blocked by piece
            }
            f += df;
            r += dr;
        }
    }
}

std::vector<Move> MoveGenerator::generate_legal_moves(board::Board& board) {
    std::vector<Move> pseudo_moves = generate_pseudo_legal_moves(board);
    std::vector<Move> legal_moves;
    pieces::Color active_color = board.active_color();

    for (const auto& move : pseudo_moves) {
        // Handle castling validation details (cannot castle in check, cannot pass through check)
        if (move.type() == MoveType::Castling) {
            if (is_in_check(board, active_color)) {
                continue; // Cannot castle out of check
            }
            // Identify intermediate squares
            // White Kingside: f1 (5)
            // White Queenside: d1 (3)
            // Black Kingside: f8 (61)
            // Black Queenside: d8 (59)
            uint8_t to_idx = move.to().index();
            pieces::Square intermediate_sq;
            if (to_idx == 6) {       // g1
                intermediate_sq = pieces::Square(5);
            } else if (to_idx == 2) { // c1
                intermediate_sq = pieces::Square(3);
            } else if (to_idx == 62) { // g8
                intermediate_sq = pieces::Square(61);
            } else if (to_idx == 58) { // c8
                intermediate_sq = pieces::Square(59);
            }

            pieces::Color opponent_color = pieces::opposite_color(active_color);
            if (is_square_attacked(board, intermediate_sq, opponent_color) ||
                is_square_attacked(board, move.to(), opponent_color)) {
                continue; // Cannot pass through or land on attacked squares
            }
        }

        // Test the move
        BoardState state = board.make_move(move);
        if (!is_in_check(board, active_color)) {
            legal_moves.push_back(move);
        }
        board.undo_move(move, state);
    }

    return legal_moves;
}

std::vector<Move> MoveGenerator::generate_pseudo_legal_moves(const board::Board& board) {
    std::vector<Move> moves;
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece piece = board.piece_at(sq);
        if (piece.is_present() && piece.color() == board.active_color()) {
            switch (piece.type()) {
                case pieces::PieceType::Pawn:
                    generate_pawn_moves(board, sq, moves);
                    break;
                case pieces::PieceType::Knight:
                    generate_knight_moves(board, sq, moves);
                    break;
                case pieces::PieceType::Bishop:
                    generate_bishop_moves(board, sq, moves);
                    break;
                case pieces::PieceType::Rook:
                    generate_rook_moves(board, sq, moves);
                    break;
                case pieces::PieceType::Queen:
                    generate_queen_moves(board, sq, moves);
                    break;
                case pieces::PieceType::King:
                    generate_king_moves(board, sq, moves);
                    break;
                default:
                    break;
            }
        }
    }
    return moves;
}

void MoveGenerator::generate_pawn_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves) {
    pieces::Color color = board.active_color();
    int direction = (color == pieces::Color::White) ? 1 : -1;
    int start_rank = (color == pieces::Color::White) ? 1 : 6;
    int promo_rank = (color == pieces::Color::White) ? 7 : 0;

    int file = sq.file();
    int rank = sq.rank();

    // 1. Single push
    int next_rank = rank + direction;
    if (next_rank >= 0 && next_rank < 8) {
        pieces::Square target(static_cast<uint8_t>(file), static_cast<uint8_t>(next_rank));
        if (board.piece_at(target).is_none()) {
            if (next_rank == promo_rank) {
                // Generate all 4 promotions
                moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Queen);
                moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Rook);
                moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Bishop);
                moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Knight);
            } else {
                moves.emplace_back(sq, target, MoveType::Normal);
            }

            // 2. Double push (only if single push was empty and starting rank)
            if (rank == start_rank) {
                int double_rank = rank + 2 * direction;
                pieces::Square double_target(static_cast<uint8_t>(file), static_cast<uint8_t>(double_rank));
                if (board.piece_at(double_target).is_none()) {
                    moves.emplace_back(sq, double_target, MoveType::DoublePawnPush);
                }
            }
        }
    }

    // 3. Captures
    int target_files[2] = { file - 1, file + 1 };
    for (int next_file : target_files) {
        if (next_file >= 0 && next_file < 8) {
            pieces::Square target(static_cast<uint8_t>(next_file), static_cast<uint8_t>(next_rank));
            pieces::Piece target_piece = board.piece_at(target);

            // Normal capture
            if (target_piece.is_present() && target_piece.color() != color) {
                if (next_rank == promo_rank) {
                    moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Queen);
                    moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Rook);
                    moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Bishop);
                    moves.emplace_back(sq, target, MoveType::Promotion, pieces::PieceType::Knight);
                } else {
                    moves.emplace_back(sq, target, MoveType::Normal);
                }
            }

            // En passant capture
            if (target == board.en_passant_square()) {
                moves.emplace_back(sq, target, MoveType::EnPassant);
            }
        }
    }
}

void MoveGenerator::generate_knight_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves) {
    pieces::Color color = board.active_color();
    int file = sq.file();
    int rank = sq.rank();

    const std::pair<int, int> offsets[8] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };

    for (const auto& [df, dr] : offsets) {
        int f = file + df;
        int r = rank + dr;
        if (f >= 0 && f < 8 && r >= 0 && r < 8) {
            pieces::Square target(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
            pieces::Piece target_piece = board.piece_at(target);
            if (target_piece.is_none() || target_piece.color() != color) {
                moves.emplace_back(sq, target, MoveType::Normal);
            }
        }
    }
}

void MoveGenerator::generate_bishop_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves) {
    const std::vector<std::pair<int, int>> directions = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };
    generate_sliding_moves(board, sq, directions, moves);
}

void MoveGenerator::generate_rook_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves) {
    const std::vector<std::pair<int, int>> directions = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };
    generate_sliding_moves(board, sq, directions, moves);
}

void MoveGenerator::generate_queen_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves) {
    const std::vector<std::pair<int, int>> directions = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1},
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };
    generate_sliding_moves(board, sq, directions, moves);
}

void MoveGenerator::generate_king_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves) {
    pieces::Color color = board.active_color();
    int file = sq.file();
    int rank = sq.rank();

    const std::pair<int, int> offsets[8] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
        {0, 1}, {1, -1}, {1, 0}, {1, 1}
    };

    for (const auto& [df, dr] : offsets) {
        int f = file + df;
        int r = rank + dr;
        if (f >= 0 && f < 8 && r >= 0 && r < 8) {
            pieces::Square target(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
            pieces::Piece target_piece = board.piece_at(target);
            if (target_piece.is_none() || target_piece.color() != color) {
                moves.emplace_back(sq, target, MoveType::Normal);
            }
        }
    }

    generate_castling_moves(board, sq, moves);
}

void MoveGenerator::generate_castling_moves(const board::Board& board, pieces::Square sq, std::vector<Move>& moves) {
    pieces::Color color = board.active_color();
    uint8_t rights = board.castling_rights();

    if (color == pieces::Color::White) {
        if (sq.index() == 4) { // e1
            // Kingside castling
            if (rights & CastlingRights::kWhiteKingside) {
                if (board.piece_at(pieces::Square(5)).is_none() && // f1
                    board.piece_at(pieces::Square(6)).is_none()) { // g1
                    moves.emplace_back(sq, pieces::Square(6), MoveType::Castling);
                }
            }
            // Queenside castling
            if (rights & CastlingRights::kWhiteQueenside) {
                if (board.piece_at(pieces::Square(3)).is_none() && // d1
                    board.piece_at(pieces::Square(2)).is_none() && // c1
                    board.piece_at(pieces::Square(1)).is_none()) { // b1
                    moves.emplace_back(sq, pieces::Square(2), MoveType::Castling);
                }
            }
        }
    } else { // Black
        if (sq.index() == 60) { // e8
            // Kingside castling
            if (rights & CastlingRights::kBlackKingside) {
                if (board.piece_at(pieces::Square(61)).is_none() && // f8
                    board.piece_at(pieces::Square(62)).is_none()) { // g8
                    moves.emplace_back(sq, pieces::Square(62), MoveType::Castling);
                }
            }
            // Queenside castling
            if (rights & CastlingRights::kBlackQueenside) {
                if (board.piece_at(pieces::Square(59)).is_none() && // d8
                    board.piece_at(pieces::Square(58)).is_none() && // c8
                    board.piece_at(pieces::Square(57)).is_none()) { // b8
                    moves.emplace_back(sq, pieces::Square(58), MoveType::Castling);
                }
            }
        }
    }
}

bool MoveGenerator::is_square_attacked(const board::Board& board, pieces::Square square, pieces::Color attacker_color) {
    if (!square.is_valid()) return false;
    int target_file = square.file();
    int target_rank = square.rank();

    // 1. Attack by pawns
    int pawn_dir = (attacker_color == pieces::Color::White) ? -1 : 1;
    int from_rank = target_rank + pawn_dir;
    if (from_rank >= 0 && from_rank < 8) {
        int from_files[2] = { target_file - 1, target_file + 1 };
        for (int from_file : from_files) {
            if (from_file >= 0 && from_file < 8) {
                pieces::Square attacker_sq(static_cast<uint8_t>(from_file), static_cast<uint8_t>(from_rank));
                pieces::Piece p = board.piece_at(attacker_sq);
                if (p.type() == pieces::PieceType::Pawn && p.color() == attacker_color) {
                    return true;
                }
            }
        }
    }

    // 2. Attack by knights
    const std::pair<int, int> knight_offsets[8] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    for (const auto& [df, dr] : knight_offsets) {
        int f = target_file + df;
        int r = target_rank + dr;
        if (f >= 0 && f < 8 && r >= 0 && r < 8) {
            pieces::Square attacker_sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
            pieces::Piece p = board.piece_at(attacker_sq);
            if (p.type() == pieces::PieceType::Knight && p.color() == attacker_color) {
                return true;
            }
        }
    }

    // 3. Attack by king
    const std::pair<int, int> king_offsets[8] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
        {0, 1}, {1, -1}, {1, 0}, {1, 1}
    };
    for (const auto& [df, dr] : king_offsets) {
        int f = target_file + df;
        int r = target_rank + dr;
        if (f >= 0 && f < 8 && r >= 0 && r < 8) {
            pieces::Square attacker_sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
            pieces::Piece p = board.piece_at(attacker_sq);
            if (p.type() == pieces::PieceType::King && p.color() == attacker_color) {
                return true;
            }
        }
    }

    // 4. Sliding attack: Bishop / Queen (Diagonals)
    const std::pair<int, int> diagonal_dirs[4] = {
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };
    for (const auto& [df, dr] : diagonal_dirs) {
        int f = target_file + df;
        int r = target_rank + dr;
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            pieces::Square attacker_sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
            pieces::Piece p = board.piece_at(attacker_sq);
            if (p.is_present()) {
                if (p.color() == attacker_color &&
                    (p.type() == pieces::PieceType::Bishop || p.type() == pieces::PieceType::Queen)) {
                    return true;
                }
                break; // Blocked by piece
            }
            f += df;
            r += dr;
        }
    }

    // 5. Sliding attack: Rook / Queen (Orthogonals)
    const std::pair<int, int> orthogonal_dirs[4] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1}
    };
    for (const auto& [df, dr] : orthogonal_dirs) {
        int f = target_file + df;
        int r = target_rank + dr;
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            pieces::Square attacker_sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
            pieces::Piece p = board.piece_at(attacker_sq);
            if (p.is_present()) {
                if (p.color() == attacker_color &&
                    (p.type() == pieces::PieceType::Rook || p.type() == pieces::PieceType::Queen)) {
                    return true;
                }
                break; // Blocked by piece
            }
            f += df;
            r += dr;
        }
    }

    return false;
}

bool MoveGenerator::is_in_check(const board::Board& board, pieces::Color color) {
    // Find king
    pieces::Square king_sq;
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.type() == pieces::PieceType::King && p.color() == color) {
            king_sq = sq;
            break;
        }
    }
    if (king_sq.is_none()) return false;
    return is_square_attacked(board, king_sq, pieces::opposite_color(color));
}

uint64_t MoveGenerator::perft(board::Board& board, int depth) {
    if (depth <= 0) return 1;

    std::vector<Move> moves = generate_legal_moves(board);
    if (depth == 1) return moves.size();

    uint64_t nodes = 0;
    for (const auto& move : moves) {
        BoardState state = board.make_move(move);
        nodes += perft(board, depth - 1);
        board.undo_move(move, state);
    }
    return nodes;
}

} // namespace chess::moves
