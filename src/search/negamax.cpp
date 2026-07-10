#include "chess/search/negamax.hpp"
#include "chess/search/evaluation.hpp"
#include "chess/search/see.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>

namespace chess::search {

static int score_move(const moves::Move& move, const board::Board& board, const moves::Move& tt_move, int ply, const SearchContext& context, const moves::Move& prev_move) {
    // 1. Principal Variation / TT Move
    if (move == tt_move) {
        return 1000000;
    }

    // 2. Captures / Promotions (ordered by SEE)
    pieces::Piece victim = board.piece_at(move.to());
    if (victim.is_present() || move.type() == moves::MoveType::EnPassant || move.type() == moves::MoveType::Promotion) {
        int see_val = see(board, move);
        if (see_val >= 0) {
            return 800000 + see_val;
        } else {
            return 100000 + see_val; // Bad capture
        }
    }

    // 3. Killer Moves
    if (ply < 128) {
        if (move == context.killer_moves[ply][0]) {
            return 700000;
        }
        if (move == context.killer_moves[ply][1]) {
            return 600000;
        }
    }

    // 3.5 Counter Move Heuristic
    if (!prev_move.from().is_none()) {
        if (move == context.counter_moves[prev_move.from().index()][prev_move.to().index()]) {
            return 550000;
        }
    }

    // 4. History Heuristic & Continuation History Moves
    pieces::Piece p = board.piece_at(move.from());
    if (p.is_present()) {
        int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
        int p_idx = 0;
        switch (p.type()) {
            case pieces::PieceType::Pawn:   p_idx = 0; break;
            case pieces::PieceType::Knight: p_idx = 1; break;
            case pieces::PieceType::Bishop: p_idx = 2; break;
            case pieces::PieceType::Rook:   p_idx = 3; break;
            case pieces::PieceType::Queen:  p_idx = 4; break;
            case pieces::PieceType::King:   p_idx = 5; break;
            default: break;
        }
        int hist_score = context.history_moves[c_idx][p_idx][move.to().index()];
        int conthist = 0;
        if (!prev_move.from().is_none()) {
            conthist = context.continuation_history[prev_move.to().index()][move.to().index()];
        }
        int combined = hist_score + conthist;
        if (combined > 400000) combined = 400000;
        if (combined < -400000) combined = -400000;
        return 200000 + combined;
    }

    // 5. Quiet/Ordinary Moves
    return 0;
}

static void order_moves(std::vector<moves::Move>& moves, const board::Board& board, const moves::Move& tt_move, int ply, const SearchContext& context, const moves::Move& prev_move) {
    std::vector<int> scores(moves.size());
    for (size_t i = 0; i < moves.size(); ++i) {
        scores[i] = score_move(moves[i], board, tt_move, ply, context, prev_move);
    }

    // Selection sort
    for (size_t i = 0; i < moves.size(); ++i) {
        for (size_t j = i + 1; j < moves.size(); ++j) {
            if (scores[j] > scores[i]) {
                std::swap(scores[i], scores[j]);
                std::swap(moves[i], moves[j]);
            }
        }
    }
}

int qsearch(board::Board& board, int alpha, int beta, SearchInfo& info, SearchContext& context) {
    info.nodes++;
    info.q_nodes++;

    // Check time limit
    if (info.nodes % 2048 == 0) {
        context.time_manager.check_time(info);
    }
    if (info.aborted) {
        return 0;
    }

    // Stand-pat evaluation
    info.leaf_nodes++;
    int eval = evaluate(board);
    int stand_pat = (board.active_color() == pieces::Color::White) ? eval : -eval;

    if (stand_pat >= beta) {
        return stand_pat;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    // Generate pseudo-legal moves
    std::vector<moves::Move> moves = moves::MoveGenerator::generate_pseudo_legal_moves(board);
    
    // Filter to captures only
    std::vector<moves::Move> captures;
    for (const auto& move : moves) {
        if (board.piece_at(move.to()).is_present() || 
            move.type() == moves::MoveType::EnPassant || 
            move.type() == moves::MoveType::Promotion) {
            captures.push_back(move);
        }
    }

    // Order captures using SEE
    std::vector<int> scores(captures.size());
    for (size_t i = 0; i < captures.size(); ++i) {
        scores[i] = see(board, captures[i]);
    }
    for (size_t i = 0; i < captures.size(); ++i) {
        for (size_t j = i + 1; j < captures.size(); ++j) {
            if (scores[j] > scores[i]) {
                std::swap(scores[i], scores[j]);
                std::swap(captures[i], captures[j]);
            }
        }
    }

    for (const auto& move : captures) {
        // Delta Pruning
        pieces::Piece target = board.piece_at(move.to());
        int target_val = 100;
        if (target.is_present()) {
            switch (target.type()) {
                case pieces::PieceType::Knight: target_val = 320; break;
                case pieces::PieceType::Bishop: target_val = 330; break;
                case pieces::PieceType::Rook:   target_val = 500; break;
                case pieces::PieceType::Queen:  target_val = 900; break;
                default: target_val = 100; break;
            }
        }
        if (move.type() == moves::MoveType::Promotion) {
            target_val += 800;
        }
        if (stand_pat + target_val + 200 < alpha) {
            continue;
        }

        // SEE pruning
        int see_score = see(board, move);
        if (see_score < 0) {
            continue;
        }

        moves::BoardState state = board.make_move(move);
        if (moves::MoveGenerator::is_in_check(board, pieces::opposite_color(board.active_color()))) {
            board.undo_move(move, state);
            continue;
        }

        int score = -qsearch(board, -beta, -alpha, info, context);
        board.undo_move(move, state);

        if (info.aborted) {
            return 0;
        }

        if (score >= beta) {
            return score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

int negamax(board::Board& board, int depth, int ply, int alpha, int beta, SearchInfo& info, SearchContext& context, const moves::Move& prev_move) {
    info.nodes++;

    // Check time limit
    if (info.nodes % 2048 == 0) {
        context.time_manager.check_time(info);
    }
    if (info.aborted) {
        return 0;
    }

    // 1. Threefold Repetition detection (draw)
    if (ply > 0) {
        uint64_t hash = board.zobrist_hash();
        for (size_t i = 0; i < context.board_history.size(); ++i) {
            if (context.board_history[i] == hash) {
                return kDrawScore;
            }
        }
    }

    // 2. 50-move rule
    if (board.halfmove_clock() >= 100) {
        return kDrawScore;
    }

    // 3. Transposition Table Probe
    moves::Move tt_move;
    int tt_score;
    info.tt_lookups++;
    if (context.tt.probe(board.zobrist_hash(), depth, ply, alpha, beta, tt_score, tt_move)) {
        info.tt_hits++;
        return tt_score;
    }

    // 4. Base Case: Leaf node -> Quiescence Search
    if (depth <= 0) {
        return qsearch(board, alpha, beta, info, context);
    }

    bool in_check = moves::MoveGenerator::is_in_check(board, board.active_color());
    bool is_pv_node = (beta - alpha > 1);

    // Static evaluation calculation
    int eval_val = evaluate(board);
    int static_eval = (board.active_color() == pieces::Color::White) ? eval_val : -eval_val;

    // 5. Reverse Futility Pruning (Static Null Move Pruning)
    if (depth <= 3 && !in_check && !is_pv_node) {
        int margin = 120 * depth;
        if (static_eval - margin >= beta) {
            return static_eval;
        }
    }

    // 6. Null Move Pruning (NMP)
    if (depth >= 3 && !in_check && !is_pv_node) {
        bool has_major_minor = false;
        for (uint8_t i = 0; i < 64; ++i) {
            pieces::Piece p = board.piece_at(pieces::Square(i));
            if (p.is_present() && p.color() == board.active_color() && 
                p.type() != pieces::PieceType::Pawn && p.type() != pieces::PieceType::King) {
                has_major_minor = true;
                break;
            }
        }

        if (has_major_minor) {
            board::Board sim_board = board;
            sim_board.set_active_color(pieces::opposite_color(board.active_color()));
            sim_board.set_en_passant_square(pieces::Square::None());

            int R = (depth > 6) ? 3 : 2;
            int score = -negamax(sim_board, depth - 1 - R, ply + 1, -beta, -beta + 1, info, context, moves::Move());

            if (info.aborted) {
                return 0;
            }

            if (score >= beta) {
                info.nmp_cutoffs++;
                return score; // Beta cutoff
            }
        }
    }

    // 7. Generate moves
    std::vector<moves::Move> moves = moves::MoveGenerator::generate_legal_moves(board);

    if (moves.empty()) {
        if (in_check) {
            return -kMateScore + ply; // Mate distance-to-root preference
        } else {
            return kDrawScore; // Stalemate
        }
    }

    // Order moves
    order_moves(moves, board, tt_move, ply, context, prev_move);

    int best_score = -kInfiniteScore;
    moves::Move best_move;
    TTBound bound = TTBound::Upper;

    int moves_searched = 0;
    for (const auto& move : moves) {
        bool is_capture = board.piece_at(move.to()).is_present() || 
                           move.type() == moves::MoveType::EnPassant;
        bool is_promo = move.type() == moves::MoveType::Promotion;

        // Push board hash to history stack
        context.board_history.push_back(board.zobrist_hash());
        moves::BoardState state = board.make_move(move);

        if (moves::MoveGenerator::is_in_check(board, pieces::opposite_color(board.active_color()))) {
            board.undo_move(move, state);
            context.board_history.pop_back();
            continue;
        }

        bool gives_check = moves::MoveGenerator::is_in_check(board, board.active_color());

        // 8. Futility Pruning
        if (depth == 1 && !in_check && !is_capture && !is_promo && !gives_check) {
            if (static_eval + 150 < alpha) {
                board.undo_move(move, state);
                context.board_history.pop_back();
                continue; // Prune quiet move
            }
        }

        // 9. Check Extension
        int extension = (gives_check) ? 1 : 0;
        int next_depth = depth - 1 + extension;
        int score = 0;

        if (moves_searched == 0) {
            // Full window search for first move
            score = -negamax(board, next_depth, ply + 1, -beta, -alpha, info, context, move);
        } else {
            // 10. Late Move Reduction (LMR)
            bool do_lmr = (depth >= 3 && moves_searched >= 4 && !in_check && !gives_check && !is_capture && !is_promo);
            if (do_lmr) {
                int reduction = (depth > 5 && moves_searched >= 8) ? 2 : 1;
                score = -negamax(board, next_depth - reduction, ply + 1, -alpha - 1, -alpha, info, context, move);
                info.lmr_reductions++;

                if (score > alpha && !info.aborted) {
                    // Re-search without reduction
                    score = -negamax(board, next_depth, ply + 1, -alpha - 1, -alpha, info, context, move);
                }
            } else {
                // Null window search for non-PV moves
                score = -negamax(board, next_depth, ply + 1, -alpha - 1, -alpha, info, context, move);
            }

            // 11. PVS Re-search: if null window search fails high, re-search with full window
            if (score > alpha && score < beta && !info.aborted) {
                score = -negamax(board, next_depth, ply + 1, -beta, -alpha, info, context, move);
                info.pvs_researches++;
            }
        }

        board.undo_move(move, state);
        context.board_history.pop_back();

        if (info.aborted) {
            return 0;
        }

        moves_searched++;

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }

        if (score > alpha) {
            alpha = score;
            bound = TTBound::Exact;
        }

        if (alpha >= beta) {
            // Beta cutoff
            info.cutoffs++;
            bound = TTBound::Lower;

            info.ordering_lookups++;
            if (moves_searched == 1) {
                info.fhf++;
            }

            // Killer Move Heuristic (only for quiet/non-capture moves)
            if (!is_capture && !is_promo && ply < 128) {
                if (context.killer_moves[ply][0] != move) {
                    context.killer_moves[ply][1] = context.killer_moves[ply][0];
                    context.killer_moves[ply][0] = move;
                }
            }

            // Counter Move Heuristic (only for quiet/non-capture moves)
            if (!is_capture && !is_promo && !prev_move.from().is_none()) {
                context.counter_moves[prev_move.from().index()][prev_move.to().index()] = move;
            }

            // Continuation History (only for quiet/non-capture moves)
            if (!is_capture && !is_promo && !prev_move.from().is_none()) {
                int& conthist = context.continuation_history[prev_move.to().index()][move.to().index()];
                int change = depth * depth;
                conthist += change - (conthist * change / 16384);
            }

            // History Heuristic (only for quiet/non-capture moves)
            if (!is_capture && !is_promo) {
                pieces::Piece p = board.piece_at(move.from());
                if (p.is_present()) {
                    int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                    int p_idx = 0;
                    switch (p.type()) {
                        case pieces::PieceType::Pawn:   p_idx = 0; break;
                        case pieces::PieceType::Knight: p_idx = 1; break;
                        case pieces::PieceType::Bishop: p_idx = 2; break;
                        case pieces::PieceType::Rook:   p_idx = 3; break;
                        case pieces::PieceType::Queen:  p_idx = 4; break;
                        case pieces::PieceType::King:   p_idx = 5; break;
                        default: break;
                    }
                    
                    int& hist = context.history_moves[c_idx][p_idx][move.to().index()];
                    int change = depth * depth;
                    hist += change - (hist * change / 16384);
                }
            }

            // History Aging/Penalty for quiet moves searched before the cutoff
            for (int k = 0; k < moves_searched - 1; ++k) {
                const auto& bad_move = moves[k];
                bool bad_capture = board.piece_at(bad_move.to()).is_present() || bad_move.type() == moves::MoveType::EnPassant;
                bool bad_promo = bad_move.type() == moves::MoveType::Promotion;
                if (!bad_capture && !bad_promo) {
                    pieces::Piece bp = board.piece_at(bad_move.from());
                    if (bp.is_present()) {
                        int bc_idx = (bp.color() == pieces::Color::White) ? 0 : 1;
                        int bp_idx = 0;
                        switch (bp.type()) {
                            case pieces::PieceType::Pawn:   bp_idx = 0; break;
                            case pieces::PieceType::Knight: bp_idx = 1; break;
                            case pieces::PieceType::Bishop: bp_idx = 2; break;
                            case pieces::PieceType::Rook:   bp_idx = 3; break;
                            case pieces::PieceType::Queen:  bp_idx = 4; break;
                            case pieces::PieceType::King:   bp_idx = 5; break;
                            default: break;
                        }
                        int& hist = context.history_moves[bc_idx][bp_idx][bad_move.to().index()];
                        hist -= depth * depth;
                        if (hist < -400000) hist = -400000;
                    }

                    if (!prev_move.from().is_none()) {
                        int& conthist = context.continuation_history[prev_move.to().index()][bad_move.to().index()];
                        conthist -= depth * depth;
                        if (conthist < -400000) conthist = -400000;
                    }
                }
            }

            break;
        }
    }

    // Store in Transposition Table
    context.tt.store(board.zobrist_hash(), depth, ply, best_score, bound, best_move);

    return best_score;
}

} // namespace chess::search
