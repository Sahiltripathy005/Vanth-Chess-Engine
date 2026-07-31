#include "chess/uci/uci.hpp"
#include <iostream>
#include <sstream>
#include <vector>

namespace chess::uci {

std::vector<std::string> split_string(const std::string& str);

void dispatch_command(UCIEngine* engine, const std::string& line) {
    if (line.empty()) return;

    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;

    std::string args;
    std::getline(ss, args);
    // Strip leading spaces from args if any
    if (!args.empty() && args[0] == ' ') {
        args = args.substr(1);
    }

    if (cmd == "uci") {
        engine->handle_uci();
    } else if (cmd == "isready") {
        engine->handle_isready();
    } else if (cmd == "ucinewgame") {
        engine->handle_ucinewgame();
    } else if (cmd == "position") {
        engine->handle_position(args);
    } else if (cmd == "go") {
        engine->handle_go(args);
    } else if (cmd == "setoption") {
        engine->handle_setoption(args);
    } else if (cmd == "stop") {
        engine->handle_stop();
    } else if (cmd == "quit") {
        // Handled in main uci loop by exiting, but we stop search here just in case
        engine->handle_stop();
    } else if (cmd == "debug") {
        engine->handle_debug(args);
    } else if (cmd == "register") {
        engine->handle_register();
    } else if (cmd == "ponderhit") {
        // Pondering infrastructure hook
        std::cout << "info string ponderhit received\n";
    } else if (cmd == "search_profile") {
        engine->handle_search_profile();
    } else if (cmd == "benchmark_compare") {
        engine->handle_benchmark_compare();
    }
}

} // namespace chess::uci
