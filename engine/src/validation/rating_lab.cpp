#include "chess/validation/rating_lab.hpp"
#include "chess/validation/tournament.hpp"
#include "chess/validation/tuning.hpp"
#include "chess/search/search.hpp"
#include "chess/moves/move_generator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <random>
#include <iomanip>
#include <ctime>

namespace chess::validation {

std::vector<RatingRecord> RatingDatabase::s_records;

// --- ELO CALCULATOR ---

double EloCalculator::expected_score(double rating_diff) {
    return 1.0 / (1.0 + std::pow(10.0, -rating_diff / 400.0));
}

double EloCalculator::rating_difference(double score) {
    if (score <= 0.0) return -1000.0;
    if (score >= 1.0) return 1000.0;
    return -400.0 * std::log10(1.0 / score - 1.0);
}

void EloCalculator::calculate_elo(int wins, int losses, int draws, double& elo_diff, double& elo_error) {
    int n = wins + losses + draws;
    if (n == 0) {
        elo_diff = 0.0;
        elo_error = 0.0;
        return;
    }

    double score = (wins + 0.5 * draws) / n;
    elo_diff = rating_difference(score);

    if (n <= 1) {
        elo_error = 0.0;
        return;
    }

    // Variance calculation
    double var = 0.0;
    var += wins * std::pow(1.0 - score, 2);
    var += losses * std::pow(0.0 - score, 2);
    var += draws * std::pow(0.5 - score, 2);
    var /= (n - 1);

    double sigma = std::sqrt(var);
    double se = sigma / std::sqrt(n);

    // 95% Confidence Interval limit for score, transformed back to Elo
    double score_upper = std::min(0.9999, score + 1.96 * se);
    double score_lower = std::max(0.0001, score - 1.96 * se);

    double elo_upper = rating_difference(score_upper);
    double elo_lower = rating_difference(score_lower);

    elo_error = (elo_upper - elo_lower) / 2.0;
}

// --- SPRT ---

SPRTResults SPRT::check(int wins, int losses, int draws, double elo0, double elo1, double alpha, double beta) {
    int n = wins + losses + draws;
    SPRTResults res;
    if (n == 0) {
        res.llr = 0.0;
        res.lBound = std::log(beta / (1.0 - alpha));
        res.uBound = std::log((1.0 - beta) / alpha);
        res.status = "CONTINUE";
        return res;
    }

    double s0 = EloCalculator::expected_score(elo0);
    double s1 = EloCalculator::expected_score(elo1);

    double points = wins + 0.5 * draws;
    double score = points / n;

    // Variance of the trinomial score
    double var = 0.0;
    var += wins * std::pow(1.0 - score, 2);
    var += losses * std::pow(0.0 - score, 2);
    var += draws * std::pow(0.5 - score, 2);
    var /= n;

    if (var < 0.001) var = 0.001; // Safeguard against zero variance

    // Compute Log-Likelihood Ratio (LLR) using normal approximation
    res.llr = ((s1 - s0) / var) * (points - n * (s0 + s1) / 2.0);
    res.lBound = std::log(beta / (1.0 - alpha));
    res.uBound = std::log((1.0 - beta) / alpha);

    if (res.llr >= res.uBound) {
        res.status = "ACCEPT";
    } else if (res.llr <= res.lBound) {
        res.status = "REJECT";
    } else {
        res.status = "CONTINUE";
    }

    return res;
}

// --- RATING DATABASE ---

bool RatingDatabase::load_database(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    s_records.clear();
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    size_t pos = 0;
    while (true) {
        pos = content.find("{", pos);
        if (pos == std::string::npos) break;

        size_t end_pos = content.find("}", pos);
        if (end_pos == std::string::npos) break;

        std::string object_str = content.substr(pos, end_pos - pos + 1);
        pos = end_pos + 1;

        auto get_str_val = [](const std::string& obj, const std::string& key) -> std::string {
            size_t kpos = obj.find("\"" + key + "\"");
            if (kpos == std::string::npos) return "";
            size_t vstart = obj.find("\"", kpos + key.size() + 2);
            if (vstart == std::string::npos) return "";
            size_t vend = obj.find("\"", vstart + 1);
            if (vend == std::string::npos) return "";
            return obj.substr(vstart + 1, vend - vstart - 1);
        };

        auto get_num_val = [](const std::string& obj, const std::string& key) -> double {
            size_t kpos = obj.find("\"" + key + "\"");
            if (kpos == std::string::npos) return 0.0;
            size_t cpos = obj.find(":", kpos);
            if (cpos == std::string::npos) return 0.0;
            size_t vend = obj.find(",", cpos);
            if (vend == std::string::npos) vend = obj.find("}", cpos);
            if (vend == std::string::npos) return 0.0;
            std::string val_str = obj.substr(cpos + 1, vend - cpos - 1);
            try {
                return std::stod(val_str);
            } catch (...) {
                return 0.0;
            }
        };

        RatingRecord rec;
        rec.name = get_str_val(object_str, "name");
        if (rec.name.empty()) continue;

        rec.version = get_str_val(object_str, "version");
        rec.commit_hash = get_str_val(object_str, "commit_hash");
        rec.search_config = get_str_val(object_str, "search_config");
        rec.eval_config = get_str_val(object_str, "eval_config");
        rec.elo = get_num_val(object_str, "elo");
        rec.elo_error = get_num_val(object_str, "elo_error");
        rec.games = static_cast<int>(get_num_val(object_str, "games"));
        rec.wins = static_cast<int>(get_num_val(object_str, "wins"));
        rec.losses = static_cast<int>(get_num_val(object_str, "losses"));
        rec.draws = static_cast<int>(get_num_val(object_str, "draws"));
        rec.date = get_str_val(object_str, "date");

        s_records.push_back(rec);
    }
    return true;
}

bool RatingDatabase::save_database(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < s_records.size(); ++i) {
        const auto& rec = s_records[i];
        file << "  {\n"
             << "    \"name\": \"" << rec.name << "\",\n"
             << "    \"version\": \"" << rec.version << "\",\n"
             << "    \"commit_hash\": \"" << rec.commit_hash << "\",\n"
             << "    \"search_config\": \"" << rec.search_config << "\",\n"
             << "    \"eval_config\": \"" << rec.eval_config << "\",\n"
             << "    \"elo\": " << rec.elo << ",\n"
             << "    \"elo_error\": " << rec.elo_error << ",\n"
             << "    \"games\": " << rec.games << ",\n"
             << "    \"wins\": " << rec.wins << ",\n"
             << "    \"losses\": " << rec.losses << ",\n"
             << "    \"draws\": " << rec.draws << ",\n"
             << "    \"date\": \"" << rec.date << "\"\n"
             << "  }" << (i + 1 < s_records.size() ? "," : "") << "\n";
    }
    file << "]\n";
    file.close();
    return true;
}

