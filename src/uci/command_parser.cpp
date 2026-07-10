#include "chess/uci/uci.hpp"
#include <sstream>
#include <vector>
#include <string>

namespace chess::uci {

// Utility to split string into tokens
std::vector<std::string> split_string(const std::string& str) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Parses options from go command
void parse_go_params(const std::string& args, int& depth, int& limit_ms, int& wtime, int& btime, int& winc, int& binc, int& movestogo, bool& is_infinite) {
    auto tokens = split_string(args);
    depth = 0;
    limit_ms = 0;
    wtime = 0;
    btime = 0;
    winc = 0;
    binc = 0;
    movestogo = 0;
    is_infinite = false;

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "depth" && i + 1 < tokens.size()) {
            depth = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
            limit_ms = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "wtime" && i + 1 < tokens.size()) {
            wtime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "btime" && i + 1 < tokens.size()) {
            btime = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "winc" && i + 1 < tokens.size()) {
            winc = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "binc" && i + 1 < tokens.size()) {
            binc = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "movestogo" && i + 1 < tokens.size()) {
            movestogo = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "infinite") {
            is_infinite = true;
        }
    }
}

} // namespace chess::uci
