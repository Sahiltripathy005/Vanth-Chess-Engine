#ifndef CHESS_RATING_LAB_HPP
#define CHESS_RATING_LAB_HPP

#include "chess/board/board.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <map>

namespace chess::validation {

enum class TimeControlMode {
    Depth,
    Nodes,
    MoveTime,
    Increment,
    Tournament
};

struct TimeControl {
    TimeControlMode mode = TimeControlMode::Depth;
    int limit = 4;        // Depth, nodes limit, or time limit in ms
    int increment = 0;    // Time increment in ms
};

struct RatingRecord {
    std::string name;
    std::string version = "3.5";
    std::string commit_hash = "dev";
    std::string search_config = "default";
    std::string eval_config = "default";
    double elo = 1500.0;
    double elo_error = 0.0;
    int games = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
    std::string date = "";
};

struct SPRTResults {
    double llr = 0.0;
    double lBound = 0.0;
    double uBound = 0.0;
    std::string status = "CONTINUE"; // ACCEPT, REJECT, CONTINUE
};

class EloCalculator {
public:
    static double expected_score(double rating_diff);
    static double rating_difference(double score);
    static void calculate_elo(int wins, int losses, int draws, double& elo_diff, double& elo_error);
};

class SPRT {
public:
    static SPRTResults check(int wins, int losses, int draws, double elo0, double elo1, double alpha = 0.05, double beta = 0.05);
};

struct TournamentConfig {
    std::string type = "self_play"; // "self_play", "round_robin", "gauntlet", "swiss"
    std::vector<std::string> player_names;
    std::vector<std::string> configs;
    TimeControl tc;
    int rounds = 1;
    bool alternate_colors = true;
    std::string opening_suite_file = "";
    double sprt_elo0 = 0.0;
    double sprt_elo1 = 0.0;
    bool use_sprt = false;
};

struct GameRecord {
    std::string white;
    std::string black;
    std::string result; // "1-0", "0-1", "1/2-1/2"
    int moves_count = 0;
    std::string pgn = "";
    std::string draw_reason = "";
    int time_ms = 0;
};

struct TournamentResults {
    std::string id;
    std::string type;
    std::vector<GameRecord> games;
    std::map<std::string, RatingRecord> final_ratings;
    double avg_game_length = 0.0;
    int mate_count = 0;
    int draw_50_count = 0;
    int draw_rep_count = 0;
    int draw_material_count = 0;
    int total_games = 0;
    bool sprt_terminated = false;
    std::string sprt_status = "CONTINUE";
};

class RatingDatabase {
public:
    static bool load_database(const std::string& filepath = "ratings_db.json");
    static bool save_database(const std::string& filepath = "ratings_db.json");
    static std::vector<RatingRecord> get_leaderboard();
    static void add_or_update_rating(const RatingRecord& record);
    static std::vector<RatingRecord> get_history(const std::string& name);
    static void clear();
private:
    static std::vector<RatingRecord> s_records;
};

class RatingLab {
public:
    static TournamentResults run_tournament(const TournamentConfig& config);
    static void export_to_pgn(const TournamentResults& results, const std::string& filepath);
    static void export_to_csv(const TournamentResults& results, const std::string& filepath);
    static std::string export_to_json(const TournamentResults& results);
    static void run_patch_comparison(const std::string& base_config, const std::string& patch_config, int games, TimeControl tc);
};

} // namespace chess::validation

#endif // CHESS_RATING_LAB_HPP
