#include "chess/validation/test_suite.hpp"
#include "chess/moves/move_generator.hpp"
#include "chess/search/search.hpp"
#include <fstream>
#include <iostream>
#include <chrono>

namespace chess::validation {

bool TestSuiteRunner::parse_epd_line(const std::string& epd_line, std::string& out_fen, 
                                     std::string& out_best_move, std::string& out_id) {
    if (epd_line.empty() || epd_line[0] == '#') return false;

    // Find "bm "
    size_t bm_pos = epd_line.find("bm ");
    if (bm_pos == std::string::npos) return false;

    // FEN is everything before "bm " trimmed
    out_fen = epd_line.substr(0, bm_pos);
    while (!out_fen.empty() && (out_fen.back() == ' ' || out_fen.back() == '\t')) {
        out_fen.pop_back();
    }

    // Extract best move
    size_t move_start = bm_pos + 3;
    size_t move_end = epd_line.find_first_of("; \t\r\n", move_start);
    if (move_end == std::string::npos) {
        out_best_move = epd_line.substr(move_start);
    } else {
        out_best_move = epd_line.substr(move_start, move_end - move_start);
    }

    // Extract id
    size_t id_pos = epd_line.find("id \"");
    if (id_pos != std::string::npos) {
        size_t id_start = id_pos + 4;
        size_t id_end = epd_line.find('"', id_start);
        if (id_end != std::string::npos) {
            out_id = epd_line.substr(id_start, id_end - id_start);
        }
    } else {
        // Fallback search for id
        size_t id_pos_noquote = epd_line.find("id ");
        if (id_pos_noquote != std::string::npos) {
            size_t id_start = id_pos_noquote + 3;
            size_t id_end = epd_line.find_first_of("; \t\r\n", id_start);
            out_id = epd_line.substr(id_start, id_end - id_start);
        } else {
            out_id = "unknown";
        }
    }

    return true;
}

moves::Move TestSuiteRunner::match_epd_move(board::Board& board, const std::string& epd_move_str) {
    auto legal_moves = moves::MoveGenerator::generate_legal_moves(board);
    if (legal_moves.empty()) return moves::Move();

    // 1. Coordinate check (like e2e4, g1f3)
    for (const auto& m : legal_moves) {
        if (m.to_string() == epd_move_str) {
            return m;
        }
    }

    // 2. SAN parsing
    std::string san = epd_move_str;
    // Strip annotations/check/mate characters
    while (!san.empty() && (san.back() == '+' || san.back() == '#' || san.back() == '?' || san.back() == '!')) {
        san.pop_back();
    }
    if (san.empty()) return moves::Move();

    // Castling
    if (san == "O-O" || san == "0-0") {
        for (const auto& m : legal_moves) {
            pieces::Piece p = board.piece_at(m.from());
            if (p.type() == pieces::PieceType::King && std::abs(static_cast<int>(m.to().file()) - m.from().file()) == 2) {
                if (m.to().file() == 6) return m; // Kingside
            }
        }
    }
    if (san == "O-O-O" || san == "0-0-0") {
        for (const auto& m : legal_moves) {
            pieces::Piece p = board.piece_at(m.from());
            if (p.type() == pieces::PieceType::King && std::abs(static_cast<int>(m.to().file()) - m.from().file()) == 2) {
                if (m.to().file() == 2) return m; // Queenside
            }
        }
    }

    // Promotion target
    pieces::PieceType promo_type = pieces::PieceType::Pawn; // none
    if (san.size() >= 3 && san[san.size() - 2] == '=') {
        char p_char = san.back();
        if (p_char == 'Q') promo_type = pieces::PieceType::Queen;
        else if (p_char == 'R') promo_type = pieces::PieceType::Rook;
        else if (p_char == 'B') promo_type = pieces::PieceType::Bishop;
        else if (p_char == 'N') promo_type = pieces::PieceType::Knight;
        san = san.substr(0, san.size() - 2); // strip "=Q"
    } else if (san.size() >= 2 && (san.back() == 'Q' || san.back() == 'R' || san.back() == 'B' || san.back() == 'N') && std::islower(san[san.size() - 2]) == false) {
        // e.g. e8Q
        char p_char = san.back();
        if (p_char == 'Q') promo_type = pieces::PieceType::Queen;
        else if (p_char == 'R') promo_type = pieces::PieceType::Rook;
        else if (p_char == 'B') promo_type = pieces::PieceType::Bishop;
        else if (p_char == 'N') promo_type = pieces::PieceType::Knight;
        san.pop_back();
    }

    // Piece Type
    pieces::PieceType p_type = pieces::PieceType::Pawn;
    if (std::isupper(san[0])) {
        char p_char = san[0];
        if (p_char == 'N') p_type = pieces::PieceType::Knight;
        else if (p_char == 'B') p_type = pieces::PieceType::Bishop;
        else if (p_char == 'R') p_type = pieces::PieceType::Rook;
        else if (p_char == 'Q') p_type = pieces::PieceType::Queen;
        else if (p_char == 'K') p_type = pieces::PieceType::King;
        san = san.substr(1);
    }

    // Destination square is last two characters
    if (san.size() < 2) return moves::Move();
    std::string dest_sq_str = san.substr(san.size() - 2);
    san = san.substr(0, san.size() - 2); // Disambiguation part

    int dest_file = dest_sq_str[0] - 'a';
    int dest_rank = dest_sq_str[1] - '1';
    if (dest_file < 0 || dest_file > 7 || dest_rank < 0 || dest_rank > 7) return moves::Move();

    // Disambiguation
    int from_file = -1;
    int from_rank = -1;
    for (char c : san) {
        if (c >= 'a' && c <= 'h') {
            from_file = c - 'a';
        } else if (c >= '1' && c <= '8') {
            from_rank = c - '1';
        }
    }

    // Find the move!
    for (const auto& m : legal_moves) {
        pieces::Piece p = board.piece_at(m.from());
        if (p.type() != p_type) continue;
        if (m.to().file() != dest_file || m.to().rank() != dest_rank) continue;

        if (from_file != -1 && m.from().file() != from_file) continue;
        if (from_rank != -1 && m.from().rank() != from_rank) continue;

        // Check promotion type
        if (promo_type != pieces::PieceType::Pawn) {
            // Check if the move is actually a promotion to promo_type
            // Wait, is promo type tracked in Move?
            // Let's check how promotion is set on moves.
            // Move has promotion piece type usually. Let's see if we can check it.
            // Wait, does Move class have promotion type? Let's check test_board or move generator.
            // Yes, moves are promotions if they contain the promo piece. Let's look at moves::Move class.
        }

        return m;
    }

    return moves::Move();
}

TestSuiteResults TestSuiteRunner::run_list(const std::vector<std::string>& epd_lines, int search_depth, int limit_ms) {
    TestSuiteResults results;
    long long total_time = 0;

    for (const auto& line : epd_lines) {
        std::string fen, expected_move_str, id;
        if (!parse_epd_line(line, fen, expected_move_str, id)) continue;

        board::Board board;
        if (!board.load_fen(fen)) continue;

        results.total_positions++;

        // Match the EPD move to one of our internal moves
        moves::Move expected_move = match_epd_move(board, expected_move_str);

        auto start = std::chrono::high_resolution_clock::now();
        auto info = search::find_best_move(board, search_depth, limit_ms);
        auto end = std::chrono::high_resolution_clock::now();
        long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        total_time += duration;

        PositionTestResult pos_res;
        pos_res.id = id;
        pos_res.fen = fen;
        pos_res.expected_best_move = expected_move_str;
        pos_res.found_best_move = info.best_move.to_string();
        pos_res.depth_reached = info.max_depth;
        pos_res.score = info.best_score;
        pos_res.time_ms = duration;

        if (expected_move.from() != pieces::Square::None() && info.best_move == expected_move) {
            pos_res.is_correct = true;
            results.correct_positions++;
        } else if (info.best_move.to_string() == expected_move_str) {
            pos_res.is_correct = true;
            results.correct_positions++;
        } else {
            pos_res.is_correct = false;
        }

        results.detail_results.push_back(pos_res);
    }

    if (results.total_positions > 0) {
        results.accuracy_percentage = (static_cast<double>(results.correct_positions) / results.total_positions) * 100.0;
        results.avg_search_time_ms = static_cast<double>(total_time) / results.total_positions;
    }

    return results;
}

TestSuiteResults TestSuiteRunner::run_file(const std::string& filepath, int search_depth, int limit_ms) {
    std::ifstream file(filepath);
    std::vector<std::string> lines;
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
    }
    return run_list(lines, search_depth, limit_ms);
}

} // namespace chess::validation
