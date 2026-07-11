#include "chess/uci/uci.hpp"
#include "chess/uci/option_manager.hpp"
#include "chess/moves/move_generator.hpp"
#include "chess/search/search.hpp"
#include "chess/search/time_manager.hpp"
#include "chess/search/transposition_table.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

namespace chess::uci {

// Forward declarations of parser/dispatcher helpers
std::vector<std::string> split_string(const std::string& str);
void dispatch_command(UCIEngine* engine, const std::string& line);
void parse_go_params(const std::string& args, int& depth, int& limit_ms, int& wtime, int& btime, int& winc, int& binc, int& movestogo, bool& is_infinite);

UCIEngine::UCIEngine() : board_(board::Board::start_position()), searching_(false) {
}

UCIEngine::~UCIEngine() {
    handle_stop();
}

void UCIEngine::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") {
            handle_stop();
            break;
        }
        dispatch_command(this, line);
    }
}

void UCIEngine::handle_uci() {
    std::cout << "id name Vanth Chess Engine\n";
    std::cout << "id author Google Deepmind team\n";
    OptionManager::get().print_options();
    std::cout << "uciok\n" << std::endl;
}

void UCIEngine::handle_isready() {
    std::cout << "readyok\n" << std::endl;
}

void UCIEngine::handle_ucinewgame() {
    handle_stop();
    board_ = board::Board::start_position();
    search::g_tt.clear();
}

void UCIEngine::handle_position(const std::string& args) {
    std::string fen;
    std::vector<std::string> move_strings;

    size_t moves_pos = args.find("moves");
    std::string pos_part = (moves_pos != std::string::npos) ? args.substr(0, moves_pos) : args;
    std::string moves_part = (moves_pos != std::string::npos) ? args.substr(moves_pos + 5) : "";

    if (pos_part.find("startpos") != std::string::npos) {
        board_ = board::Board::start_position();
    } else {
        size_t fen_idx = pos_part.find("fen");
        if (fen_idx != std::string::npos) {
            fen = pos_part.substr(fen_idx + 4);
            // Trim trailing spaces
            while (!fen.empty() && std::isspace(fen.back())) {
                fen.pop_back();
            }
            // Trim leading spaces
            while (!fen.empty() && std::isspace(fen.front())) {
                fen.erase(fen.begin());
            }
            if (!board_.load_fen(fen)) {
                board_ = board::Board::start_position();
            }
        }
    }

    if (!moves_part.empty()) {
        std::stringstream ss(moves_part);
        std::string mv;
        while (ss >> mv) {
            move_strings.push_back(mv);
        }
        play_moves(move_strings);
    }
}

void UCIEngine::handle_go(const std::string& args) {
    handle_stop();

    int depth = 0;
    int limit_ms = 0;
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movestogo = 0;
    bool is_infinite = false;

    // Check for ponder flag in go command
    if (args.find("ponder") != std::string::npos) {
        is_infinite = true;
    }

    parse_go_params(args, depth, limit_ms, wtime, btime, winc, binc, movestogo, is_infinite);

    search_thread_ = std::thread(&UCIEngine::async_search, this, depth, limit_ms, wtime, btime, winc, binc, movestogo, is_infinite);
}

void UCIEngine::handle_setoption(const std::string& args) {
    // Expected format: setoption name <name> [value <val>]
    size_t name_pos = args.find("name");
    if (name_pos == std::string::npos) return;

    size_t val_pos = args.find("value");
    std::string name_str;
    std::string val_str;

    if (val_pos != std::string::npos) {
        name_str = args.substr(name_pos + 5, val_pos - (name_pos + 5));
        val_str = args.substr(val_pos + 6);
    } else {
        name_str = args.substr(name_pos + 5);
    }

    // Trim spaces
    while (!name_str.empty() && std::isspace(name_str.back())) name_str.pop_back();
    while (!name_str.empty() && std::isspace(name_str.front())) name_str.erase(name_str.begin());
    while (!val_str.empty() && std::isspace(val_str.back())) val_str.pop_back();
    while (!val_str.empty() && std::isspace(val_str.front())) val_str.erase(val_str.begin());

    OptionManager::get().set_option(name_str, val_str);
}

void UCIEngine::handle_stop() {
    search::g_stop_search = true;
    if (search_thread_.joinable()) {
        search_thread_.join();
    }
}

void UCIEngine::handle_debug(const std::string& args) {
    (void)args;
}

void UCIEngine::handle_register() {
    std::cout << "registration ok\n" << std::endl;
}

void UCIEngine::play_moves(const std::vector<std::string>& move_strings) {
    for (const auto& mv_str : move_strings) {
        std::vector<moves::Move> legal = moves::MoveGenerator::generate_legal_moves(board_);
        bool found = false;
        for (const auto& m : legal) {
            if (m.to_string() == mv_str) {
                board_.make_move(m);
                found = true;
                break;
            }
        }
        if (!found) {
            break; // Stop if move is illegal
        }
    }
}

void UCIEngine::async_search(int depth, int limit_ms, int wtime, int btime, int winc, int binc, int movestogo, bool is_infinite) {
    searching_ = true;
    search::g_stop_search = false;

    if (depth <= 0) {
        depth = 64; // Default depth limit
    }

    int time_left = 0;
    int inc = 0;
    if (board_.active_color() == pieces::Color::White) {
        time_left = wtime;
        inc = winc;
    } else {
        time_left = btime;
        inc = binc;
    }

    auto info = search::find_best_move(board_, depth, limit_ms, time_left, inc, movestogo, is_infinite);

    if (!info.best_move.from().is_none()) {
        std::cout << "bestmove " << info.best_move.to_string();
        if (info.pv.size() >= 2) {
            std::cout << " ponder " << info.pv[1].to_string();
        }
        std::cout << std::endl;
    } else {
        auto legal = moves::MoveGenerator::generate_legal_moves(board_);
        if (!legal.empty()) {
            std::cout << "bestmove " << legal[0].to_string() << std::endl;
        } else {
            std::cout << "bestmove 0000" << std::endl;
        }
    }

    searching_ = false;
}

} // namespace chess::uci
