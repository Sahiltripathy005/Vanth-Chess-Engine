#ifndef CHESS_TIME_MANAGER_HPP
#define CHESS_TIME_MANAGER_HPP

#include "chess/search/search_info.hpp"
#include <chrono>
#include <atomic>

namespace chess::search {

class TimeManager {
public:
    TimeManager();

    // Initializes limits based on inputs.
    // - limit_ms: exact search time limit (0 if none)
    // - time_left_ms: time remaining on clock (0 if none)
    // - increment_ms: clock increment per move (0 if none)
    // - moves_to_go: number of moves until next time control (0 if none)
    // - is_infinite: whether to ignore time limits
    void start(
        int limit_ms = 0,
        int time_left_ms = 0,
        int increment_ms = 0,
        int moves_to_go = 0,
        bool is_infinite = false
    );

    // Checks the elapsed time and sets info.aborted to true if the hard limit is reached.
    void check_time(SearchInfo& info);

    // Returns whether the soft limit has been reached.
    // Useful for deciding whether to start another iteration in Iterative Deepening.
    bool should_stop_iterative_deepening() const;

    bool is_infinite() const { return is_infinite_; }
    int hard_limit_ms() const { return hard_limit_ms_; }
    int soft_limit_ms() const { return soft_limit_ms_; }

private:
    std::chrono::steady_clock::time_point start_time_;
    int soft_limit_ms_ = 0;
    int hard_limit_ms_ = 0;
    bool is_infinite_ = false;
};

extern std::atomic<bool> g_stop_search;

} // namespace chess::search

#endif // CHESS_TIME_MANAGER_HPP
