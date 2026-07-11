#include "chess/board/board.hpp"
#include "chess/search/evaluation_constants.hpp"

namespace chess::search {

// Piece-Square Tables definition

PST kPawnPST_MG = {{
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
}};

PST kPawnPST_EG = {{
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     30, 30, 30, 30, 30, 30, 30, 30,
     20, 20, 20, 20, 20, 20, 20, 20,
     10, 10, 10, 10, 10, 10, 10, 10,
      5,  5,  5,  5,  5,  5,  5,  5,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0
}};

PST kKnightPST_MG = {{
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
}};

PST kKnightPST_EG = {{
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
}};

PST kBishopPST_MG = {{
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
}};

PST kBishopPST_EG = {{
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
    -20,-10,-10,-10,-10,-10,-10,-20
}};

PST kRookPST_MG = {{
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
}};

PST kRookPST_EG = {{
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0
}};

PST kQueenPST_MG = {{
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
}};

PST kQueenPST_EG = {{
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  5,  5,  5,  5,  5,  5,-10,
     -5,  5,  5,  5,  5,  5,  5, -5,
     -5,  5,  5,  5,  5,  5,  5, -5,
    -10,  5,  5,  5,  5,  5,  5,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
}};

PST kKingPST_MG = {{
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
}};

PST kKingPST_EG = {{
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
}};

// Weights Definitions

PawnStructureWeights kPawnWeights;
KingSafetyWeights kKingSafetyWeights;
MobilityWeights kMobilityWeights;
RookActivityWeights kRookWeights;

int kBishopPairBonus = 40;
int kTempoBonus = 10;

int evaluate_piece_square_tables(const board::Board& board, int phase) {
    int score_mg = 0;
    int score_eg = 0;

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square sq(i);
        pieces::Piece p = board.piece_at(sq);
        if (p.is_present()) {
            int sq_idx = sq.index();
            if (p.color() == pieces::Color::Black) {
                sq_idx ^= 56; // Mirror vertically for Black
            }

            int val_mg = 0;
            int val_eg = 0;

            switch (p.type()) {
                case pieces::PieceType::Pawn:
                    val_mg = kPawnPST_MG[sq_idx];
                    val_eg = kPawnPST_EG[sq_idx];
                    break;
                case pieces::PieceType::Knight:
                    val_mg = kKnightPST_MG[sq_idx];
                    val_eg = kKnightPST_EG[sq_idx];
                    break;
                case pieces::PieceType::Bishop:
                    val_mg = kBishopPST_MG[sq_idx];
                    val_eg = kBishopPST_EG[sq_idx];
                    break;
                case pieces::PieceType::Rook:
                    val_mg = kRookPST_MG[sq_idx];
                    val_eg = kRookPST_EG[sq_idx];
                    break;
                case pieces::PieceType::Queen:
                    val_mg = kQueenPST_MG[sq_idx];
                    val_eg = kQueenPST_EG[sq_idx];
                    break;
                case pieces::PieceType::King:
                    val_mg = kKingPST_MG[sq_idx];
                    val_eg = kKingPST_EG[sq_idx];
                    break;
                default: break;
            }

            if (p.color() == pieces::Color::White) {
                score_mg += val_mg;
                score_eg += val_eg;
            } else {
                score_mg -= val_mg;
                score_eg -= val_eg;
            }
        }
    }

    return (score_mg * phase + score_eg * (256 - phase)) / 256;
}

} // namespace chess::search
