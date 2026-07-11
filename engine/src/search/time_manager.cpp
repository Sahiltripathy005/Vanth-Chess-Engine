#include "chess/search/time_manager.hpp"
#include <algorithm>

namespace chess::search {

std::atomic<bool> g_stop_search(false);

TimeManager::TimeManager() 
    : start_time_(std::chrono::steady_clock::now()) {}

void TimeManager::start(
    int limit_ms,
    int time_left_ms,
    int increment_ms,
    int moves_to_go,
    bool is_infinite
) {
    start_time_ = std::chrono::steady_clock::now();
    is_infinite_ = is_infinite;

    if (is_infinite_) {
        soft_limit_ms_ = 2000000000;
        hard_limit_ms_ = 2000000000;
        return;
    }

    if (limit_ms > 0) {
        // Exact time search
        soft_limit_ms_ = limit_ms;
        hard_limit_ms_ = limit_ms;
    } else if (time_left_ms > 0) {
        // Active time control
        int mtg = (moves_to_go > 0) ? moves_to_go : 35;
        // Allocate a fraction of remaining time
        int base_time = time_left_ms / mtg;
        // Include increment
        base_time += increment_ms;

        // Hard limit should have a safety buffer to prevent time loss
        // Allow up to twice the allocated base time if needed, but not more than 80% of remaining time
        int hard_limit = std::min(base_time * 2, static_cast<int>(time_left_ms * 0.8));
        // Soft limit is where we start warning / stopping before the next depth iteration
        int soft_limit = base_time * 6 / 10; // 60% of base time

        // Ensure bounds
        soft_limit = std::max(soft_limit, 10);
        hard_limit = std::max(hard_limit, 20);

        soft_limit_ms_ = soft_limit;
        hard_limit_ms_ = hard_limit;
    } else {
        // Default search budget (e.g. 5 seconds) if nothing is specified
        soft_limit_ms_ = 3000;
        hard_limit_ms_ = 5000;
    }
}

void TimeManager::check_time(SearchInfo& info) {
    if (g_stop_search) {
        info.aborted = true;
        return;
    }
    if (is_infinite_) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
    
    if (elapsed >= hard_limit_ms_) {
        info.aborted = true;
    }
}

bool TimeManager::should_stop_iterative_deepening() const {
    if (g_stop_search) return true;
    if (is_infinite_) return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
    
    return elapsed >= soft_limit_ms_;
}

} // namespace chess::search
