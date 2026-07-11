#include "chess/search/evaluation.hpp"
#include "chess/search/evaluation_constants.hpp"

namespace chess::search {

int evaluate_tempo(const board::Board& board) {
    return (board.active_color() == pieces::Color::White) ? kTempoBonus : -kTempoBonus;
}

int evaluate(const board::Board& board) {
    int phase = get_game_phase(board);

    int material = evaluate_material(board);
    int pst = evaluate_piece_square_tables(board, phase);
    int pawns = evaluate_pawn_structure(board);
    int piece_activity = evaluate_piece_activity(board, phase);
    int king_safety = evaluate_king_safety(board, phase);
    int space = evaluate_space(board);
    int threats = evaluate_threats(board);
    int endgame = evaluate_endgame(board, phase);
    int tempo = evaluate_tempo(board);

    return material + pst + pawns + piece_activity + king_safety + space + threats + endgame + tempo;
}

} // namespace chess::search
