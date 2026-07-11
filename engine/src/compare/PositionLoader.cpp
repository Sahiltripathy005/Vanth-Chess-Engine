#include "PositionLoader.h"
#include "chess/board/board.hpp"
#include "chess/moves/move_generator.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace chess::compare {

moves::Move parseSANMove(board::Board& board, const std::string& san);

std::vector<std::string> PositionLoader::loadFEN(const std::string& fen) {
    return { fen };
}

std::vector<std::string> PositionLoader::loadFromFile(const std::string& path) {
    std::vector<std::string> fens;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open position file: " << path << std::endl;
        return fens;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Trim spaces
        while (!line.empty() && std::isspace(line.back())) line.pop_back();
        while (!line.empty() && std::isspace(line.front())) line.erase(line.begin());
        
        if (!line.empty() && line[0] != '#') {
            fens.push_back(line);
        }
    }
    return fens;
}

std::vector<std::string> PositionLoader::loadFromPGN(const std::string& path) {
    std::vector<std::string> fens;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open PGN file: " << path << std::endl;
        return fens;
    }

    std::string line;
    std::string moveText;
    bool readingMoves = false;

    while (std::getline(file, line)) {
        // Trim
        while (!line.empty() && std::isspace(line.back())) line.pop_back();
        while (!line.empty() && std::isspace(line.front())) line.erase(line.begin());

        if (line.empty()) continue;

        if (line[0] == '[') {
            if (readingMoves) {
                board::Board board = board::Board::start_position();
                fens.push_back(board.to_fen());

                std::stringstream ss(moveText);
                std::string token;
                while (ss >> token) {
                    if (token.empty()) continue;
                    if (token[0] == '{') {
                        std::string rest;
                        std::getline(ss, rest, '}');
                        continue;
                    }
                    if (token[0] == '$') continue;
                    if (token.back() == '.') continue;
                    if (token.find('.') != std::string::npos) continue;
                    if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*") {
                        continue;
                    }

                    moves::Move m = parseSANMove(board, token);
                    if (m.from().is_none() || m.to().is_none()) {
                        break;
                    }
                    board.make_move(m);
                    fens.push_back(board.to_fen());
                }

                moveText.clear();
                readingMoves = false;
            }
            continue;
        }

        readingMoves = true;
        moveText += " " + line;
    }

    if (readingMoves && !moveText.empty()) {
        board::Board board = board::Board::start_position();
        fens.push_back(board.to_fen());

        std::stringstream ss(moveText);
        std::string token;
        while (ss >> token) {
            if (token.empty()) continue;
            if (token[0] == '{') {
                std::string rest;
                std::getline(ss, rest, '}');
                continue;
            }
            if (token[0] == '$') continue;
            if (token.back() == '.') continue;
            if (token.find('.') != std::string::npos) continue;
            if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*") {
                continue;
            }

            moves::Move m = parseSANMove(board, token);
            if (m.from().is_none() || m.to().is_none()) {
                break;
            }
            board.make_move(m);
            fens.push_back(board.to_fen());
        }
    }

    return fens;
}

