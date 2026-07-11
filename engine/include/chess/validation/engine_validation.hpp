#ifndef CHESS_ENGINE_VALIDATION_HPP
#define CHESS_ENGINE_VALIDATION_HPP

#include "chess/board/board.hpp"
#include <string>
#include <vector>

namespace chess::validation {

struct EngineValidationReport {
    bool overall_pass = true;
    int total_modules_run = 0;
    int total_modules_passed = 0;
    
    // Module 1: Tactical
    int tactical_total = 0;
    int tactical_solved = 0;
    
    // Module 2: Strategic
    int strategic_total = 0;
    int strategic_passed = 0;
    
    // Module 3: Endgame
    int endgame_total = 0;
    int endgame_passed = 0;
    
    // Module 4: Opening
    int opening_total = 0;
    int opening_passed = 0;
    
    // Module 5: Perft
    bool perft_passed = true;
    
    // Module 6: Stability
    bool stability_passed = true;
    
    // Module 9: Symmetry
    bool symmetry_passed = true;
    
    // Module 14: Stress
    int stress_positions_checked = 0;
    bool stress_passed = true;
    
    // Module 15: Regression
    int regression_bugs_total = 0;
    int regression_bugs_passed = 0;
};

class EngineValidation {
public:
    // Runs the entire CI suite and generates a summary report
    static bool run_full_validation_lab();
    
    // Individual Module runners
    static bool run_tactical_suite(EngineValidationReport& report);
    static bool run_strategic_suite(EngineValidationReport& report);
    static bool run_endgame_suite(EngineValidationReport& report);
    static bool run_opening_suite(EngineValidationReport& report);
    static bool run_perft_suite(EngineValidationReport& report);
    static bool run_search_stability_tests(EngineValidationReport& report);
    static bool run_root_move_analysis(const std::string& fen = "");
    static bool run_search_heuristic_isolation(const std::string& fen = "");
    static bool run_evaluation_symmetry_tests(EngineValidationReport& report);
    static bool run_known_test_suite(const std::string& epd_file);
    static bool run_random_stress_testing(int num_positions, EngineValidationReport& report);
    static bool run_regression_framework(EngineValidationReport& report);
    
    // Helper to generate the final JSON dashboard
    static void generate_dashboard_report(const EngineValidationReport& report, const std::string& filepath);
};

} // namespace chess::validation

#endif // CHESS_ENGINE_VALIDATION_HPP
