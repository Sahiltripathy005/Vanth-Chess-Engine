#ifndef CHESS_EVALUATION_CONSTANTS_HPP
#define CHESS_EVALUATION_CONSTANTS_HPP

#include <array>
#include <cstdint>

namespace chess::search {

// Piece-Square Tables (PST) and other constants for Evaluation.
// Indexed such that a1=0, h1=7, a8=56, h8=63.
// From White's perspective; Black's perspective mirrors the ranks (index ^ 56).

// PST arrays: [64]
using PST = std::array<int, 64>;

// Middlegame PSTs
extern PST kPawnPST_MG;
extern PST kKnightPST_MG;
extern PST kBishopPST_MG;
extern PST kRookPST_MG;
extern PST kQueenPST_MG;
extern PST kKingPST_MG;

// Endgame PSTs
extern PST kPawnPST_EG;
extern PST kKnightPST_EG;
extern PST kBishopPST_EG;
extern PST kRookPST_EG;
extern PST kQueenPST_EG;
extern PST kKingPST_EG;

// Configurable Weights Structs
struct PawnStructureWeights {
    int passed_pawn = 30;         // Bonus for passed pawns
    int passed_pawn_by_rank[8] = { 0, 5, 10, 20, 40, 70, 100, 0 }; // Rank bonus
    int isolated_pawn = -15;       // Penalty for isolated pawns
    int doubled_pawn = -15;        // Penalty for doubled pawns
    int backward_pawn = -10;       // Penalty for backward pawns
    int connected_pawn = 10;       // Bonus for connected pawns
    int pawn_chain = 5;            // Bonus per pawn protecting another
    int candidate_passed = 15;     // Bonus for candidate passed pawns
    int blocked_pawn = -10;        // Penalty for blocked pawns (pawn directly blocked by enemy piece)
    int central_pawn = 15;         // Bonus for pawns in center (d4, e4, d5, e5)
};

struct KingSafetyWeights {
    int shield_pawn_1st_row = 15;   // Bonus for pawns directly in front of castled king
    int shield_pawn_2nd_row = 8;    // Bonus for pawns one rank ahead in front of king
    int open_file_near_king = -20;  // Penalty for open file next to or on king's file
    int semi_open_file_near_king = -10; // Penalty for semi-open file near king
    int castled_king_bonus = 30;    // Bonus if king is castled (e.g. g1/c1/g8/c8)
    int king_exposure_penalty = -2; // Penalty per enemy piece attacking near king
    int king_endgame_activity = 20; // Bonus for active king in endgame
};

struct MobilityWeights {
    int knight_per_square = 4;
    int bishop_per_square = 3;
    int rook_per_square = 2;
    int queen_per_square = 1;
};

struct RookActivityWeights {
    int open_file = 20;
    int semi_open_file = 10;
    int seventh_rank = 30;
    int connected_rooks = 15;
    int behind_passed_pawn = 25;
};

extern PawnStructureWeights kPawnWeights;
extern KingSafetyWeights kKingSafetyWeights;
extern MobilityWeights kMobilityWeights;
extern RookActivityWeights kRookWeights;

extern int kBishopPairBonus;
extern int kTempoBonus;

} // namespace chess::search

#endif // CHESS_EVALUATION_CONSTANTS_HPP
