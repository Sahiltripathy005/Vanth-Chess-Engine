#include "chess/validation/tuning.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

namespace chess::validation {

TuningManager& TuningManager::get() {
    static TuningManager instance;
    return instance;
}

TuningManager::TuningManager() {
    register_parameters();
}

void TuningManager::add_param(const std::string& name, int* ptr, int min_val, int max_val, int step) {
    parameters_.push_back({name, ptr, *ptr, min_val, max_val, step});
}

void TuningManager::add_pst(const std::string& prefix, search::PST& pst, int min_val, int max_val, int step) {
    for (int i = 0; i < 64; ++i) {
        std::string name = prefix + "_" + std::to_string(i);
        add_param(name, &pst[i], min_val, max_val, step);
    }
}

void TuningManager::register_parameters() {
    parameters_.clear();

    // 1. Scalar constants
    add_param("bishop_pair_bonus", &search::kBishopPairBonus, 0, 100, 5);
    add_param("tempo_bonus", &search::kTempoBonus, 0, 50, 2);

    // 2. Pawn structure weights
    add_param("pawn_passed_pawn", &search::kPawnWeights.passed_pawn, 0, 100, 5);
    for (int r = 0; r < 8; ++r) {
        add_param("pawn_passed_pawn_rank_" + std::to_string(r), &search::kPawnWeights.passed_pawn_by_rank[r], 0, 200, 5);
    }
    add_param("pawn_isolated_pawn", &search::kPawnWeights.isolated_pawn, -50, 0, 5);
    add_param("pawn_doubled_pawn", &search::kPawnWeights.doubled_pawn, -50, 0, 5);
    add_param("pawn_backward_pawn", &search::kPawnWeights.backward_pawn, -50, 0, 5);
    add_param("pawn_connected_pawn", &search::kPawnWeights.connected_pawn, 0, 50, 2);
    add_param("pawn_pawn_chain", &search::kPawnWeights.pawn_chain, 0, 30, 2);
    add_param("pawn_candidate_passed", &search::kPawnWeights.candidate_passed, 0, 50, 5);
    add_param("pawn_blocked_pawn", &search::kPawnWeights.blocked_pawn, -50, 0, 5);
    add_param("pawn_central_pawn", &search::kPawnWeights.central_pawn, 0, 50, 2);

    // 3. King safety weights
    add_param("king_shield_pawn_1st_row", &search::kKingSafetyWeights.shield_pawn_1st_row, 0, 50, 2);
    add_param("king_shield_pawn_2nd_row", &search::kKingSafetyWeights.shield_pawn_2nd_row, 0, 30, 2);
    add_param("king_open_file_near_king", &search::kKingSafetyWeights.open_file_near_king, -100, 0, 5);
    add_param("king_semi_open_file_near_king", &search::kKingSafetyWeights.semi_open_file_near_king, -50, 0, 5);
    add_param("king_castled_king_bonus", &search::kKingSafetyWeights.castled_king_bonus, 0, 100, 5);
    add_param("king_exposure_penalty", &search::kKingSafetyWeights.king_exposure_penalty, -20, 0, 1);
    add_param("king_endgame_activity", &search::kKingSafetyWeights.king_endgame_activity, 0, 100, 5);

    // 4. Mobility weights
    add_param("mobility_knight_per_square", &search::kMobilityWeights.knight_per_square, 0, 20, 1);
    add_param("mobility_bishop_per_square", &search::kMobilityWeights.bishop_per_square, 0, 20, 1);
    add_param("mobility_rook_per_square", &search::kMobilityWeights.rook_per_square, 0, 10, 1);
    add_param("mobility_queen_per_square", &search::kMobilityWeights.queen_per_square, 0, 5, 1);

    // 5. Rook activity weights
    add_param("rook_open_file", &search::kRookWeights.open_file, 0, 50, 2);
    add_param("rook_semi_open_file", &search::kRookWeights.semi_open_file, 0, 30, 2);
    add_param("rook_seventh_rank", &search::kRookWeights.seventh_rank, 0, 80, 5);
    add_param("rook_connected_rooks", &search::kRookWeights.connected_rooks, 0, 40, 2);
    add_param("rook_behind_passed_pawn", &search::kRookWeights.behind_passed_pawn, 0, 60, 5);

    // 6. PSTs (Middlegame & Endgame)
    add_pst("pst_pawn_mg", search::kPawnPST_MG, -100, 100, 5);
    add_pst("pst_pawn_eg", search::kPawnPST_EG, -100, 100, 5);
    add_pst("pst_knight_mg", search::kKnightPST_MG, -100, 100, 5);
    add_pst("pst_knight_eg", search::kKnightPST_EG, -100, 100, 5);
    add_pst("pst_bishop_mg", search::kBishopPST_MG, -100, 100, 5);
    add_pst("pst_bishop_eg", search::kBishopPST_EG, -100, 100, 5);
    add_pst("pst_rook_mg", search::kRookPST_MG, -100, 100, 5);
    add_pst("pst_rook_eg", search::kRookPST_EG, -100, 100, 5);
    add_pst("pst_queen_mg", search::kQueenPST_MG, -100, 100, 5);
    add_pst("pst_queen_eg", search::kQueenPST_EG, -100, 100, 5);
    add_pst("pst_king_mg", search::kKingPST_MG, -100, 100, 5);
    add_pst("pst_king_eg", search::kKingPST_EG, -100, 100, 5);
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool TuningManager::load_weights(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) continue;

        std::string name = trim(line.substr(0, equal_pos));
        std::string value_str = trim(line.substr(equal_pos + 1));

        try {
            int val = std::stoi(value_str);
            set_parameter(name, val);
        } catch (...) {
            // Ignore format errors
        }
    }
    return true;
}

