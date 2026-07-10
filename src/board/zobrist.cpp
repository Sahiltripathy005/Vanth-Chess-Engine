#include "chess/board/zobrist.hpp"

namespace chess::board {

uint64_t Zobrist::piece_keys[2][6][64];
uint64_t Zobrist::side_to_move_key;
uint64_t Zobrist::castling_keys[16];
uint64_t Zobrist::en_passant_keys[8];
bool Zobrist::initialized = false;

struct PRNG {
    uint64_t state = 1070372ULL;
    uint64_t next() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1DULL;
    }
};

void Zobrist::init() {
    if (initialized) return;

    PRNG prng;
    for (int color = 0; color < 2; ++color) {
        for (int p_type = 0; p_type < 6; ++p_type) {
            for (int sq = 0; sq < 64; ++sq) {
                piece_keys[color][p_type][sq] = prng.next();
            }
        }
    }

    side_to_move_key = prng.next();

    for (int i = 0; i < 16; ++i) {
        castling_keys[i] = prng.next();
    }

    for (int i = 0; i < 8; ++i) {
        en_passant_keys[i] = prng.next();
    }

    initialized = true;
}

uint64_t Zobrist::piece_key(pieces::Color color, pieces::PieceType type, pieces::Square sq) {
    init();
    if (color == pieces::Color::None || type == pieces::PieceType::None || !sq.is_valid()) {
        return 0;
    }
    int c_idx = (color == pieces::Color::White) ? 0 : 1;
    int p_idx = 0;
    switch (type) {
        case pieces::PieceType::Pawn:   p_idx = 0; break;
        case pieces::PieceType::Knight: p_idx = 1; break;
        case pieces::PieceType::Bishop: p_idx = 2; break;
        case pieces::PieceType::Rook:   p_idx = 3; break;
        case pieces::PieceType::Queen:  p_idx = 4; break;
        case pieces::PieceType::King:   p_idx = 5; break;
        default: return 0;
    }
    return piece_keys[c_idx][p_idx][sq.index()];
}

uint64_t Zobrist::side_key() {
    init();
    return side_to_move_key;
}

uint64_t Zobrist::castling_key(uint8_t castling_rights) {
    init();
    return castling_keys[castling_rights & 0x0F];
}

uint64_t Zobrist::en_passant_key(pieces::Square ep_square) {
    init();
    if (!ep_square.is_valid()) return 0;
    return en_passant_keys[ep_square.file()];
}

} // namespace chess::board
