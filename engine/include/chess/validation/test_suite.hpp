#ifndef CHESS_TEST_SUITE_HPP
#define CHESS_TEST_SUITE_HPP

#include "chess/board/board.hpp"
#include <string>
#include <vector>

namespace chess::validation {

struct PositionTestResult {
    std::string id;
    std::string fen;
    std::string expected_best_move;
    std::string found_best_move;
    bool is_correct = false;
    int depth_reached = 0;
    int score = 0;
    long long time_ms = 0;
};

struct TestSuiteResults {
    int total_positions = 0;
    int correct_positions = 0;
    double accuracy_percentage = 0.0;
    double avg_search_time_ms = 0.0;
    std::vector<PositionTestResult> detail_results;
};

class TestSuiteRunner {
public:
    // Parses a single EPD line into a FEN string, an expected best move (in SAN or LAN), and an ID
    static bool parse_epd_line(const std::string& epd_line, std::string& out_fen, 
                               std::string& out_best_move, std::string& out_id);

    // Matches a SAN or coordinate move from EPD against the legal moves list and returns the matching Move
    static moves::Move match_epd_move(board::Board& board, const std::string& epd_move_str);

    // Runs the test suite on a file of EPD positions
    static TestSuiteResults run_file(const std::string& filepath, int search_depth, int limit_ms);

    // Runs the test suite on a list of raw EPD strings
    static TestSuiteResults run_list(const std::vector<std::string>& epd_lines, int search_depth, int limit_ms);
};

} // namespace chess::validation

#endif // CHESS_TEST_SUITE_HPP
