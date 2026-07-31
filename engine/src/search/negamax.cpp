#include "chess/search/negamax.hpp"
#include "chess/search/evaluation.hpp"
#include "chess/search/see.hpp"
#include "chess/moves/move_generator.hpp"
#include <algorithm>
#include <cmath>

namespace chess::search {

bool g_enable_tt = true;
bool g_enable_qsearch = true;
bool g_enable_nmp = true;
bool g_enable_lmr = true;
bool g_enable_rfp = true;
bool g_enable_pvs = true;
bool g_enable_futility = true;
bool g_enable_razoring = false;
bool g_enable_aspiration = false;
CaptureOrderingMode g_capture_ordering_mode = CaptureOrderingMode::Hybrid;

// V3.6 Search Optimization Flags
bool g_enable_v36_ordering = true;
bool g_enable_conthist = true;
bool g_enable_countermoves = true;

// Central Config instance & reductions table (Module 6)
SearchConfig g_search_config;
int g_reductions[64][64];

void init_search_heuristics() {
    for (int d = 0; d < 64; ++d) {
        for (int m = 0; m < 64; ++m) {
            if (d == 0 || m == 0) {
                g_reductions[d][m] = 0;
            } else {
                double r = g_search_config.lmr_base + std::log(d) * std::log(m) / g_search_config.lmr_divisor;
                g_reductions[d][m] = std::max(0, (int)std::round(r));
            }
        }
    }
}

static int get_piece_value_mvv_lva(pieces::PieceType type) {
    switch (type) {
        case pieces::PieceType::Pawn:   return 100;
        case pieces::PieceType::Knight: return 320;
        case pieces::PieceType::Bishop: return 330;
        case pieces::PieceType::Rook:   return 500;
        case pieces::PieceType::Queen:  return 900;
        case pieces::PieceType::King:   return 20000;
        default: return 0;
    }
}

static int get_piece_index(pieces::PieceType type) {
    switch (type) {
        case pieces::PieceType::Pawn:   return 0;
        case pieces::PieceType::Knight: return 1;
        case pieces::PieceType::Bishop: return 2;
        case pieces::PieceType::Rook:   return 3;
        case pieces::PieceType::Queen:  return 4;
        case pieces::PieceType::King:   return 5;
        default: return 0;
    }
}

static int mvv_lva(const board::Board& board, const moves::Move& move) {
    pieces::PieceType victim = board.piece_at(move.to()).type();
    if (move.type() == moves::MoveType::EnPassant) {
        victim = pieces::PieceType::Pawn;
    }
    pieces::PieceType attacker = board.piece_at(move.from()).type();
    return 10 * get_piece_value_mvv_lva(victim) - get_piece_value_mvv_lva(attacker);
}

static int score_move(const moves::Move& move, const board::Board& board, const moves::Move& tt_move, int ply, const SearchContext& context, const moves::Move& prev_move) {
    // --- V3.5 Fallback Mode ---
    if (!g_enable_v36_ordering) {
        if (ply == 0 && !context.previous_pv_move.from().is_none() && move == context.previous_pv_move) {
            return 2000000;
        }
        if (move == tt_move) {
            return 1900000;
        }
        pieces::Piece victim = board.piece_at(move.to());
        bool is_cap = victim.is_present() || move.type() == moves::MoveType::EnPassant;
        bool is_promo = move.type() == moves::MoveType::Promotion;
        if (is_cap || is_promo) {
            int see_val = 0;
            int mvv_val = 0;
            if (g_capture_ordering_mode == CaptureOrderingMode::MvvLvaOnly) {
                mvv_val = mvv_lva(board, move);
                return 800000 + mvv_val;
            } else if (g_capture_ordering_mode == CaptureOrderingMode::SeeOnly) {
                see_val = see(board, move);
                if (see_val > 0) {
                    return 1000000 + see_val;
                } else if (see_val == 0) {
                    return 900000;
                } else {
                    return 100000 + see_val;
                }
            } else {
                see_val = see(board, move);
                mvv_val = mvv_lva(board, move);
                if (see_val > 0) {
                    return 1000000 + see_val * 1000 + mvv_val;
                } else if (see_val == 0) {
                    return 900000 + mvv_val;
                } else {
                    return 100000 + see_val * 1000 + mvv_val;
                }
            }
        }
        if (ply < 128) {
            if (move == context.killer_moves[ply][0]) return 800000;
            if (move == context.killer_moves[ply][1]) return 750000;
        }
        int countermove_bonus = 0;
        if (!prev_move.from().is_none()) {
            if (move == context.counter_moves[prev_move.from().index()][prev_move.to().index()]) {
                countermove_bonus = 100000;
            }
        }
        pieces::Piece p = board.piece_at(move.from());
        if (p.is_present()) {
            int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
            int p_idx = get_piece_index(p.type());
            int hist_score = context.history_moves[c_idx][p_idx][move.to().index()];
            return 200000 + hist_score * 4 + countermove_bonus;
        }
        return 200000 + countermove_bonus;
    }

    // --- V3.6 Professional Move Ordering ---
    if (ply == 0 && !context.previous_pv_move.from().is_none() && move == context.previous_pv_move) {
        return 10000000;
    }
    if (move == tt_move) {
        return 9000000;
    }

    pieces::Piece victim = board.piece_at(move.to());
    bool is_cap = victim.is_present() || move.type() == moves::MoveType::EnPassant;
    bool is_promo = move.type() == moves::MoveType::Promotion;

    if (is_cap || is_promo) {
        int see_val = see(board, move);
        int mvv_val = mvv_lva(board, move);

        if (see_val > 0) {
            return 8000000 + see_val * 100 + mvv_val;
        } else if (is_promo) {
            int promo_val = get_piece_value_mvv_lva(move.promotion());
            return 7000000 + promo_val;
        } else if (see_val == 0) {
            return 6500000 + mvv_val;
        } else {
            return 1000000 + see_val * 100 + mvv_val;
        }
    }

    // Quiet Moves Heuristics
    bool is_killer_0 = (ply < 128 && move == context.killer_moves[ply][0]);
    bool is_killer_1 = (ply < 128 && move == context.killer_moves[ply][1]);
    
    bool is_counter = false;
    if (g_enable_countermoves && !prev_move.from().is_none()) {
        if (move == context.counter_moves[prev_move.from().index()][prev_move.to().index()]) {
            is_counter = true;
        }
    }

    int hist_score = 0;
    pieces::Piece p = board.piece_at(move.from());
    if (p.is_present()) {
        int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
        int p_idx = get_piece_index(p.type());
        hist_score = context.history_moves[c_idx][p_idx][move.to().index()];
    }

    int conthist = 0;
    if (g_enable_conthist && p.is_present() && p.type() != pieces::PieceType::King) {
        if (context.move_history.size() >= 1) {
            const auto& prev = context.move_history.back();
            pieces::Piece prev_p = board.piece_at(prev.to());
            if (prev_p.is_present() && prev_p.type() != pieces::PieceType::King) {
                int prev_p_idx = get_piece_index(prev_p.type());
                int curr_p_idx = get_piece_index(p.type());
                conthist += context.continuation_history_1->data[prev_p_idx][prev.to().index()][curr_p_idx][move.to().index()];
            }
        }
        if (context.move_history.size() >= 2) {
            const auto& prev_prev = context.move_history[context.move_history.size() - 2];
            pieces::Piece prev_prev_p = board.piece_at(prev_prev.to());
            if (prev_prev_p.is_present() && prev_prev_p.type() != pieces::PieceType::King) {
                int prev_prev_p_idx = get_piece_index(prev_prev_p.type());
                int curr_p_idx = get_piece_index(p.type());
                conthist += context.continuation_history_2->data[prev_prev_p_idx][prev_prev.to().index()][curr_p_idx][move.to().index()];
            }
        }
    }

    return 2000000 + (is_killer_0 ? 3000000 : 0) + (is_killer_1 ? 2800000 : 0) + (is_counter ? 1500000 : 0) + hist_score + conthist;
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

    if (info.nodes % 2048 == 0) {
        context.time_manager.check_time(info);
    }
    if (info.aborted) {
        return 0;
    }

    info.leaf_nodes++;
    int eval = evaluate(board);
    int stand_pat = (board.active_color() == pieces::Color::White) ? eval : -eval;

    if (stand_pat >= beta) {
        return stand_pat;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    std::vector<moves::Move> moves = moves::MoveGenerator::generate_pseudo_legal_moves(board);
    
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

    for (size_t i = 0; i < captures.size(); ++i) {
        const auto& move = captures[i];
        int see_score = scores[i];

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
            info.delta_prunings++;
            continue;
        }

        // SEE pruning
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

    // Guaranteed LMR table initialization (Module 1)
    static bool lmr_init = false;
    if (!lmr_init) {
        init_search_heuristics();
        lmr_init = true;
    }

    if (info.nodes % 2048 == 0) {
        context.time_manager.check_time(info);
    }
    if (info.aborted) {
        return 0;
    }

    if (ply > 0) {
        uint64_t hash = board.zobrist_hash();
        for (size_t i = 0; i < context.board_history.size(); ++i) {
            if (context.board_history[i] == hash) {
                return kDrawScore;
            }
        }
    }

    if (board.halfmove_clock() >= 100) {
        return kDrawScore;
    }

    // --- Mate Distance Pruning (Module 5) ---
    if (g_search_config.mdp_enable) {
        int mated_score = -kMateScore + ply;
        int mate_score = kMateScore - ply;
        if (alpha < mated_score) {
            alpha = mated_score;
            if (alpha >= beta) {
                info.mdp_prunings++;
                info.mdp_usage++;
                return alpha;
            }
        }
        if (beta > mate_score) {
            beta = mate_score;
            if (alpha >= beta) {
                info.mdp_prunings++;
                info.mdp_usage++;
                return beta;
            }
        }
    }

    moves::Move tt_move;
    int tt_score;
    info.tt_lookups++;
    TTBound tt_bound = TTBound::Exact;
    if (g_enable_tt && context.tt.probe(board.zobrist_hash(), depth, ply, alpha, beta, tt_score, tt_move, &tt_bound)) {
        info.tt_hits++;
        if (tt_bound == TTBound::Exact) {
            info.tt_exact_hits++;
        } else if (tt_bound == TTBound::Lower) {
            info.tt_lower_hits++;
        } else if (tt_bound == TTBound::Upper) {
            info.tt_upper_hits++;
        }
        return tt_score;
    }

    if (depth <= 0) {
        if (!g_enable_qsearch) {
            int eval_val = evaluate(board);
            return (board.active_color() == pieces::Color::White) ? eval_val : -eval_val;
        }
        return qsearch(board, alpha, beta, info, context);
    }

    bool in_check = moves::MoveGenerator::is_in_check(board, board.active_color());
    bool is_pv_node = (beta - alpha > 1);

    // Track Node Types for profiling (Module 7)
    if (is_pv_node) {
        info.pv_nodes_count++;
    }

    int eval_val = evaluate(board);
    int static_eval = (board.active_color() == pieces::Color::White) ? eval_val : -eval_val;

    // Track static evaluation history for improving node check (Module 1)
    if (ply < 128) {
        context.static_eval_history[ply] = static_eval;
    }

    // Reverse Futility Pruning
    if (g_enable_rfp && depth <= 3 && !in_check && !is_pv_node) {
        int margin = g_search_config.rfp_margin * depth;
        if (static_eval - margin >= beta) {
            return static_eval;
        }
    }

    // --- Null Move Pruning 2.0 (Module 2) ---
    if (g_enable_nmp && depth >= 3 && !in_check && !is_pv_node) {
        bool has_major_minor = false;
        for (uint8_t i = 0; i < 64; ++i) {
            pieces::Piece pc = board.piece_at(pieces::Square(i));
            if (pc.is_present() && pc.color() == board.active_color() && 
                pc.type() != pieces::PieceType::Pawn && pc.type() != pieces::PieceType::King) {
                has_major_minor = true;
                break;
            }
        }

        // Automatic zugzwang checks (material threshold)
        if (has_major_minor && static_eval >= beta) {
            info.nmp_usage++;
            moves::BoardState state = board.make_null_move();

            // Adaptive reduction R
            int R = g_search_config.null_move_base + depth / 6;
            if (R > g_search_config.null_move_max) R = g_search_config.null_move_max;

            int score = -negamax(board, depth - 1 - R, ply + 1, -beta, -beta + 1, info, context, moves::Move());

            board.undo_null_move(state);

            if (info.aborted) {
                return 0;
            }

            if (score >= beta) {
                // Optional Verification Search (Module 2)
                if (g_search_config.nmp_verification && depth > 5) {
                    // Search at reduced depth to verify the fail-high
                    int verify_score = negamax(board, depth - 1 - R, ply + 1, beta - 1, beta, info, context, moves::Move());
                    if (verify_score >= beta) {
                        info.nmp_cutoffs++;
                        return score;
                    }
                } else {
                    info.nmp_cutoffs++;
                    return score;
                }
            }
        }
    }

    // --- Internal Iterative Deepening (Module 3) ---
    if (is_pv_node && tt_move.from().is_none() && depth >= g_search_config.iid_depth) {
        info.iid_calls++;
        info.iid_usage++;
        // Run a shallower search to populate the transposition table
        negamax(board, depth - 2, ply, alpha, beta, info, context, prev_move);
        moves::Move new_tt_move;
        int dummy_score;
        if (g_tt.probe(board.zobrist_hash(), depth - 2, ply, alpha, beta, dummy_score, new_tt_move)) {
            if (!new_tt_move.from().is_none()) {
                tt_move = new_tt_move;
                info.iid_successes++;
            }
        }
    }

    std::vector<moves::Move> moves = moves::MoveGenerator::generate_legal_moves(board);

    if (moves.empty()) {
        if (in_check) {
            return -kMateScore + ply;
        } else {
            return kDrawScore;
        }
    }

    info.nodes_visited++;
    info.moves_generated_total += moves.size();

    order_moves(moves, board, tt_move, ply, context, prev_move);

    int best_score = -kInfiniteScore;
    moves::Move best_move;
    TTBound bound = TTBound::Upper;

    int moves_searched = 0;
    bool has_beta_cutoff = false;

    for (const auto& move : moves) {
        pieces::Piece p = board.piece_at(move.from());
        pieces::Piece victim = board.piece_at(move.to());

        bool is_capture = victim.is_present() || move.type() == moves::MoveType::EnPassant;
        bool is_promo = move.type() == moves::MoveType::Promotion;
        bool is_tt = (move == tt_move);
        bool is_killer = (ply < 128 && (move == context.killer_moves[ply][0] || move == context.killer_moves[ply][1]));
        
        bool is_counter = false;
        if (g_enable_countermoves && !prev_move.from().is_none()) {
            if (move == context.counter_moves[prev_move.from().index()][prev_move.to().index()]) {
                is_counter = true;
            }
        }
        
        int see_score = 0;
        if (is_capture || is_promo) {
            see_score = see(board, move);
        }

        context.board_history.push_back(board.zobrist_hash());
        context.move_history.push_back(move);
        moves::BoardState state = board.make_move(move);

        if (moves::MoveGenerator::is_in_check(board, pieces::opposite_color(board.active_color()))) {
            board.undo_move(move, state);
            context.board_history.pop_back();
            context.move_history.pop_back();
            continue;
        }

        info.moves_searched_total++;
        if (is_tt) info.tt_move_searched++;
        else if (is_capture || is_promo) {
            if (see_score > 0) info.see_winning_searched++;
            else if (see_score == 0) info.see_neutral_searched++;
            else info.see_losing_searched++;
        }
        else if (is_killer) info.killer_searched++;
        else if (is_counter) info.countermove_searched++;
        else {
            if (p.is_present()) {
                int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                int p_idx = get_piece_index(p.type());
                int hist_score = context.history_moves[c_idx][p_idx][move.to().index()];
                if (hist_score > 0) {
                    info.history_searched++;
                }
            }
        }

        if (!is_capture && !is_promo) {
            if (p.is_present()) {
                int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                int p_idx = get_piece_index(p.type());
                int hist_score = context.history_moves[c_idx][p_idx][move.to().index()];
                info.history_score_sum += std::abs(hist_score);
                info.history_score_count++;
            }
            if (moves_searched >= 4) {
                info.late_moves_searched++;
                info.late_moves_position_sum += moves_searched;
            }
        }

        bool gives_check = moves::MoveGenerator::is_in_check(board, board.active_color());

        if (moves_searched > 0 && depth == 1 && !in_check && !is_capture && !is_promo && !gives_check) {
            if (static_eval + 150 < alpha) {
                board.undo_move(move, state);
                context.board_history.pop_back();
                context.move_history.pop_back();
                continue;
            }
        }

        int extension = (gives_check) ? 1 : 0;
        if (extension > 0) {
            info.total_extension_depth++;
        }
        int next_depth = depth - 1 + extension;
        int score = 0;

        if (moves_searched == 0) {
            score = -negamax(board, next_depth, ply + 1, -beta, -alpha, info, context, move);
        } else {
            // --- Adaptive Late Move Reduction 2.0 (Module 1) ---
            bool do_lmr = (g_enable_lmr && depth >= 3 && moves_searched >= 4 && !in_check && !gives_check);
            if (do_lmr) {
                // Fetch base precalculated LMR value
                int reduction = g_reductions[std::min(63, depth)][std::min(63, moves_searched)];

                // Check recapture status
                bool is_recapture = (!prev_move.from().is_none() && move.to() == prev_move.to() && is_capture);

                // Passed pawn pushes
                bool is_passed_pawn_push = (p.is_present() && p.type() == pieces::PieceType::Pawn && 
                    ((p.color() == pieces::Color::White && move.to().rank() >= 5) || 
                     (p.color() == pieces::Color::Black && move.to().rank() <= 2)));

                // Dynamic Reduction Adjustments
                if (is_pv_node) {
                    reduction -= 1; // PV node: reduce less
                }

                // Improving Check
                bool improving = false;
                if (ply >= 2 && context.static_eval_history[ply] > context.static_eval_history[ply - 2]) {
                    improving = true;
                }
                if (improving) {
                    reduction -= 1; // Improving node: reduce less
                }

                // History score adjustments
                int hist_total = 0;
                if (p.is_present()) {
                    int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                    int p_idx = get_piece_index(p.type());
                    hist_total += context.history_moves[c_idx][p_idx][move.to().index()];
                }
                if (hist_total > 8000) {
                    reduction -= 1;
                } else if (hist_total < -8000) {
                    reduction += 1;
                }

                // SEE pruning
                if (is_capture && see_score < 0) {
                    reduction += 1; // Bad capture: reduce more
                }

                // Bounds checking
                reduction = std::max(0, reduction);

                // Conditions to completely disable reductions
                if (is_pv_node || is_tt || is_killer || is_counter || gives_check || is_recapture || is_passed_pawn_push || is_promo || (is_capture && see_score >= 0)) {
                    reduction = 0;
                }

                if (reduction > 0) {
                    info.lmr_usage++;
                    info.total_reduction_depth += reduction;
                    score = -negamax(board, next_depth - reduction, ply + 1, -alpha - 1, -alpha, info, context, move);
                    info.lmr_reductions++;
                    info.lmr_reduction_depth_sum += reduction;

                    if (score > alpha && !info.aborted) {
                        score = -negamax(board, next_depth, ply + 1, -alpha - 1, -alpha, info, context, move);
                    }
                } else {
                    score = -negamax(board, next_depth, ply + 1, -alpha - 1, -alpha, info, context, move);
                }
            } else {
                score = -negamax(board, next_depth, ply + 1, -alpha - 1, -alpha, info, context, move);
            }

            if (score > alpha && score < beta && !info.aborted) {
                score = -negamax(board, next_depth, ply + 1, -beta, -alpha, info, context, move);
                info.pvs_researches++;
            }
        }

        board.undo_move(move, state);
        context.board_history.pop_back();
        context.move_history.pop_back();

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
            has_beta_cutoff = true;
            info.cutoffs++;
            bound = TTBound::Lower;

            info.ordering_lookups++;
            if (moves_searched == 1) {
                info.fhf++;
            }

            info.cutoff_count_total++;
            info.cutoff_index_sum += (moves_searched - 1);
            if (is_tt) info.tt_move_cutoffs++;
            else if (is_capture || is_promo) {
                if (see_score > 0) info.see_winning_cutoffs++;
                else if (see_score == 0) info.see_neutral_cutoffs++;
                else info.see_losing_cutoffs++;
            }
            else if (is_killer) info.killer_cutoffs++;
            else if (is_counter) info.countermove_cutoffs++;
            else {
                if (p.is_present()) {
                    int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                    int p_idx = get_piece_index(p.type());
                    int hist_score = context.history_moves[c_idx][p_idx][move.to().index()];
                    if (hist_score > 0) {
                        info.history_cutoffs++;
                    }
                }
            }

            if (!is_capture && !is_promo && ply < 128) {
                if (context.killer_moves[ply][0] != move) {
                    context.killer_moves[ply][1] = context.killer_moves[ply][0];
                    context.killer_moves[ply][0] = move;
                }
            }

            if (!is_capture && !is_promo && !prev_move.from().is_none()) {
                context.counter_moves[prev_move.from().index()][prev_move.to().index()] = move;
            }

            int history_change = std::min(400, depth * depth);

            // Update Continuation History tables (1 ply and 2 plies ago - Module 8)
            // Skip King moves to prevent positional endgame regression
            if (g_enable_conthist && !is_capture && !is_promo && p.is_present() && p.type() != pieces::PieceType::King) {
                int curr_p_idx = get_piece_index(p.type());
                if (context.move_history.size() >= 1) {
                    const auto& prev = context.move_history.back();
                    pieces::Piece prev_p = board.piece_at(prev.to());
                    if (prev_p.is_present() && prev_p.type() != pieces::PieceType::King) {
                        int prev_p_idx = get_piece_index(prev_p.type());
                        int& conthist = context.continuation_history_1->data[prev_p_idx][prev.to().index()][curr_p_idx][move.to().index()];
                        conthist += history_change - (conthist * std::abs(history_change) / 16384);
                    }
                }
                if (context.move_history.size() >= 2) {
                    const auto& prev_prev = context.move_history[context.move_history.size() - 2];
                    pieces::Piece prev_prev_p = board.piece_at(prev_prev.to());
                    if (prev_prev_p.is_present() && prev_prev_p.type() != pieces::PieceType::King) {
                        int prev_prev_p_idx = get_piece_index(prev_prev_p.type());
                        int& conthist = context.continuation_history_2->data[prev_prev_p_idx][prev_prev.to().index()][curr_p_idx][move.to().index()];
                        conthist += history_change - (conthist * std::abs(history_change) / 16384);
                    }
                }
            }

            // Update Main History Heuristic
            if (!is_capture && !is_promo) {
                if (p.is_present()) {
                    int c_idx = (p.color() == pieces::Color::White) ? 0 : 1;
                    int p_idx = get_piece_index(p.type());
                    int& hist = context.history_moves[c_idx][p_idx][move.to().index()];
                    hist += history_change - (hist * std::abs(history_change) / 16384);
                }
            }

            // Penalize prior quiet moves searched before this cutoff
            for (int k = 0; k < moves_searched - 1; ++k) {
                const auto& bad_move = moves[k];
                bool bad_capture = board.piece_at(bad_move.to()).is_present() || bad_move.type() == moves::MoveType::EnPassant;
                bool bad_promo = bad_move.type() == moves::MoveType::Promotion;
                if (!bad_capture && !bad_promo) {
                    pieces::Piece bp = board.piece_at(bad_move.from());
                    if (bp.is_present()) {
                        int bc_idx = (bp.color() == pieces::Color::White) ? 0 : 1;
                        int bp_idx = get_piece_index(bp.type());
                        int& hist = context.history_moves[bc_idx][bp_idx][bad_move.to().index()];
                        hist -= history_change + (hist * std::abs(history_change) / 16384);
                    }

                    if (g_enable_conthist && bp.is_present() && bp.type() != pieces::PieceType::King) {
                        int bad_p_idx = get_piece_index(bp.type());
                        if (context.move_history.size() >= 1) {
                            const auto& prev = context.move_history.back();
                            pieces::Piece prev_p = board.piece_at(prev.to());
                            if (prev_p.is_present() && prev_p.type() != pieces::PieceType::King) {
                                int prev_p_idx = get_piece_index(prev_p.type());
                                int& conthist = context.continuation_history_1->data[prev_p_idx][prev.to().index()][bad_p_idx][bad_move.to().index()];
                                conthist -= history_change + (conthist * std::abs(history_change) / 16384);
                            }
                        }
                        if (context.move_history.size() >= 2) {
                            const auto& prev_prev = context.move_history[context.move_history.size() - 2];
                            pieces::Piece prev_prev_p = board.piece_at(prev_prev.to());
                            if (prev_prev_p.is_present() && prev_prev_p.type() != pieces::PieceType::King) {
                                int prev_prev_p_idx = get_piece_index(prev_prev_p.type());
                                int& conthist = context.continuation_history_2->data[prev_prev_p_idx][prev_prev.to().index()][bad_p_idx][bad_move.to().index()];
                                conthist -= history_change + (conthist * std::abs(history_change) / 16384);
                            }
                        }
                    }
                }
            }

            break;
        }
    }

    // Node-Type analysis counting (Module 7)
    if (has_beta_cutoff) {
        info.cut_nodes_count++;
    } else {
        info.all_nodes_count++;
    }

    context.tt.store(board.zobrist_hash(), depth, ply, best_score, bound, best_move);
    return best_score;
}

} // namespace chess::search
