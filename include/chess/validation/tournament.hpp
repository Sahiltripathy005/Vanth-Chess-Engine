#ifndef CHESS_TOURNAMENT_HPP
#define CHESS_TOURNAMENT_HPP

#include "chess/board/board.hpp"
#include <string>
#include <vector>
#include <chrono>

namespace chess::validation {

struct MatchPlayer {
    std::string name;
    int max_depth = 4;
    std::chrono::milliseconds time_limit = std::chrono::milliseconds(50);
    std::string config_file = ""; // File to load weights from. Empty means current weights.
};

struct MatchResults {
    int player1_wins = 0;
    int player2_wins = 0;
    int draws = 0;
    int total_games = 0;
    double avg_game_length = 0.0;
    double avg_depth_reached = 0.0;
    uint64_t avg_nodes_searched = 0;
};

class TournamentManager {
public:
    // Plays a single game between two players, returning the winning status (1 = player1, 2 = player2, 0 = draw)
    static int play_game(const MatchPlayer& white, const MatchPlayer& black, 
                         std::vector<moves::Move>& game_moves, MatchResults& out_stats);

    // Runs a batch tournament (match) of multiple games between player1 and player2, alternating colors
    static MatchResults run_match(const MatchPlayer& p1, const MatchPlayer& p2, int game_count);

    // Run automated self-play for validation
    static MatchResults run_self_play(int game_count, int depth, std::chrono::milliseconds time_limit);
};

} // namespace chess::validation

#endif // CHESS_TOURNAMENT_HPP
