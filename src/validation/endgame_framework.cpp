#include "chess/validation/endgame_framework.hpp"
#include <map>
#include <vector>
#include <algorithm>

namespace chess::validation {

EndgameFramework::EndgameFramework() {}

bool EndgameFramework::is_endgame(const board::Board& board) {
    int white_material = 0;
    int black_material = 0;

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Piece p = board.piece_at(pieces::Square(i));
        if (p.is_none() || p.type() == pieces::PieceType::King) continue;

        int val = 0;
        switch (p.type()) {
            case pieces::PieceType::Pawn: val = 100; break;
            case pieces::PieceType::Knight: val = 300; break;
            case pieces::PieceType::Bishop: val = 300; break;
            case pieces::PieceType::Rook: val = 500; break;
            case pieces::PieceType::Queen: val = 900; break;
            default: break;
        }

        if (p.color() == pieces::Color::White) {
            white_material += val;
        } else {
            black_material += val;
        }
    }

    // Standard engine endgame definition: major/minor piece material <= 1300 per side
    // e.g. Queen + Rook = 1400 (not endgame yet), or 2 Rooks + Minor = 1300 (endgame)
    return (white_material <= 1300 && black_material <= 1300);
}

std::string EndgameFramework::get_material_signature(const board::Board& board) {
    std::string white_side = "K";
    std::string black_side = "K";

    std::vector<char> white_pieces;
    std::vector<char> black_pieces;

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Piece p = board.piece_at(pieces::Square(i));
        if (p.is_none() || p.type() == pieces::PieceType::King) continue;

        char c = '?';
        switch (p.type()) {
            case pieces::PieceType::Pawn: c = 'P'; break;
            case pieces::PieceType::Knight: c = 'N'; break;
            case pieces::PieceType::Bishop: c = 'B'; break;
            case pieces::PieceType::Rook: c = 'R'; break;
            case pieces::PieceType::Queen: c = 'Q'; break;
            default: break;
        }

        if (p.color() == pieces::Color::White) {
            white_pieces.push_back(c);
        } else {
            black_pieces.push_back(c);
        }
    }

    // Sort pieces by value/character descending: Q, R, B, N, P
    auto sort_fn = [](char a, char b) {
        auto val = [](char x) {
            if (x == 'Q') return 5;
            if (x == 'R') return 4;
            if (x == 'B') return 3;
            if (x == 'N') return 2;
            return 1; // P
        };
        return val(a) > val(b);
    };

    std::sort(white_pieces.begin(), white_pieces.end(), sort_fn);
    std::sort(black_pieces.begin(), black_pieces.end(), sort_fn);

    for (char c : white_pieces) white_side += c;
    for (char c : black_pieces) black_side += c;

    return white_side + "v" + black_side;
}

TablebaseResult EndgameFramework::probe_wdl(const board::Board& board, int& out_score) {
    (void)board;
    out_score = 0;
    return TablebaseResult::Miss; // Stub only
}

TablebaseResult EndgameFramework::probe_dtz(const board::Board& board, moves::Move& out_best_move, int& out_dtz) {
    (void)board;
    out_best_move = moves::Move();
    out_dtz = 0;
    return TablebaseResult::Miss; // Stub only
}

} // namespace chess::validation
