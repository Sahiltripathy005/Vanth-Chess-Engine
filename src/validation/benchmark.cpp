#include "chess/validation/benchmark.hpp"
#include "chess/search/search.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace chess::validation {

size_t BenchmarkSuite::get_memory_usage_kb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss; // Usually in KB on Linux
    }
#endif
    return 0;
}

static std::string get_current_date() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now_time);
#else
    localtime_r(&now_time, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

BenchmarkMetrics BenchmarkSuite::run_standard(int target_depth) {
    board::Board board = board::Board::start_position();
    
    auto start = std::chrono::high_resolution_clock::now();
    auto info = search::find_best_move(board, target_depth);
    auto end = std::chrono::high_resolution_clock::now();
    
    long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    BenchmarkMetrics m;
    m.date = get_current_date();
    m.depth = target_depth;
    m.time_ms = duration;
    m.nodes = info.nodes;
    m.cutoffs = info.cutoffs;
    m.tt_hits = info.tt_hits;
    m.tt_lookups = info.tt_lookups;
    
    double seconds = duration / 1000.0;
    m.nps = (seconds > 0.0) ? static_cast<uint64_t>(info.nodes / seconds) : 0;
    
    m.branching_factor = (target_depth > 0) ? std::pow(static_cast<double>(info.nodes), 1.0 / target_depth) : 0.0;
    
    if (info.ordering_lookups > 0) {
        m.move_ordering_efficiency = (static_cast<double>(info.fhf) / info.ordering_lookups) * 100.0;
    }
    
    m.memory_usage_kb = get_memory_usage_kb();
    m.best_move = info.best_move.to_string();
    m.evaluation = info.best_score;
    
    return m;
}

std::vector<BenchmarkMetrics> BenchmarkSuite::run_positions(const std::vector<std::string>& fens, int target_depth) {
    std::vector<BenchmarkMetrics> results;
    for (const auto& fen : fens) {
        board::Board board;
        if (!board.load_fen(fen)) continue;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto info = search::find_best_move(board, target_depth);
        auto end = std::chrono::high_resolution_clock::now();
        
        long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        BenchmarkMetrics m;
        m.date = get_current_date();
        m.depth = target_depth;
        m.time_ms = duration;
        m.nodes = info.nodes;
        m.cutoffs = info.cutoffs;
        m.tt_hits = info.tt_hits;
        m.tt_lookups = info.tt_lookups;
        
        double seconds = duration / 1000.0;
        m.nps = (seconds > 0.0) ? static_cast<uint64_t>(info.nodes / seconds) : 0;
        m.branching_factor = (target_depth > 0) ? std::pow(static_cast<double>(info.nodes), 1.0 / target_depth) : 0.0;
        
        if (info.ordering_lookups > 0) {
            m.move_ordering_efficiency = (static_cast<double>(info.fhf) / info.ordering_lookups) * 100.0;
        }
        
        m.memory_usage_kb = get_memory_usage_kb();
        m.best_move = info.best_move.to_string();
        m.evaluation = info.best_score;
        
        results.push_back(m);
    }
    return results;
}

bool BenchmarkSuite::save_to_json(const std::string& filepath, const std::vector<BenchmarkMetrics>& metrics) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    file << "[\n";
    for (size_t i = 0; i < metrics.size(); ++i) {
        const auto& m = metrics[i];
        file << "  {\n"
             << "    \"date\": \"" << m.date << "\",\n"
             << "    \"commit_hash\": \"" << m.commit_hash << "\",\n"
             << "    \"config_name\": \"" << m.config_name << "\",\n"
             << "    \"depth\": " << m.depth << ",\n"
             << "    \"time_ms\": " << m.time_ms << ",\n"
             << "    \"nodes\": " << m.nodes << ",\n"
             << "    \"nps\": " << m.nps << ",\n"
             << "    \"cutoffs\": " << m.cutoffs << ",\n"
             << "    \"tt_hits\": " << m.tt_hits << ",\n"
             << "    \"tt_lookups\": " << m.tt_lookups << ",\n"
             << "    \"branching_factor\": " << std::fixed << std::setprecision(4) << m.branching_factor << ",\n"
             << "    \"move_ordering_efficiency\": " << std::fixed << std::setprecision(2) << m.move_ordering_efficiency << ",\n"
             << "    \"memory_usage_kb\": " << m.memory_usage_kb << ",\n"
             << "    \"best_move\": \"" << m.best_move << "\",\n"
             << "    \"evaluation\": " << m.evaluation << "\n"
             << "  }" << ((i + 1 < metrics.size()) ? "," : "") << "\n";
    }
    file << "]\n";
    return true;
}

bool BenchmarkSuite::save_to_csv(const std::string& filepath, const std::vector<BenchmarkMetrics>& metrics) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    
    file << "date,commit_hash,config_name,depth,time_ms,nodes,nps,cutoffs,tt_hits,tt_lookups,branching_factor,move_ordering_efficiency,memory_usage_kb,best_move,evaluation\n";
    for (const auto& m : metrics) {
        file << m.date << ","
             << m.commit_hash << ","
             << m.config_name << ","
             << m.depth << ","
             << m.time_ms << ","
             << m.nodes << ","
             << m.nps << ","
             << m.cutoffs << ","
             << m.tt_hits << ","
             << m.tt_lookups << ","
             << std::fixed << std::setprecision(4) << m.branching_factor << ","
             << std::fixed << std::setprecision(2) << m.move_ordering_efficiency << ","
             << m.memory_usage_kb << ","
             << m.best_move << ","
             << m.evaluation << "\n";
    }
    return true;
}

std::string BenchmarkSuite::generate_report(const BenchmarkMetrics& m) {
    std::stringstream ss;
    ss << "==================================================\n"
       << "            BENCHMARK REPORT                      \n"
       << "==================================================\n"
       << "Date:          " << m.date << "\n"
       << "Commit:        " << m.commit_hash << "\n"
       << "Configuration: " << m.config_name << "\n"
       << "Depth:         " << m.depth << "\n"
       << "Time:          " << m.time_ms << " ms\n"
       << "Nodes:         " << m.nodes << "\n"
       << "NPS:           " << m.nps << "\n"
       << "Cutoffs:       " << m.cutoffs << "\n"
       << "TT Hits:       " << m.tt_hits << " / " << m.tt_lookups << "\n"
       << "Branching F:   " << std::fixed << std::setprecision(2) << m.branching_factor << "\n"
       << "FHF Efficiency: " << std::fixed << std::setprecision(2) << m.move_ordering_efficiency << "%\n"
       << "Memory:        " << m.memory_usage_kb << " KB\n"
       << "Best Move:     " << m.best_move << "\n"
       << "Evaluation:    " << m.evaluation << "\n"
       << "==================================================\n";
    return ss.str();
}

} // namespace chess::validation
