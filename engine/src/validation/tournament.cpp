#include "chess/validation/tournament.hpp"
#include "chess/validation/tuning.hpp"
#include "chess/search/search.hpp"
#include "chess/search/negamax.hpp"
#include "chess/moves/move_generator.hpp"
#include <iostream>
#include <algorithm>

namespace chess::validation {

int TournamentManager::play_game(const MatchPlayer& white, const MatchPlayer& black, 
                                 std::vector<moves::Move>& game_moves, MatchResults& out_stats) {
    board::Board board = board::Board::start_position();
    std::vector<uint64_t> hash_history;
    hash_history.push_back(board.zobrist_hash());

    uint64_t total_nodes = 0;
    uint64_t total_depth = 0;
    int moves_played = 0;

    int result = 0; // 0 = draw, 1 = white, 2 = black
    bool game_active = true;

    while (game_active) {
        // Check moves limit to avoid infinite game
        if (moves_played >= 150) {
            result = 0;
            break;
        }

        std::vector<moves::Move> legal_moves = moves::MoveGenerator::generate_legal_moves(board);

        // 1. Checkmate / Stalemate
        if (legal_moves.empty()) {
            if (moves::MoveGenerator::is_in_check(board, board.active_color())) {
                // Checkmate! The other side won
                result = (board.active_color() == pieces::Color::White) ? 2 : 1;
            } else {
                // Stalemate
                result = 0;
            }
            break;
        }

        // 2. 50-move rule
        if (board.halfmove_clock() >= 100) {
            result = 0;
            break;
        }

        // 3. Threefold Repetition
        uint64_t current_hash = board.zobrist_hash();
        int rep_count = 0;
        for (uint64_t h : hash_history) {
            if (h == current_hash) {
                rep_count++;
            }
        }
        if (rep_count >= 3) {
            result = 0;
            break;
        }

        // Setup parameters for active side
        const MatchPlayer& current_player = (board.active_color() == pieces::Color::White) ? white : black;
        if (!current_player.config_file.empty()) {
            TuningManager::get().load_weights(current_player.config_file);
        } else {
            TuningManager::get().reset_to_defaults();
        }

        // Toggle search parameters dynamically based on player name (Module 9)
        if (current_player.name.find("Baseline") != std::string::npos ||
            current_player.name.find("V3.6") != std::string::npos ||
            current_player.name.find("Base") != std::string::npos) {
            search::g_enable_v36_ordering = false;
            search::g_enable_conthist = false;
            search::g_enable_countermoves = false;
            search::g_enable_lmr = false;
            search::g_enable_nmp = false;
            search::g_search_config.mdp_enable = false;
            search::g_search_config.iid_depth = 99;
        } else {
            search::g_enable_v36_ordering = true;
            search::g_enable_conthist = true;
            search::g_enable_countermoves = true;
            search::g_enable_lmr = true;
            search::g_enable_nmp = true;
            search::g_search_config.mdp_enable = true;
            search::g_search_config.iid_depth = 5;
        }

        // Search
        int limit_ms = static_cast<int>(current_player.time_limit.count());
        auto info = search::find_best_move(board, current_player.max_depth, limit_ms);

        if (info.best_move.from() == pieces::Square::None()) {
            // Pick a fallback legal move if search failed to return anything
            info.best_move = legal_moves[0];
        }

        // Accumulate statistics
        total_nodes += info.nodes;
        total_depth += info.max_depth;

        // Apply move
        board.make_move(info.best_move);
        game_moves.push_back(info.best_move);
        hash_history.push_back(board.zobrist_hash());
        moves_played++;
    }

    // Reset to defaults at end of game
    TuningManager::get().reset_to_defaults();

    // Fill output statistics for this single game
    out_stats = MatchResults();
    out_stats.total_games = 1;
    if (result == 1) {
        out_stats.player1_wins = 1;
    } else if (result == 2) {
        out_stats.player2_wins = 1;
    } else {
        out_stats.draws = 1;
    }

    out_stats.avg_game_length = moves_played;
    if (moves_played > 0) {
        out_stats.avg_depth_reached = static_cast<double>(total_depth) / moves_played;
        out_stats.avg_nodes_searched = total_nodes / moves_played;
    }

    return result;
}

MatchResults TournamentManager::run_match(const MatchPlayer& p1, const MatchPlayer& p2, int game_count) {
    MatchResults results;

    for (int i = 0; i < game_count; ++i) {
        std::vector<moves::Move> game_moves;
        MatchResults game_res;
        
        bool p1_is_white = (i % 2 == 0);
        int winner = 0;
        
        if (p1_is_white) {
            winner = play_game(p1, p2, game_moves, game_res);
        } else {
            winner = play_game(p2, p1, game_moves, game_res);
        }
        
        results.total_games++;
        
        if (winner == 1) {
            // White won
            if (p1_is_white) results.player1_wins++;
            else results.player2_wins++;
        } else if (winner == 2) {
            // Black won
            if (p1_is_white) results.player2_wins++;
            else results.player1_wins++;
        } else {
            // Draw
            results.draws++;
        }

        // Weighted average update
        double weight = 1.0 / results.total_games;
        results.avg_game_length = results.avg_game_length * (1.0 - weight) + game_res.avg_game_length * weight;
        results.avg_depth_reached = results.avg_depth_reached * (1.0 - weight) + game_res.avg_depth_reached * weight;
        results.avg_nodes_searched = static_cast<uint64_t>(results.avg_nodes_searched * (1.0 - weight) + game_res.avg_nodes_searched * weight);
    }

    return results;
}

MatchResults TournamentManager::run_self_play(int game_count, int depth, std::chrono::milliseconds time_limit) {
    MatchPlayer p1{"Vanth_Baseline", depth, time_limit, ""};
    MatchPlayer p2{"Vanth_Candidate", depth, time_limit, ""};
    return run_match(p1, p2, game_count);
}

} // namespace chess::validation