std::vector<RatingRecord> RatingDatabase::get_leaderboard() {
    auto sorted = s_records;
    std::sort(sorted.begin(), sorted.end(), [](const RatingRecord& a, const RatingRecord& b) {
        return a.elo > b.elo;
    });
    return sorted;
}

void RatingDatabase::add_or_update_rating(const RatingRecord& record) {
    for (auto& rec : s_records) {
        if (rec.name == record.name) {
            rec = record;
            return;
        }
    }
    s_records.push_back(record);
}

std::vector<RatingRecord> RatingDatabase::get_history(const std::string& name) {
    std::vector<RatingRecord> hist;
    for (const auto& rec : s_records) {
        if (rec.name == name) {
            hist.push_back(rec);
        }
    }
    return hist;
}

void RatingDatabase::clear() {
    s_records.clear();
}

// --- TOURNAMENT GAME HELPER ---

static int play_game_with_tc(const MatchPlayer& white, const MatchPlayer& black, TimeControl tc, 
                             std::vector<moves::Move>& game_moves, std::string& draw_reason, int& total_time_ms, const std::string& opening_fen = "") {
    board::Board board;
    if (!opening_fen.empty()) {
        board.load_fen(opening_fen);
    } else {
        board = board::Board::start_position();
    }
    
    std::vector<uint64_t> hash_history;
    hash_history.push_back(board.zobrist_hash());
    
    int moves_played = 0;
    int result = 0; // 0 = draw, 1 = white, 2 = black
    
    // Initialize clocks in ms
    int w_time = 30000;
    int b_time = 30000;
    int w_inc = 0;
    int b_inc = 0;
    
    if (tc.mode == TimeControlMode::Increment || tc.mode == TimeControlMode::Tournament) {
        w_time = tc.limit;
        b_time = tc.limit;
        w_inc = tc.increment;
        b_inc = tc.increment;
    } else if (tc.mode == TimeControlMode::MoveTime) {
        w_time = tc.limit * 40;
        b_time = tc.limit * 40;
    }
    
    auto start_game_time = std::chrono::high_resolution_clock::now();
    
    while (true) {
        if (moves_played >= 150) {
            result = 0;
            draw_reason = "150 moves rule limit";
            break;
        }
        
        std::vector<moves::Move> legal_moves = moves::MoveGenerator::generate_legal_moves(board);
        if (legal_moves.empty()) {
            if (moves::MoveGenerator::is_in_check(board, board.active_color())) {
                result = (board.active_color() == pieces::Color::White) ? 2 : 1;
                draw_reason = "checkmate";
            } else {
                result = 0;
                draw_reason = "stalemate";
            }
            break;
        }
        
        if (board.halfmove_clock() >= 100) {
            result = 0;
            draw_reason = "50-move rule";
            break;
        }
        
        // Repetition check
        uint64_t current_hash = board.zobrist_hash();
        int rep_count = 0;
        for (uint64_t h : hash_history) {
            if (h == current_hash) rep_count++;
        }
        if (rep_count >= 3) {
            result = 0;
            draw_reason = "threefold repetition";
            break;
        }
        
        // Insufficient material check
        int w_pieces = 0, b_pieces = 0;
        int w_bishops = 0, w_knights = 0, b_bishops = 0, b_knights = 0;
        bool raw_pawns_or_major = false;
        
        for (int sq = 0; sq < 64; ++sq) {
            pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(sq)));
            if (p.is_none()) continue;
            if (p.type() == pieces::PieceType::King) continue;
            if (p.type() == pieces::PieceType::Pawn || p.type() == pieces::PieceType::Rook || p.type() == pieces::PieceType::Queen) {
                raw_pawns_or_major = true;
                break;
            }
            if (p.color() == pieces::Color::White) {
                w_pieces++;
                if (p.type() == pieces::PieceType::Bishop) w_bishops++;
                else if (p.type() == pieces::PieceType::Knight) w_knights++;
            } else {
                b_pieces++;
                if (p.type() == pieces::PieceType::Bishop) b_bishops++;
                else if (p.type() == pieces::PieceType::Knight) b_knights++;
            }
        }
        
        if (!raw_pawns_or_major) {
            if ((w_pieces == 0 && b_pieces == 0) ||
                (w_pieces == 1 && w_bishops == 1 && b_pieces == 0) ||
                (w_pieces == 1 && w_knights == 1 && b_pieces == 0) ||
                (b_pieces == 1 && b_bishops == 1 && w_pieces == 0) ||
                (b_pieces == 1 && b_knights == 1 && w_pieces == 0) ||
                (w_pieces == 1 && w_bishops == 1 && b_pieces == 1 && b_bishops == 1)) {
                result = 0;
                draw_reason = "insufficient material";
                break;
            }
        }
        
        // Setup parameters for active side
        const MatchPlayer& current_player = (board.active_color() == pieces::Color::White) ? white : black;
        if (!current_player.config_file.empty()) {
            TuningManager::get().load_weights(current_player.config_file);
        } else {
            TuningManager::get().reset_to_defaults();
        }
        
        search::SearchInfo info;
        auto move_start = std::chrono::high_resolution_clock::now();
        
        if (tc.mode == TimeControlMode::Depth) {
            info = search::find_best_move(board, tc.limit, 0);
        } else if (tc.mode == TimeControlMode::MoveTime) {
            info = search::find_best_move(board, 64, tc.limit);
        } else if (tc.mode == TimeControlMode::Nodes) {
            info = search::find_best_move(board, 3, 0); // node simulation via depth
        } else if (tc.mode == TimeControlMode::Increment || tc.mode == TimeControlMode::Tournament) {
            int active_clock = (board.active_color() == pieces::Color::White) ? w_time : b_time;
            int active_inc = (board.active_color() == pieces::Color::White) ? w_inc : b_inc;
            info = search::find_best_move(board, 64, 0, active_clock, active_inc);
        }
        
        auto move_end = std::chrono::high_resolution_clock::now();
        int time_spent = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(move_end - move_start).count());
        
        if (tc.mode == TimeControlMode::Increment || tc.mode == TimeControlMode::Tournament) {
            if (board.active_color() == pieces::Color::White) {
                w_time -= time_spent;
                if (w_time < 0) {
                    result = 2; // Black wins
                    draw_reason = "loss on time";
                    break;
                }
                w_time += w_inc;
            } else {
                b_time -= time_spent;
                if (b_time < 0) {
                    result = 1; // White wins
                    draw_reason = "loss on time";
                    break;
                }
                b_time += b_inc;
            }
        }
        
        if (info.best_move.from() == pieces::Square::None()) {
            info.best_move = legal_moves[0];
        }
        
        board.make_move(info.best_move);
        game_moves.push_back(info.best_move);
        hash_history.push_back(board.zobrist_hash());
        moves_played++;
    }
    
    TuningManager::get().reset_to_defaults();
    auto end_game_time = std::chrono::high_resolution_clock::now();
    total_time_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(end_game_time - start_game_time).count());
    
    return result;
}