moves::Move parseSANMove(board::Board& board, const std::string& san) {
    std::string clean = san;
    clean.erase(std::remove_if(clean.begin(), clean.end(), [](char c) {
        return c == '+' || c == '#' || c == '!' || c == '?';
    }), clean.end());

    if (clean.empty()) return moves::Move();

    std::vector<moves::Move> legal = moves::MoveGenerator::generate_legal_moves(board);

    if (clean == "O-O" || clean == "o-o" || clean == "0-0") {
        for (const auto& m : legal) {
            if (m.type() == moves::MoveType::Castling) {
                if (m.to().file() == 6) return m;
            }
        }
        return moves::Move();
    }
    if (clean == "O-O-O" || clean == "o-o-o" || clean == "0-0-0") {
        for (const auto& m : legal) {
            if (m.type() == moves::MoveType::Castling) {
                if (m.to().file() == 2) return m;
            }
        }
        return moves::Move();
    }

    if (clean.length() >= 4 && clean.length() <= 5) {
        char f1 = clean[0], r1 = clean[1], f2 = clean[2], r2 = clean[3];
        if (f1 >= 'a' && f1 <= 'h' && r1 >= '1' && r1 <= '8' &&
            f2 >= 'a' && f2 <= 'h' && r2 >= '1' && r2 <= '8') {
            for (const auto& m : legal) {
                if (m.to_string() == clean) return m;
            }
        }
    }

    pieces::PieceType pType = pieces::PieceType::Pawn;
    size_t idx = 0;
    if (isupper(clean[0])) {
        char pc = clean[0];
        if (pc == 'N') pType = pieces::PieceType::Knight;
        else if (pc == 'B') pType = pieces::PieceType::Bishop;
        else if (pc == 'R') pType = pieces::PieceType::Rook;
        else if (pc == 'Q') pType = pieces::PieceType::Queen;
        else if (pc == 'K') pType = pieces::PieceType::King;
        idx = 1;
    }

    pieces::PieceType promoType = pieces::PieceType::None;
    size_t eqPos = clean.find('=');
    if (eqPos != std::string::npos && eqPos + 1 < clean.length()) {
        char pr = clean[eqPos + 1];
        if (pr == 'N' || pr == 'n') promoType = pieces::PieceType::Knight;
        else if (pr == 'B' || pr == 'b') promoType = pieces::PieceType::Bishop;
        else if (pr == 'R' || pr == 'r') promoType = pieces::PieceType::Rook;
        else if (pr == 'Q' || pr == 'q') promoType = pieces::PieceType::Queen;
        clean = clean.substr(0, eqPos);
    } else if (!clean.empty() && islower(clean.back()) && 
               (clean.back() == 'n' || clean.back() == 'b' || clean.back() == 'r' || clean.back() == 'q') &&
               (clean.length() >= 4)) {
        char pr = clean.back();
        if (pr == 'n') promoType = pieces::PieceType::Knight;
        else if (pr == 'b') promoType = pieces::PieceType::Bishop;
        else if (pr == 'r') promoType = pieces::PieceType::Rook;
        else if (pr == 'q') promoType = pieces::PieceType::Queen;
        clean.pop_back();
    }

    std::string move_part = clean.substr(idx);
    if (move_part.empty()) return moves::Move();

    size_t xPos = move_part.find('x');
    if (xPos != std::string::npos) {
        move_part.erase(xPos, 1);
    }

    if (move_part.length() < 2) return moves::Move();

    std::string dest_sq_str = move_part.substr(move_part.length() - 2);
    std::string disambig = move_part.substr(0, move_part.length() - 2);

    int destFile = dest_sq_str[0] - 'a';
    int destRank = dest_sq_str[1] - '1';

    if (destFile < 0 || destFile > 7 || destRank < 0 || destRank > 7) return moves::Move();

    std::vector<moves::Move> candidates;
    for (const auto& m : legal) {
        if (m.to().file() == destFile && m.to().rank() == destRank) {
            pieces::Piece p = board.piece_at(m.from());
            if (p.type() == pType && p.color() == board.active_color()) {
                if (promoType != pieces::PieceType::None && m.promotion() != promoType) {
                    continue;
                }
                candidates.push_back(m);
            }
        }
    }

    if (candidates.empty()) return moves::Move();
    if (candidates.size() == 1) return candidates[0];

    if (disambig.length() == 1) {
        char dc = disambig[0];
        if (dc >= 'a' && dc <= 'h') {
            int f = dc - 'a';
            for (const auto& m : candidates) {
                if (m.from().file() == f) return m;
            }
        } else if (dc >= '1' && dc <= '8') {
            int r = dc - '1';
            for (const auto& m : candidates) {
                if (m.from().rank() == r) return m;
            }
        }
    } else if (disambig.length() == 2) {
        int f = disambig[0] - 'a';
        int r = disambig[1] - '1';
        for (const auto& m : candidates) {
            if (m.from().file() == f && m.from().rank() == r) return m;
        }
    }

    return candidates[0];
}

} // namespace chess::compare
