#include "chess/board/board.hpp"
#include <vector>
#include <cctype>

namespace chess::board {

static std::vector<std::string_view> split_by_whitespace(std::string_view str) {
    std::vector<std::string_view> tokens;
    size_t start = 0;
    while (start < str.size()) {
        while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
            start++;
        }
        if (start >= str.size()) break;
        size_t end = start;
        while (end < str.size() && !std::isspace(static_cast<unsigned char>(str[end]))) {
            end++;
        }
        tokens.push_back(str.substr(start, end - start));
        start = end;
    }
    return tokens;
}

bool Board::load_fen(std::string_view fen) {
    std::vector<std::string_view> fields = split_by_whitespace(fen);
    if (fields.size() < 4) {
        return false;
    }

    pieces_.fill(pieces::Piece());
    state_ = moves::BoardState();

    // Field 1: Piece placement
    std::string_view placement = fields[0];
    int rank = 7;
    int file = 0;
    for (char c : placement) {
        if (c == '/') {
            rank--;
            file = 0;
            if (rank < 0) {
                return false;
            }
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            int empty_squares = c - '0';
            file += empty_squares;
            if (file > 8) {
                return false;
            }
        } else {
            auto piece_opt = pieces::Piece::from_char(c);
            if (!piece_opt) {
                return false;
            }
            if (file >= 8) {
                return false;
            }
            pieces::Square sq(static_cast<uint8_t>(file), static_cast<uint8_t>(rank));
            set_piece(sq, *piece_opt);
            file++;
        }
    }

    // Field 2: Active Color
    std::string_view active_col_str = fields[1];
    if (active_col_str == "w") {
        state_.active_color = pieces::Color::White;
    } else if (active_col_str == "b") {
        state_.active_color = pieces::Color::Black;
    } else {
        return false;
    }

    // Field 3: Castling rights
    std::string_view castling_str = fields[2];
    state_.castling_rights = moves::CastlingRights::kNone;
    if (castling_str != "-") {
        for (char c : castling_str) {
            switch (c) {
                case 'K': state_.castling_rights |= moves::CastlingRights::kWhiteKingside; break;
                case 'Q': state_.castling_rights |= moves::CastlingRights::kWhiteQueenside; break;
                case 'k': state_.castling_rights |= moves::CastlingRights::kBlackKingside; break;
                case 'q': state_.castling_rights |= moves::CastlingRights::kBlackQueenside; break;
                default: return false;
            }
        }
    }

    // Field 4: En passant target square
    std::string_view ep_str = fields[3];
    if (ep_str == "-") {
        state_.en_passant_square = pieces::Square::None();
    } else {
        auto sq_opt = pieces::Square::from_string(ep_str);
        if (!sq_opt || !sq_opt->is_valid()) {
            return false;
        }
        state_.en_passant_square = *sq_opt;
    }

    // Field 5: Halfmove clock
    if (fields.size() > 4) {
        std::string_view halfmove_str = fields[4];
        int val = 0;
        for (char c : halfmove_str) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
            val = val * 10 + (c - '0');
        }
        state_.halfmove_clock = static_cast<uint16_t>(val);
    }

    // Field 6: Fullmove clock
    if (fields.size() > 5) {
        std::string_view fullmove_str = fields[5];
        int val = 0;
        for (char c : fullmove_str) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
            val = val * 10 + (c - '0');
        }
        state_.fullmove_clock = static_cast<uint16_t>(val);
    }

    state_.zobrist_hash = compute_zobrist_hash();
    return true;
}

std::string Board::to_fen() const {
    std::string fen;

    // Field 1: Piece placement
    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            pieces::Square sq(static_cast<uint8_t>(file), static_cast<uint8_t>(rank));
            pieces::Piece piece = piece_at(sq);
            if (piece.is_none()) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    fen += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen += piece.to_char();
            }
        }
        if (empty_count > 0) {
            fen += std::to_string(empty_count);
        }
        if (rank > 0) {
            fen += '/';
        }
    }

    fen += ' ';

    // Field 2: Active Color
    fen += (state_.active_color == pieces::Color::White) ? 'w' : 'b';

    fen += ' ';

    // Field 3: Castling rights
    std::string castling;
    if (state_.castling_rights & moves::CastlingRights::kWhiteKingside)  castling += 'K';
    if (state_.castling_rights & moves::CastlingRights::kWhiteQueenside) castling += 'Q';
    if (state_.castling_rights & moves::CastlingRights::kBlackKingside)  castling += 'k';
    if (state_.castling_rights & moves::CastlingRights::kBlackQueenside) castling += 'q';
    if (castling.empty()) {
        fen += '-';
    } else {
        fen += castling;
    }

    fen += ' ';

    // Field 4: En passant target square
    if (state_.en_passant_square.is_none()) {
        fen += '-';
    } else {
        fen += state_.en_passant_square.to_string();
    }

    fen += ' ';

    // Field 5: Halfmove clock
    fen += std::to_string(state_.halfmove_clock);

    fen += ' ';

    // Field 6: Fullmove clock
    fen += std::to_string(state_.fullmove_clock);

    return fen;
}

} // namespace chess::board
