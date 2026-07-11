#include "chess/search/search.hpp"
#include "chess/search/negamax.hpp"
#include "chess/moves/move_generator.hpp"
#include "chess/search/time_manager.hpp"
#include "chess/uci/option_manager.hpp"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace chess::search {

SearchInfo find_best_move(
    board::Board& board,
    int depth,
    int limit_ms,
    int time_left_ms,
    int increment_ms,
    int moves_to_go,
    bool is_infinite
) {
    auto start_time = std::chrono::high_resolution_clock::now();

    SearchInfo info;
    info.max_depth = depth;
    info.nodes = 0;
    info.leaf_nodes = 0;
    info.cutoffs = 0;
    info.tt_hits = 0;
    info.tt_lookups = 0;
    info.fhf = 0;
    info.ordering_lookups = 0;
    info.q_nodes = 0;
    info.lmr_reductions = 0;
    info.nmp_cutoffs = 0;
    info.pvs_researches = 0;
    info.aborted = false;
    std::memset(info.killer_moves, 0, sizeof(info.killer_moves));
    std::memset(info.history_moves, 0, sizeof(info.history_moves));

    // Check game terminal conditions at root
    std::vector<moves::Move> root_moves = moves::MoveGenerator::generate_legal_moves(board);

    if (root_moves.empty()) {
        info.best_move = moves::Move();
        if (moves::MoveGenerator::is_in_check(board, board.active_color())) {
            info.best_score = (board.active_color() == pieces::Color::White) ? -kMateScore : kMateScore;
        } else {
            info.best_score = kDrawScore;
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        info.time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        return info;
    }

    TimeManager time_manager;
    time_manager.start(limit_ms, time_left_ms, increment_ms, moves_to_go, is_infinite);

    SearchContext context{g_tt, time_manager};
    context.clear_heuristics();

    // Start with the current position in repetition history
    context.board_history.push_back(board.zobrist_hash());

    moves::Move best_move = root_moves[0];
    int best_score = -kInfiniteScore;

    int multipv_setting = 1;
    try {
        multipv_setting = uci::OptionManager::get().get_spin("MultiPV");
    } catch (...) {
        multipv_setting = 1;
    }
    if (multipv_setting < 1) multipv_setting = 1;
    int multipv_count = std::min(multipv_setting, (int)root_moves.size());

    if (multipv_count > 1) {
        struct RootMoveInfo {
            moves::Move move;
            int score = -kInfiniteScore;
            std::vector<moves::Move> pv;
        };
        std::vector<RootMoveInfo> root_infos;
        for (const auto& m : root_moves) {
            root_infos.push_back({m, -kInfiniteScore, {m}});
        }

        for (int d = 1; d <= depth; ++d) {
            if (d > 1 && time_manager.should_stop_iterative_deepening()) {
                break;
            }

            for (auto& rinfo : root_infos) {
                auto state = board.make_move(rinfo.move);
                int score = -negamax(board, d - 1, 1, -kInfiniteScore, kInfiniteScore, info, context, rinfo.move);
                board.undo_move(rinfo.move, state);

                if (info.aborted) break;

                rinfo.score = score;

                // Rebuild PV
                rinfo.pv.clear();
                rinfo.pv.push_back(rinfo.move);
                auto state2 = board.make_move(rinfo.move);
                board::Board temp_board = board;
                for (int p = 1; p < d; ++p) {
                    moves::Move pv_move;
                    int pv_score;
                    if (g_tt.probe(temp_board.zobrist_hash(), 0, p, -kInfiniteScore, kInfiniteScore, pv_score, pv_move)) {
                        if (pv_move.from().is_none()) break;
                        std::vector<moves::Move> legal = moves::MoveGenerator::generate_legal_moves(temp_board);
                        bool legal_found = false;
                        for (const auto& lm : legal) {
                            if (lm == pv_move) {
                                legal_found = true;
                                break;
                            }
                        }
                        if (!legal_found) break;
                        rinfo.pv.push_back(pv_move);
                        temp_board.make_move(pv_move);
                    } else {
                        break;
                    }
                }
                board.undo_move(rinfo.move, state2);
            }

            if (info.aborted) break;

            std::sort(root_infos.begin(), root_infos.end(), [](const RootMoveInfo& a, const RootMoveInfo& b) {
                return a.score > b.score;
            });

            auto curr_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - start_time).count();
            uint64_t nps = (duration > 0) ? (info.nodes * 1000 / duration) : 0;

            for (int idx = 0; idx < multipv_count; ++idx) {
                const auto& rinfo = root_infos[idx];
                std::cout << "info depth " << d << " seldepth " << d << " multipv " << (idx + 1);
                int print_score = (board.active_color() == pieces::Color::White) ? rinfo.score : -rinfo.score;
                if (std::abs(print_score) > kMateScore - 1000) {
                    int mate_in_plies = kMateScore - std::abs(print_score);
                    int mate_in_moves = (mate_in_plies + 1) / 2;
                    if (print_score < 0) mate_in_moves = -mate_in_moves;
                    std::cout << " score mate " << mate_in_moves;
                } else {
                    std::cout << " score cp " << print_score;
                }
                std::cout << " nodes " << info.nodes << " nps " << nps << " time " << duration << " hashfull 0 pv";
                for (const auto& m : rinfo.pv) {
                    std::cout << " " << m.to_string();
                }
                std::cout << std::endl;
            }

            best_move = root_infos[0].move;
            best_score = root_infos[0].score;
            info.pv = root_infos[0].pv;

            SearchTimelinePoint pt;
            pt.depth = d;
            pt.duration_ms = duration;
            pt.nodes = info.nodes;
            pt.score = (board.active_color() == pieces::Color::White) ? best_score : -best_score;
            pt.best_move = best_move.to_string();
            info.timeline.push_back(pt);
        }

        info.alternatives.clear();
        for (int idx = 0; idx < multipv_count; ++idx) {
            int alt_score = (board.active_color() == pieces::Color::White) ? root_infos[idx].score : -root_infos[idx].score;
            info.alternatives.push_back({root_infos[idx].move, alt_score, root_infos[idx].pv});
        }
    } else {
        // Iterative Deepening Loop
        for (int d = 1; d <= depth; ++d) {
            // Check soft time limit
            if (d > 1 && time_manager.should_stop_iterative_deepening()) {
                break;
            }

            int score = 0;
            int alpha = -kInfiniteScore;
            int beta = kInfiniteScore;
            const int margin = 40; // 40 centipawns aspiration window margin

            if (d >= 5 && best_score > -kInfiniteScore + 1000 && best_score < kInfiniteScore - 1000) {
                alpha = best_score - margin;
                beta = best_score + margin;
            }

            int fail_count = 0;
            while (true) {
                score = negamax(board, d, 0, alpha, beta, info, context);

                if (info.aborted) {
                    break;
                }

                if (score <= alpha) {
                    if (alpha == -kInfiniteScore) {
                        break;
                    }
                    // Fail-low: expand alpha
                    alpha = std::max(-kInfiniteScore, alpha - margin * (1 << fail_count));
                    fail_count++;
                } else if (score >= beta) {
                    if (beta == kInfiniteScore) {
                        break;
                    }
                    // Fail-high: expand beta
                    beta = std::min(kInfiniteScore, beta + margin * (1 << fail_count));
                    fail_count++;
                } else {
                    // Score falls in window
                    break;
                }
            }

            if (info.aborted) {
                break; // Discard results of the aborted iteration
            }

            best_score = score;

            // Retrieve best move found from Transposition Table
            moves::Move tt_move;
            int tt_score;
            if (g_tt.probe(board.zobrist_hash(), d, 0, -kInfiniteScore, kInfiniteScore, tt_score, tt_move)) {
                if (!tt_move.from().is_none()) {
                    best_move = tt_move;
                    best_score = tt_score;
                }
            }

            // Extract the Principal Variation (PV)
            info.pv.clear();
            board::Board temp_board = board;
            // Follow the TT exact entries to rebuild the PV
            for (int p = 0; p < d; ++p) {
                moves::Move pv_move;
                int pv_score;
                if (g_tt.probe(temp_board.zobrist_hash(), 0, p, -kInfiniteScore, kInfiniteScore, pv_score, pv_move)) {
                    if (pv_move.from().is_none()) break;

                    // Validate pv_move is legal in temp_board
                    std::vector<moves::Move> legal = moves::MoveGenerator::generate_legal_moves(temp_board);
                    bool legal_found = false;
                    for (const auto& m : legal) {
                        if (m == pv_move) {
                            legal_found = true;
                            break;
                        }
                    }
                    if (!legal_found) break;

                    info.pv.push_back(pv_move);
                    temp_board.make_move(pv_move);
                } else {
                    break;
                }
            }

            // Print UCI standard info line
            auto curr_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - start_time).count();
            uint64_t nps = (duration > 0) ? (info.nodes * 1000 / duration) : 0;
            std::cout << "info depth " << d << " seldepth " << d;
            int print_score = (board.active_color() == pieces::Color::White) ? best_score : -best_score;
            if (std::abs(print_score) > kMateScore - 1000) {
                int mate_in_plies = kMateScore - std::abs(print_score);
                int mate_in_moves = (mate_in_plies + 1) / 2;
                if (print_score < 0) mate_in_moves = -mate_in_moves;
                std::cout << " score mate " << mate_in_moves;
            } else {
                std::cout << " score cp " << print_score;
            }
            std::cout << " nodes " << info.nodes << " nps " << nps << " time " << duration << " hashfull 0 pv";
            for (const auto& m : info.pv) {
                std::cout << " " << m.to_string();
            }
            std::cout << std::endl;

            SearchTimelinePoint pt;
            pt.depth = d;
            pt.duration_ms = duration;
            pt.nodes = info.nodes;
            pt.score = print_score;
            pt.best_move = best_move.to_string();
            info.timeline.push_back(pt);
        }
    }

    info.best_move = best_move;
    info.best_score = (board.active_color() == pieces::Color::White) ? best_score : -best_score;

    auto end_time = std::chrono::high_resolution_clock::now();
    info.time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Age the TT entries for replacement policy
    g_tt.increment_age();

    std::memcpy(info.killer_moves, context.killer_moves, sizeof(info.killer_moves));
    std::memcpy(info.history_moves, context.history_moves, sizeof(info.history_moves));

    return info;
}

} // namespace chess::search