// --- RATING LAB ---

TournamentResults RatingLab::run_tournament(const TournamentConfig& config) {
    TournamentResults results;
    results.id = "T_" + std::to_string(std::time(nullptr));
    results.type = config.type;
    
    // Balanced opening FEN list
    std::vector<std::string> opening_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Start position
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", // Sicilian Defense
        "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 0 3", // Ruy Lopez
        "rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", // French Defense
        "rnbqkbnr/pp1ppppp/2p5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2"  // Caro-Kann
    };
    
    if (!config.opening_suite_file.empty()) {
        std::ifstream file(config.opening_suite_file);
        if (file.is_open()) {
            opening_fens.clear();
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line[0] != '#') opening_fens.push_back(line);
            }
        }
    }
    
    // Initialize results ratings structure
    for (const auto& name : config.player_names) {
        RatingRecord rec;
        rec.name = name;
        rec.date = "2026-07-11";
        results.final_ratings[name] = rec;
    }
    
    // Generate pairings depending on type
    struct MatchPair {
        std::string p1;
        std::string p2;
        std::string config1;
        std::string config2;
        std::string fen;
    };
    
    std::vector<MatchPair> pairings;
    
    if (config.type == "self_play" || config.type == "compare") {
        if (config.player_names.size() >= 2) {
            std::string p1 = config.player_names[0];
            std::string p2 = config.player_names[1];
            std::string c1 = config.configs.size() > 0 ? config.configs[0] : "";
            std::string c2 = config.configs.size() > 1 ? config.configs[1] : "";
            
            for (int r = 0; r < config.rounds; ++r) {
                std::string fen = opening_fens[r % opening_fens.size()];
                pairings.push_back({p1, p2, c1, c2, fen});
                if (config.alternate_colors) {
                    pairings.push_back({p2, p1, c2, c1, fen}); // Mirror color run
                }
            }
        }
    } else if (config.type == "round_robin" || config.type == "swiss") {
        // Round robin pairing
        for (size_t i = 0; i < config.player_names.size(); ++i) {
            for (size_t j = i + 1; j < config.player_names.size(); ++j) {
                std::string p1 = config.player_names[i];
                std::string p2 = config.player_names[j];
                std::string c1 = config.configs.size() > i ? config.configs[i] : "";
                std::string c2 = config.configs.size() > j ? config.configs[j] : "";
                
                for (int r = 0; r < config.rounds; ++r) {
                    std::string fen = opening_fens[r % opening_fens.size()];
                    pairings.push_back({p1, p2, c1, c2, fen});
                    if (config.alternate_colors) {
                        pairings.push_back({p2, p1, c2, c1, fen});
                    }
                }
            }
        }
    } else if (config.type == "gauntlet") {
        // Gauntlet pairing: Player 0 against everyone else
        if (!config.player_names.empty()) {
            std::string candidate = config.player_names[0];
            std::string c_cand = config.configs.size() > 0 ? config.configs[0] : "";
            
            for (size_t j = 1; j < config.player_names.size(); ++j) {
                std::string opponent = config.player_names[j];
                std::string c_opp = config.configs.size() > j ? config.configs[j] : "";
                
                for (int r = 0; r < config.rounds; ++r) {
                    std::string fen = opening_fens[r % opening_fens.size()];
                    pairings.push_back({candidate, opponent, c_cand, c_opp, fen});
                    if (config.alternate_colors) {
                        pairings.push_back({opponent, candidate, c_opp, c_cand, fen});
                    }
                }
            }
        }
    }
    
    int total_moves = 0;
    int game_index = 0;
    int p1_wins = 0, p2_wins = 0, draws = 0;
    
    for (const auto& pairing : pairings) {
        MatchPlayer w{pairing.p1, config.tc.limit, std::chrono::milliseconds(50), pairing.config1};
        MatchPlayer b{pairing.p2, config.tc.limit, std::chrono::milliseconds(50), pairing.config2};
        
        std::vector<moves::Move> moves;
        std::string draw_reason = "";
        int time_ms = 0;
        
        int winner = play_game_with_tc(w, b, config.tc, moves, draw_reason, time_ms, pairing.fen);
        results.total_games++;
        
        GameRecord game;
        game.white = pairing.p1;
        game.black = pairing.p2;
        game.moves_count = static_cast<int>(moves.size());
        game.time_ms = time_ms;
        game.draw_reason = draw_reason;
        
        total_moves += game.moves_count;
        
        if (winner == 1) {
            game.result = "1-0";
            results.final_ratings[pairing.p1].wins++;
            results.final_ratings[pairing.p2].losses++;
            if (pairing.p1 == config.player_names[0]) p1_wins++;
            else p2_wins++;
        } else if (winner == 2) {
            game.result = "0-1";
            results.final_ratings[pairing.p1].losses++;
            results.final_ratings[pairing.p2].wins++;
            if (pairing.p1 == config.player_names[0]) p2_wins++;
            else p1_wins++;
        } else {
            game.result = "1/2-1/2";
            results.final_ratings[pairing.p1].draws++;
            results.final_ratings[pairing.p2].draws++;
            draws++;
            
            if (draw_reason == "50-move rule") results.draw_50_count++;
            else if (draw_reason == "threefold repetition") results.draw_rep_count++;
            else if (draw_reason == "insufficient material") results.draw_material_count++;
        }
        
        results.final_ratings[pairing.p1].games++;
        results.final_ratings[pairing.p2].games++;
        
        // Format simple PGN output
        std::stringstream pgn_ss;
        pgn_ss << "[Event \"Vanth Rating Lab Tournament\"]\n"
               << "[Site \"Local Lab\"]\n"
               << "[Date \"2026.07.11\"]\n"
               << "[Round \"" << (game_index + 1) << "\"]\n"
               << "[White \"" << pairing.p1 << "\"]\n"
               << "[Black \"" << pairing.p2 << "\"]\n"
               << "[Result \"" << game.result << "\"]\n"
               << "[Termination \"" << draw_reason << "\"]\n\n";
        
        for (size_t m = 0; m < moves.size(); ++m) {
            if (m % 2 == 0) pgn_ss << (m / 2 + 1) << ". ";
            pgn_ss << moves[m].to_string() << " ";
        }
        pgn_ss << "\n";
        game.pgn = pgn_ss.str();
        results.games.push_back(game);
        
        game_index++;
        
        // SPRT check if enabled
        if (config.use_sprt && config.player_names.size() >= 2) {
            auto sprt_res = SPRT::check(p1_wins, p2_wins, draws, config.sprt_elo0, config.sprt_elo1);
            if (sprt_res.status != "CONTINUE") {
                results.sprt_terminated = true;
                results.sprt_status = sprt_res.status;
                break; // Auto-terminate match queue
            }
        }
    }
    
    if (results.total_games > 0) {
        results.avg_game_length = static_cast<double>(total_moves) / results.total_games;
    }
    
    // Calculate final ELO values
    for (auto& pair : results.final_ratings) {
        double elo_diff = 0.0;
        double elo_error = 0.0;
        EloCalculator::calculate_elo(pair.second.wins, pair.second.losses, pair.second.draws, elo_diff, elo_error);
        pair.second.elo = 1500.0 + elo_diff;
        pair.second.elo_error = elo_error;
    }
    
    return results;
}

