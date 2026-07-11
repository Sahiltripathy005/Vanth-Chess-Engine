#ifndef CHESS_VERIFIER_HPP
#define CHESS_VERIFIER_HPP

#include "chess/board/board.hpp"
#include <string>
#include <vector>

namespace chess::validation {

struct ValidationResult {
    bool ok = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

struct MaterialAuditResult {
    int white_pawns = 0;
    int white_knights = 0;
    int white_bishops = 0;
    int white_rooks = 0;
    int white_queens = 0;
    int white_total = 0;

    int black_pawns = 0;
    int black_knights = 0;
    int black_bishops = 0;
    int black_rooks = 0;
    int black_queens = 0;
    int black_total = 0;

    int expected_eval = 0;
};

class Verifier {
public:
    // Validate a position's invariants
    static ValidationResult validate_position(const board::Board& board);

    // Audit material counts and calculate static material score
    static MaterialAuditResult audit_material(const board::Board& board);

    // Trace search execution
    static void trace_search(board::Board& board, int depth);

    // Trace static evaluation subcomponents
    static void trace_eval(const board::Board& board);

    // Trace Zobrist hash calculation step-by-step
    static void trace_hash(const board::Board& board);

    // Trace move generation filtering (pseudo-legal vs legal)
    static void trace_movegen(const board::Board& board);

    // Trace Transposition Table probe status and statistics
    static void trace_tt(const board::Board& board);

    // Trace search history / heuristic tables (killer and history moves)
    static void trace_history(board::Board& board, int depth);

    // Verify make/undo move consistency for all legal moves in the position
    static void trace_makeundo(board::Board& board);

    // Execute self-checks for board, hash keys, perft, and make/undo recursion
    static bool run_selfcheck();

    // Run regression tests for historical bugs
    static bool run_bug_tests(const std::string& bug_id = "");

    // Stress test move generation and board invariants via random games
    static bool run_stress_test(int num_games = 100);
};

} // namespace chess::validation

#endif // CHESS_VERIFIER_HPP