bool TuningManager::save_weights(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# Chess Engine Evaluation Weights Configuration\n\n";
    for (const auto& param : parameters_) {
        file << param.name << " = " << *param.ptr << "\n";
    }
    return true;
}

bool TuningManager::set_parameter(const std::string& name, int value) {
    for (auto& param : parameters_) {
        if (param.name == name) {
            *param.ptr = std::max(param.min_value, std::min(param.max_value, value));
            return true;
        }
    }
    return false;
}

bool TuningManager::get_parameter(const std::string& name, int& value) const {
    for (const auto& param : parameters_) {
        if (param.name == name) {
            value = *param.ptr;
            return true;
        }
    }
    return false;
}

void TuningManager::reset_to_defaults() {
    for (auto& param : parameters_) {
        *param.ptr = param.default_value;
    }
}

void TuningManager::run_hill_climbing(const std::vector<std::string>& param_names,
                                      int iterations,
                                      const std::function<double(const std::string& param_name, int new_val)>& evaluator) {
    std::cout << "Starting Hill Climbing tuning...\n";

    for (int iter = 0; iter < iterations; ++iter) {
        std::cout << "\n--- Iteration " << iter + 1 << " / " << iterations << " ---\n";
        for (const auto& name : param_names) {
            TuningParameter* param = nullptr;
            for (auto& p : parameters_) {
                if (p.name == name) {
                    param = &p;
                    break;
                }
            }
            if (!param) continue;

            int original_val = *param->ptr;

            // Try +step
            int val_plus = original_val + param->step;
            double score_plus = -9999.0;
            if (val_plus <= param->max_value) {
                *param->ptr = val_plus;
                score_plus = evaluator(name, val_plus);
            }

            // Try -step
            int val_minus = original_val - param->step;
            double score_minus = -9999.0;
            if (val_minus >= param->min_value) {
                *param->ptr = val_minus;
                score_minus = evaluator(name, val_minus);
            }

            // Revert or choose best
            if (score_plus > 0.0 && score_plus >= score_minus) {
                *param->ptr = val_plus;
                std::cout << "Parameter " << name << " optimized: " << original_val << " -> " << val_plus 
                          << " (score delta: " << score_plus << ")\n";
            } else if (score_minus > 0.0 && score_minus > score_plus) {
                *param->ptr = val_minus;
                std::cout << "Parameter " << name << " optimized: " << original_val << " -> " << val_minus 
                          << " (score delta: " << score_minus << ")\n";
            } else {
                *param->ptr = original_val;
                std::cout << "Parameter " << name << " kept at baseline: " << original_val << "\n";
            }
        }
    }
}

void TuningManager::run_grid_search(const std::vector<std::string>& param_names,
                                    const std::function<double()>& evaluator) {
    std::cout << "Starting Grid Search tuning...\n";
    if (param_names.empty()) return;

    // A helper recursive lambdas to iterate grids
    std::function<void(size_t)> search = [&](size_t idx) {
        if (idx == param_names.size()) {
            double result = evaluator();
            std::cout << "Grid Candidate evaluation: ";
            for (const auto& name : param_names) {
                int val = 0;
                get_parameter(name, val);
                std::cout << name << "=" << val << " ";
            }
            std::cout << "-> score: " << result << "\n";
            return;
        }

        const auto& name = param_names[idx];
        TuningParameter* param = nullptr;
        for (auto& p : parameters_) {
            if (p.name == name) {
                param = &p;
                break;
            }
        }
        if (!param) return;

        int orig = *param->ptr;
        for (int v = param->min_value; v <= param->max_value; v += param->step) {
            *param->ptr = v;
            search(idx + 1);
        }
        *param->ptr = orig;
    };

    search(0);
}

} // namespace chess::validation
