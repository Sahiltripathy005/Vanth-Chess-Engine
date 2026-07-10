#include "chess/moves/move.hpp"

namespace chess::moves {

std::string Move::to_string() const {
    if (from().is_none() || to().is_none()) {
        return "0000";
    }
    std::string result = from().to_string() + to().to_string();
    if (type() == MoveType::Promotion && promotion() != pieces::PieceType::None) {
        result += pieces::to_char(promotion());
    }
    return result;
}

} // namespace chess::moves