void RatingLab::export_to_pgn(const TournamentResults& results, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    for (const auto& game : results.games) {
        file << game.pgn << "\n\n";
    }
    file.close();
}

void RatingLab::export_to_csv(const TournamentResults& results, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    file << "White,Black,Result,Moves,Time(ms),TerminationReason\n";
    for (const auto& game : results.games) {
        file << game.white << ","
             << game.black << ","
             << game.result << ","
             << game.moves_count << ","
             << game.time_ms << ","
             << game.draw_reason << "\n";
    }
    file.close();
}

std::string RatingLab::export_to_json(const TournamentResults& results) {
    std::stringstream ss;
    ss << "{\n"
       << "  \"tournament_id\": \"" << results.id << "\",\n"
       << "  \"type\": \"" << results.type << "\",\n"
       << "  \"total_games\": " << results.total_games << ",\n"
       << "  \"avg_game_length\": " << results.avg_game_length << ",\n"
       << "  \"draws_50_count\": " << results.draw_50_count << ",\n"
       << "  \"draws_rep_count\": " << results.draw_rep_count << ",\n"
       << "  \"draws_material_count\": " << results.draw_material_count << ",\n"
       << "  \"sprt_terminated\": " << (results.sprt_terminated ? "true" : "false") << ",\n"
       << "  \"sprt_status\": \"" << results.sprt_status << "\",\n"
       << "  \"ratings\": [\n";
    
    size_t r_idx = 0;
    for (const auto& pair : results.final_ratings) {
        const auto& rec = pair.second;
        ss << "    {\n"
           << "      \"name\": \"" << rec.name << "\",\n"
           << "      \"elo\": " << rec.elo << ",\n"
           << "      \"elo_error\": " << rec.elo_error << ",\n"
           << "      \"wins\": " << rec.wins << ",\n"
           << "      \"losses\": " << rec.losses << ",\n"
           << "      \"draws\": " << rec.draws << ",\n"
           << "      \"games\": " << rec.games << "\n"
           << "    }" << (r_idx + 1 < results.final_ratings.size() ? "," : "") << "\n";
        r_idx++;
    }
    ss << "  ]\n"
       << "}\n";
    return ss.str();
}

