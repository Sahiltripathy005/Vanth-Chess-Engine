#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"
#include "chess/moves/move_generator.hpp"

namespace chess::search {

int evaluate_mobility(const board::Board& board) {
    int score = 0;

    // Active player mobility
    auto active_moves = moves::MoveGenerator::generate_pseudo_legal_moves(board);
    for (const auto& move : active_moves) {
        pieces::Piece p = board.piece_at(move.from());
        int val = 0;
        switch (p.type()) {
            case pieces::PieceType::Knight: val = kMobilityWeights.knight_per_square; break;
            case pieces::PieceType::Bishop: val = kMobilityWeights.bishop_per_square; break;
            case pieces::PieceType::Rook:   val = kMobilityWeights.rook_per_square; break;
            case pieces::PieceType::Queen:  val = kMobilityWeights.queen_per_square; break;
            default: break;
        }
        if (p.color() == pieces::Color::White) {
            score += val;
        } else {
            score -= val;
        }
    }

    // Inactive player mobility
    board::Board temp_board = board;
    temp_board.set_active_color(pieces::opposite_color(board.active_color()));
    auto inactive_moves = moves::MoveGenerator::generate_pseudo_legal_moves(temp_board);
    for (const auto& move : inactive_moves) {
        pieces::Piece p = temp_board.piece_at(move.from());
        int val = 0;
        switch (p.type()) {
            case pieces::PieceType::Knight: val = kMobilityWeights.knight_per_square; break;
            case pieces::PieceType::Bishop: val = kMobilityWeights.bishop_per_square; break;
            case pieces::PieceType::Rook:   val = kMobilityWeights.rook_per_square; break;
            case pieces::PieceType::Queen:  val = kMobilityWeights.queen_per_square; break;
            default: break;
        }
        if (p.color() == pieces::Color::White) {
            score += val;
        } else {
            score -= val;
        }
    }

    return score;
}

} // namespace chess::search
