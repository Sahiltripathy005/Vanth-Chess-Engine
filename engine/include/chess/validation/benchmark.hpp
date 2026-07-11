#ifndef CHESS_BENCHMARK_HPP
#define CHESS_BENCHMARK_HPP

#include "chess/board/board.hpp"
#include <string>
#include <vector>

namespace chess::validation {

struct BenchmarkMetrics {
    std::string date;
    std::string commit_hash = "local_dev";
    std::string config_name = "default";
    int depth = 0;
    long long time_ms = 0;
    uint64_t nodes = 0;
    uint64_t nps = 0;
    uint64_t cutoffs = 0;
    uint64_t tt_hits = 0;
    uint64_t tt_lookups = 0;
    double branching_factor = 0.0;
    double move_ordering_efficiency = 0.0;
    size_t memory_usage_kb = 0;
    std::string best_move;
    int evaluation = 0;
};

class BenchmarkSuite {
public:
    // Helper to get current memory usage of the process in KB
    static size_t get_memory_usage_kb();

    // Runs a benchmark on the starting position up to a given depth
    static BenchmarkMetrics run_standard(int target_depth);

    // Runs a benchmark on a set of custom FENs
    static std::vector<BenchmarkMetrics> run_positions(const std::vector<std::string>& fens, int target_depth);

    // Outputs benchmark results to JSON and CSV files
    static bool save_to_json(const std::string& filepath, const std::vector<BenchmarkMetrics>& metrics);
    static bool save_to_csv(const std::string& filepath, const std::vector<BenchmarkMetrics>& metrics);

    // Generates a comprehensive benchmark report string
    static std::string generate_report(const BenchmarkMetrics& metric);
};

} // namespace chess::validation

#endif // CHESS_BENCHMARK_HPP
