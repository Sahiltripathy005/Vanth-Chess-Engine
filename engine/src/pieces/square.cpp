#include "chess/pieces/square.hpp"

namespace chess::pieces {

std::optional<Square> Square::from_string(std::string_view algebraic) {
    if (algebraic.size() != 2) {
        return std::nullopt;
    }
    char file_char = algebraic[0];
    char rank_char = algebraic[1];

    if (file_char < 'a' || file_char > 'h' || rank_char < '1' || rank_char > '8') {
        return std::nullopt;
    }

    uint8_t file = static_cast<uint8_t>(file_char - 'a');
    uint8_t rank = static_cast<uint8_t>(rank_char - '1');

    return Square(file, rank);
}

std::string Square::to_string() const {
    if (is_none()) {
        return "-";
    }
    std::string result;
    result += static_cast<char>('a' + file());
    result += static_cast<char>('1' + rank());
    return result;
}

} // namespace chess::pieces
