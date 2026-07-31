#ifndef CHESS_UCI_HPP
#define CHESS_UCI_HPP

#include "chess/board/board.hpp"
#include "chess/search/search_info.hpp"
#include <string>
#include <vector>
#include <thread>
#include <atomic>

namespace chess::uci {

class UCIEngine {
public:
    UCIEngine();
    ~UCIEngine();

    // Runs the main command-reading loop from stdin
    void run();

    // Command Handlers
    void handle_uci();
    void handle_isready();
    void handle_ucinewgame();
    void handle_position(const std::string& args);
    void handle_go(const std::string& args);
    void handle_setoption(const std::string& args);
    void handle_stop();
    void handle_debug(const std::string& args);
    void handle_register();
    void handle_search_profile();
    void handle_benchmark_compare();

private:
    board::Board board_;
    std::atomic<bool> searching_;
    std::thread search_thread_;
    search::SearchInfo last_search_info_;

    // Helper: parses a move list and plays them on the board
    void play_moves(const std::vector<std::string>& move_strings);

    // Asynchronous search task called in a separate thread
    void async_search(int depth, int limit_ms, int wtime, int btime, int winc, int binc, int movestogo, bool is_infinite);
};

} // namespace chess::uci

#endif // CHESS_UCI_HPP
