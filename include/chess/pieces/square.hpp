#ifndef CHESS_SQUARE_HPP
#define CHESS_SQUARE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

namespace chess::pieces {

class Square {
public:
    static constexpr uint8_t kNone = 64;

    constexpr Square() : index_(kNone) {}
    explicit constexpr Square(uint8_t index) : index_(index) {}
    constexpr Square(uint8_t file, uint8_t rank) 
        : index_((file < 8 && rank < 8) ? (rank * 8 + file) : kNone) {}

    static std::optional<Square> from_string(std::string_view algebraic);

    constexpr uint8_t index() const { return index_; }
    constexpr uint8_t file() const { return index_ < 64 ? (index_ % 8) : 8; }
    constexpr uint8_t rank() const { return index_ < 64 ? (index_ / 8) : 8; }

    constexpr bool is_valid() const { return index_ < 64; }
    constexpr bool is_none() const { return index_ == kNone; }

    constexpr bool operator==(const Square& other) const {
        return index_ == other.index_;
    }

    constexpr bool operator!=(const Square& other) const {
        return index_ != other.index_;
    }

    static constexpr Square None() { return Square(kNone); }

    std::string to_string() const;

private:
    uint8_t index_;
};

} // namespace chess::pieces

#endif // CHESS_SQUARE_HPP