void RatingLab::run_patch_comparison(const std::string& base_config, const std::string& patch_config, int games, TimeControl tc) {
    TournamentConfig config;
    config.type = "compare";
    config.player_names = { "Vanth_Candidate", "Vanth_Baseline" };
    config.configs = { patch_config, base_config };
    config.tc = tc;
    config.rounds = games;
    config.alternate_colors = true;
    
    std::cout << "Starting Patch Comparison: Vanth_Candidate vs Vanth_Baseline...\n";
    auto results = run_tournament(config);
    
    std::cout << "========================================\n";
    std::cout << "        Tournament Match Finished       \n";
    std::cout << "========================================\n";
    std::cout << "Total Games Played: " << results.total_games << "\n";
    
    auto cand = results.final_ratings["Vanth_Candidate"];
    auto base = results.final_ratings["Vanth_Baseline"];
    
    double win_rate = static_cast<double>(cand.wins) / results.total_games * 100.0;
    double draw_rate = static_cast<double>(cand.draws) / results.total_games * 100.0;
    
    std::cout << "\nCandidate Performance:\n"
             << "  Wins:   " << cand.wins << " (" << std::fixed << std::setprecision(1) << win_rate << "%)\n"
             << "  Losses: " << cand.losses << "\n"
             << "  Draws:  " << cand.draws << " (" << draw_rate << "%)\n"
             << "  Elo:    " << cand.elo << " +/- " << cand.elo_error << " (vs Baseline: " << base.elo << ")\n";
             
    double elo_diff = cand.elo - base.elo;
    std::cout << "\nEstimated Elo Gain/Loss: " << std::showpos << elo_diff << " Elo\n";
    
    if (elo_diff > cand.elo_error) {
        std::cout << "\nRecommendation: ACCEPT patch (statistically significant gain)\n";
    } else if (elo_diff < -cand.elo_error) {
        std::cout << "\nRecommendation: REJECT patch (statistically significant loss)\n";
    } else {
        std::cout << "\nRecommendation: KEEP TESTING / REJECT (no significant Elo difference)\n";
    }
    std::cout << "========================================\n";
}

} // namespace chess::validation
