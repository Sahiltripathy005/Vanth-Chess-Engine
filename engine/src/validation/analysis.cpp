#include "chess/validation/analysis.hpp"
#include "chess/search/evaluation.hpp"
#include "chess/moves/move_generator.hpp"
#include "chess/search/search.hpp"
#include "chess/validation/endgame_framework.hpp"
#include "chess/uci/option_manager.hpp"
#include <sstream>
#include <iomanip>
#include <map>
#include <cmath>
#include <algorithm>

namespace chess::validation {

EvalBreakdown AnalysisTools::get_evaluation_breakdown(const board::Board& board) {
    int phase = search::get_game_phase(board);
    
    EvalBreakdown b;
    b.material = search::evaluate_material(board);
    b.pst = search::evaluate_piece_square_tables(board, phase);
    b.pawn_structure = search::evaluate_pawn_structure(board);
    b.mobility = search::evaluate_mobility(board);
    b.piece_activity = search::evaluate_piece_activity(board, phase) - b.mobility;
    b.king_safety = search::evaluate_king_safety(board, phase);
    b.space = search::evaluate_space(board);
    b.threats = search::evaluate_threats(board);
    b.endgame = search::evaluate_endgame(board, phase);
    b.tempo = search::evaluate_tempo(board);
    b.total = search::evaluate(board);
    
    return b;
}

std::string AnalysisTools::format_evaluation_breakdown(const EvalBreakdown& b) {
    std::stringstream ss;
    ss << "--------------------------------------------------\n"
       << "         STATIC EVALUATION BREAKDOWN              \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "Material:" << std::right << std::setw(10) << b.material << " cp\n"
       << std::left << std::setw(25) << "Piece-Square Tables:" << std::right << std::setw(10) << b.pst << " cp\n"
       << std::left << std::setw(25) << "Pawn Structure:" << std::right << std::setw(10) << b.pawn_structure << " cp\n"
       << std::left << std::setw(25) << "Mobility:" << std::right << std::setw(10) << b.mobility << " cp\n"
       << std::left << std::setw(25) << "Piece Activity:" << std::right << std::setw(10) << b.piece_activity << " cp\n"
       << std::left << std::setw(25) << "King Safety:" << std::right << std::setw(10) << b.king_safety << " cp\n"
       << std::left << std::setw(25) << "Space Control:" << std::right << std::setw(10) << b.space << " cp\n"
       << std::left << std::setw(25) << "Tactical Threats:" << std::right << std::setw(10) << b.threats << " cp\n"
       << std::left << std::setw(25) << "Endgame Knowledge:" << std::right << std::setw(10) << b.endgame << " cp\n"
       << std::left << std::setw(25) << "Tempo Bonus:" << std::right << std::setw(10) << b.tempo << " cp\n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "TOTAL SCORE:" << std::right << std::setw(10) << b.total << " cp\n"
       << "--------------------------------------------------\n";
    return ss.str();
}

std::string AnalysisTools::get_hash_table_statistics(const search::TranspositionTable& tt) {
    std::stringstream ss;
    size_t entry_bytes = 24; // size of TTEntry
    size_t size_bytes = tt.size() * entry_bytes;
    double size_mb = static_cast<double>(size_bytes) / (1024.0 * 1024.0);
    
    double hit_rate = 0.0;
    if (tt.lookups() > 0) {
        hit_rate = (static_cast<double>(tt.hits()) / tt.lookups()) * 100.0;
    }
    
    ss << "--------------------------------------------------\n"
       << "         TRANSPOSITION TABLE STATISTICS           \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "Total Entries:" << std::right << std::setw(10) << tt.size() << "\n"
       << std::left << std::setw(25) << "Allocated Memory:" << std::right << std::fixed << std::setprecision(2) << std::setw(8) << size_mb << " MB\n"
       << std::left << std::setw(25) << "Total Lookups:" << std::right << std::setw(10) << tt.lookups() << "\n"
       << std::left << std::setw(25) << "Total Hits:" << std::right << std::setw(10) << tt.hits() << "\n"
       << std::left << std::setw(25) << "TT Hit Rate:" << std::right << std::fixed << std::setprecision(2) << std::setw(8) << hit_rate << "%\n"
       << "--------------------------------------------------\n";
    return ss.str();
}

std::string AnalysisTools::get_move_ordering_efficiency(const search::SearchInfo& info) {
    std::stringstream ss;
    double ordering_eff = 0.0;
    if (info.ordering_lookups > 0) {
        ordering_eff = (static_cast<double>(info.fhf) / info.ordering_lookups) * 100.0;
    }
    
    ss << "--------------------------------------------------\n"
       << "         MOVE ORDERING EFFICIENCY                 \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(25) << "Ordering Lookups:" << std::right << std::setw(10) << info.ordering_lookups << "\n"
       << std::left << std::setw(25) << "First-Move Beta Cutoffs (FHF):" << std::right << std::setw(10) << info.fhf << "\n"
       << std::left << std::setw(25) << "Ordering Efficiency:" << std::right << std::fixed << std::setprecision(2) << std::setw(8) << ordering_eff << "%\n"
       << "--------------------------------------------------\n";
    return ss.str();
}

std::string AnalysisTools::format_search_timeline(const std::vector<SearchTimelinePoint>& timeline) {
    std::stringstream ss;
    ss << "--------------------------------------------------\n"
       << "         SEARCH TIMELINE STATISTICS               \n"
       << "--------------------------------------------------\n"
       << std::left << std::setw(8) << "Depth" 
       << std::left << std::setw(12) << "Time (ms)" 
       << std::left << std::setw(12) << "Nodes" 
       << std::left << std::setw(10) << "Score" 
       << "Best Move\n"
       << "--------------------------------------------------\n";
    for (const auto& pt : timeline) {
        ss << std::left << std::setw(8) << pt.depth
           << std::left << std::setw(12) << pt.duration_ms
           << std::left << std::setw(12) << pt.nodes
           << std::left << std::setw(10) << pt.score
           << pt.best_move << "\n";
    }
    ss << "--------------------------------------------------\n";
    return ss.str();
}

static std::string piece_type_to_string(pieces::PieceType t) {
    switch (t) {
        case pieces::PieceType::Pawn:   return "Pawn";
        case pieces::PieceType::Knight: return "Knight";
        case pieces::PieceType::Bishop: return "Bishop";
        case pieces::PieceType::Rook:   return "Rook";
        case pieces::PieceType::Queen:  return "Queen";
        case pieces::PieceType::King:   return "King";
        default: return "None";
    }
}

MoveExplanation AnalysisTools::explain_move(const board::Board& board, const moves::Move& move) {
    MoveExplanation exp;
    exp.move = move.to_string();

    pieces::Piece moving_piece = board.piece_at(move.from());
    if (moving_piece.is_none()) {
        exp.reasons.push_back("No piece at starting square.");
        return exp;
    }

    std::string piece_str = piece_type_to_string(moving_piece.type());

    // Basic descriptions
    if (move.type() == moves::MoveType::Castling) {
        exp.reasons.push_back("Castles to protect the king and activate/connect the rook.");
    } else if (move.type() == moves::MoveType::Promotion) {
        exp.reasons.push_back("Promotes a pawn to a " + piece_type_to_string(move.promotion()) + ".");
    }

    // Capture detection
    pieces::Piece target_piece = board.piece_at(move.to());
    if (target_piece.is_present()) {
        exp.reasons.push_back("Captures the opponent's " + piece_type_to_string(target_piece.type()) + " on " + move.to().to_string() + ".");
    } else if (move.type() == moves::MoveType::EnPassant) {
        exp.reasons.push_back("Captures the opponent's pawn en passant.");
    }

    // Check detection
    board::Board temp_board = board;
    temp_board.make_move(move);
    if (moves::MoveGenerator::is_in_check(temp_board, temp_board.active_color())) {
        exp.reasons.push_back("Delivers check to the enemy king.");
    }

    // Check escaping
    if (moves::MoveGenerator::is_in_check(board, board.active_color())) {
        exp.reasons.push_back("Escapes a check on the king.");
    }

    // Central square occupation/control
    pieces::Square to_sq = move.to();
    int tf = to_sq.file();
    int tr = to_sq.rank();
    if ((tf == 3 || tf == 4) && (tr == 3 || tr == 4)) {
        exp.reasons.push_back("Occupies the key central square " + to_sq.to_string() + " to establish central presence.");
    }

    // Minor piece development
    if (moving_piece.type() == pieces::PieceType::Knight || moving_piece.type() == pieces::PieceType::Bishop) {
        int from_rank = move.from().rank();
        if ((moving_piece.color() == pieces::Color::White && from_rank == 0) ||
            (moving_piece.color() == pieces::Color::Black && from_rank == 7)) {
            exp.reasons.push_back("Develops a minor piece (" + piece_str + ") from its starting square.");
        }
    }

    // Compare evaluation subscores
    auto before_breakdown = get_evaluation_breakdown(board);
    auto after_breakdown = get_evaluation_breakdown(temp_board);

    // Multiplier relative to the mover
    int side_mult = (board.active_color() == pieces::Color::White) ? 1 : -1;

    int material_diff = side_mult * (after_breakdown.material - before_breakdown.material);
    int pst_diff = side_mult * (after_breakdown.pst - before_breakdown.pst);
    int mobility_diff = side_mult * (after_breakdown.mobility - before_breakdown.mobility);
    int activity_diff = side_mult * (after_breakdown.piece_activity - before_breakdown.piece_activity);
    int king_safety_diff = side_mult * (after_breakdown.king_safety - before_breakdown.king_safety);
    int space_diff = side_mult * (after_breakdown.space - before_breakdown.space);
    int pawn_structure_diff = side_mult * (after_breakdown.pawn_structure - before_breakdown.pawn_structure);
    int threats_diff = side_mult * (after_breakdown.threats - before_breakdown.threats);

    if (material_diff > 0) {
        exp.reasons.push_back("Wins material (material balance improves by " + std::to_string(material_diff) + " cp).");
    }
    if (pst_diff > 15) {
        exp.reasons.push_back("Improves piece activity and position according to piece-square tables.");
    }
    if (mobility_diff > 15) {
        exp.reasons.push_back("Increases overall piece mobility and square control (+ " + std::to_string(mobility_diff) + " cp).");
    }
    if (activity_diff > 15) {
        exp.reasons.push_back("Enhances coordination and piece activity.");
    }
    if (king_safety_diff > 15) {
        exp.reasons.push_back("Improves king safety and shelter.");
    } else if (king_safety_diff < -15) {
        exp.reasons.push_back("Accepts potential vulnerabilities around the king for active play.");
    }
    if (space_diff > 10) {
        exp.reasons.push_back("Claims more board space and territorial control.");
    }
    if (pawn_structure_diff > 15) {
        exp.reasons.push_back("Improves pawn structure (resolves weaknesses or advances candidate pawns).");
    }
    if (threats_diff > 15) {
        exp.reasons.push_back("Creates direct threats against opponent's pieces.");
    }

    if (exp.reasons.empty()) {
        exp.reasons.push_back("Maintains structural balance and solidifies the position.");
    }

    return exp;
}

BlunderAnalysis AnalysisTools::analyze_blunder(const board::Board& board, const moves::Move& move_played, int depth) {
    BlunderAnalysis b;
    b.move_played = move_played.to_string();

    board::Board temp_board_before = board;
    auto before_info = search::find_best_move(temp_board_before, depth);
    int before_score = before_info.best_score;

    board::Board temp_board = board;
    temp_board.make_move(move_played);
    
    auto after_info = search::find_best_move(temp_board, std::max(1, depth - 1));
    int after_score = after_info.best_score;

    int drop = (board.active_color() == pieces::Color::White) ? (before_score - after_score) : (after_score - before_score);
    b.score_drop = drop;

    if (drop >= 150) {
        b.is_blunder = true;
    }

    if (b.is_blunder) {
        auto before_breakdown = get_evaluation_breakdown(board);
        auto after_breakdown = get_evaluation_breakdown(temp_board);
        int side_mult = (board.active_color() == pieces::Color::White) ? 1 : -1;

        int mat_diff = side_mult * (after_breakdown.material - before_breakdown.material);
        int king_diff = side_mult * (after_breakdown.king_safety - before_breakdown.king_safety);
        int threats_diff = side_mult * (after_breakdown.threats - before_breakdown.threats);
        int positional_diff = side_mult * (
            (after_breakdown.pst + after_breakdown.space + after_breakdown.pawn_structure + after_breakdown.piece_activity) -
            (before_breakdown.pst + before_breakdown.space + before_breakdown.pawn_structure + before_breakdown.piece_activity)
        );

        if (mat_diff <= -100) {
            b.material_loss = true;
        }
        if (king_diff <= -30) {
            b.king_safety_mistake = true;
        }
        if (threats_diff <= -30) {
            b.tactical_mistake = true;
        }
        if (positional_diff <= -50) {
            b.positional_loss = true;
        }

        if (before_score > 250 && after_score <= 50 && !b.material_loss) {
            b.missed_opportunity = true;
        }

        std::stringstream ss;
        ss << "Blunder! Evaluation drops by " << drop << " cp. ";
        if (b.material_loss) {
            ss << "This move directly hangs material. ";
        }
        if (b.king_safety_mistake) {
            ss << "This move seriously weakens the king's shelter. ";
        }
        if (b.tactical_mistake) {
            ss << "This move exposes a tactical flaw or under-defended piece. ";
        }
        if (b.positional_loss) {
            ss << "This move concedes important board space or coordination. ";
        }
        if (b.missed_opportunity) {
            ss << "This move misses a winning opportunity or tactical blow. ";
        }
        if (ss.str() == "Blunder! Evaluation drops by " + std::to_string(drop) + " cp. ") {
            ss << "The move is tactically or strategically inferior to " << before_info.best_move.to_string() << ".";
        }
        b.explanation = ss.str();
    } else {
        b.explanation = "No major blunder detected.";
    }

    return b;
}

static bool check_ray_alignment_helper(pieces::Square start, pieces::Square target, int step_f, int step_r) {
    int sf = start.file();
    int sr = start.rank();
    int tf = target.file();
    int tr = target.rank();

    int df = tf - sf;
    int dr = tr - sr;

    if (step_f == 0 && df != 0) return false;
    if (step_r == 0 && dr != 0) return false;
    if (step_f != 0 && step_r != 0 && std::abs(df) != std::abs(dr)) return false;
    if (step_f != 0 && df * step_f < 0) return false;
    if (step_r != 0 && dr * step_r < 0) return false;

    return true;
}

TacticalThemes AnalysisTools::detect_tactical_themes(const board::Board& board) {
    TacticalThemes themes;

    pieces::Square white_king_sq = pieces::Square::None();
    pieces::Square black_king_sq = pieces::Square::None();
    pieces::Square white_queen_sq = pieces::Square::None();
    pieces::Square black_queen_sq = pieces::Square::None();

    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Piece p = board.piece_at(pieces::Square(i));
        if (p.is_present()) {
            if (p.type() == pieces::PieceType::King) {
                if (p.color() == pieces::Color::White) white_king_sq = pieces::Square(i);
                else black_king_sq = pieces::Square(i);
            } else if (p.type() == pieces::PieceType::Queen) {
                if (p.color() == pieces::Color::White) white_queen_sq = pieces::Square(i);
                else black_queen_sq = pieces::Square(i);
            }
        }
    }

    // Generate pseudo-legal moves for active side to detect forks, double attacks
    auto active_moves = moves::MoveGenerator::generate_pseudo_legal_moves(board);
    
    // Fork and Double Attack detection
    std::map<uint8_t, std::vector<pieces::Square>> attack_map;
    for (const auto& m : active_moves) {
        pieces::Piece target = board.piece_at(m.to());
        if (target.is_present() && target.color() != board.active_color()) {
            attack_map[m.from().index()].push_back(m.to());
        }
    }

    for (const auto& pair : attack_map) {
        if (pair.second.size() >= 2) {
            bool has_valuable_target = false;
            for (const auto& target_sq : pair.second) {
                pieces::PieceType t = board.piece_at(target_sq).type();
                if (t == pieces::PieceType::Queen || t == pieces::PieceType::Rook || 
                    t == pieces::PieceType::Bishop || t == pieces::PieceType::Knight ||
                    t == pieces::PieceType::King) {
                    has_valuable_target = true;
                }
            }
            if (has_valuable_target) {
                themes.fork = true;
                themes.double_attack = true;
                pieces::Piece fork_piece = board.piece_at(pieces::Square(pair.first));
                themes.descriptions.push_back("Potential fork/double attack detected by " + 
                    piece_type_to_string(fork_piece.type()) + " on " + pieces::Square(pair.first).to_string());
            }
        }
    }

    // Pin, Skewer, and Discovered Attack detection
    for (uint8_t i = 0; i < 64; ++i) {
        pieces::Square attacker_sq(i);
        pieces::Piece att = board.piece_at(attacker_sq);
        if (att.is_none()) continue;

        pieces::Color att_color = att.color();
        pieces::Color victim_color = pieces::opposite_color(att_color);

        pieces::Square target_king = (victim_color == pieces::Color::White) ? white_king_sq : black_king_sq;
        pieces::Square target_queen = (victim_color == pieces::Color::White) ? white_queen_sq : black_queen_sq;

        if (att.type() == pieces::PieceType::Bishop || att.type() == pieces::PieceType::Rook || att.type() == pieces::PieceType::Queen) {
            int steps[8][2] = { {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
            for (int s = 0; s < 8; ++s) {
                int df = steps[s][0];
                int dr = steps[s][1];

                if (att.type() == pieces::PieceType::Bishop && (df == 0 || dr == 0)) continue;
                if (att.type() == pieces::PieceType::Rook && (df != 0 && dr != 0)) continue;

                // Check alignment to King
                if (!target_king.is_none() && check_ray_alignment_helper(attacker_sq, target_king, df, dr)) {
                    int f = attacker_sq.file() + df;
                    int r = attacker_sq.rank() + dr;
                    int pieces_between = 0;
                    pieces::Square between_sq = pieces::Square::None();

                    while (f != target_king.file() || r != target_king.rank()) {
                        pieces::Square sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
                        if (board.piece_at(sq).is_present()) {
                            pieces_between++;
                            between_sq = sq;
                        }
                        f += df;
                        r += dr;
                    }

                    if (pieces_between == 1) {
                        pieces::Piece p_between = board.piece_at(between_sq);
                        if (p_between.color() == victim_color) {
                            themes.pin = true;
                            themes.descriptions.push_back("Pin: Enemy " + piece_type_to_string(p_between.type()) + 
                                " on " + between_sq.to_string() + " is pinned to the King by " + 
                                piece_type_to_string(att.type()) + " on " + attacker_sq.to_string());
                        } else if (p_between.color() == att_color) {
                            themes.discovered_attack = true;
                            themes.descriptions.push_back("Discovered attack setup: moving " + 
                                piece_type_to_string(p_between.type()) + " on " + between_sq.to_string() + 
                                " will expose the enemy King to attack by " + piece_type_to_string(att.type()));
                        }
                    }
                }

                // Check alignment to Queen
                if (!target_queen.is_none() && check_ray_alignment_helper(attacker_sq, target_queen, df, dr)) {
                    int f = attacker_sq.file() + df;
                    int r = attacker_sq.rank() + dr;
                    int pieces_between = 0;
                    pieces::Square between_sq = pieces::Square::None();

                    while (f != target_queen.file() || r != target_queen.rank()) {
                        pieces::Square sq(static_cast<uint8_t>(f), static_cast<uint8_t>(r));
                        if (board.piece_at(sq).is_present()) {
                            pieces_between++;
                            between_sq = sq;
                        }
                        f += df;
                        r += dr;
                    }

                    if (pieces_between == 1) {
                        pieces::Piece p_between = board.piece_at(between_sq);
                        if (p_between.color() == victim_color) {
                            themes.pin = true;
                            themes.descriptions.push_back("Pin: Enemy " + piece_type_to_string(p_between.type()) + 
                                " on " + between_sq.to_string() + " is pinned to the Queen by " + 
                                piece_type_to_string(att.type()) + " on " + attacker_sq.to_string());
                        }
                    }
                }
            }
        }
    }

    // Back-rank weakness detection
    pieces::Square enemy_king_sq = (board.active_color() == pieces::Color::White) ? black_king_sq : white_king_sq;
    if (!enemy_king_sq.is_none()) {
        int kr = enemy_king_sq.rank();
        int kf = enemy_king_sq.file();
        // If King is on back rank
        if (kr == 0 || kr == 7) {
            // Check front pawns
            int front_rank = (kr == 0) ? 1 : 6;
            int pawn_block_count = 0;
            for (int f = std::max(0, kf - 1); f <= std::min(7, kf + 1); ++f) {
                pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(f), static_cast<uint8_t>(front_rank)));
                if (p.is_present() && p.type() == pieces::PieceType::Pawn && p.color() == board.piece_at(enemy_king_sq).color()) {
                    pawn_block_count++;
                }
            }
            if (pawn_block_count >= 2) {
                themes.back_rank_weakness = true;
                themes.descriptions.push_back("Potential back-rank weakness detected around enemy king on " + enemy_king_sq.to_string());
            }
        }
    }

    if (themes.descriptions.empty()) {
        themes.descriptions.push_back("No active tactical themes detected.");
    }

    return themes;
}

PositionClassifications AnalysisTools::classify_position(const board::Board& board) {
    PositionClassifications pc;
    
    int phase = search::get_game_phase(board);
    
    // Basic phase classification
    if (phase > 200) {
        pc.opening = true;
        pc.labels.push_back("Opening");
    } else if (EndgameFramework::is_endgame(board) || phase < 60) {
        pc.endgame = true;
        pc.labels.push_back("Endgame");
    } else {
        pc.middlegame = true;
        pc.labels.push_back("Middlegame");
    }

    // Pawn count and files
    auto count_pawns_on_file = [&](int file) {
        int pawns = 0;
        for (int r = 0; r < 8; ++r) {
            pieces::Piece p = board.piece_at(pieces::Square(static_cast<uint8_t>(file), static_cast<uint8_t>(r)));
            if (p.is_present() && p.type() == pieces::PieceType::Pawn) {
                pawns++;
            }
        }
        return pawns;
    };

    int pawn_count = 0;
    int open_files = 0;
    for (int f = 0; f < 8; ++f) {
        int file_pawns = count_pawns_on_file(f);
        pawn_count += file_pawns;
        if (file_pawns == 0) {
            open_files++;
        }
    }

    if (pawn_count >= 12 && open_files <= 2) {
        pc.closed_position = true;
        pc.labels.push_back("Closed Position");
    } else if (pawn_count <= 8 && open_files >= 4) {
        pc.open_position = true;
        pc.labels.push_back("Open Position");
    } else {
        pc.semi_open = true;
        pc.labels.push_back("Semi-Open Position");
    }

    // Tactical vs Strategic
    int threats_val = std::abs(search::evaluate_threats(board));
    if (threats_val > 50) {
        pc.tactical = true;
        pc.labels.push_back("Tactical");
    } else {
        pc.strategic = true;
        pc.labels.push_back("Strategic");
    }

    // King safety attacks
    int safety_w = search::evaluate_king_safety(board, phase);
    if (safety_w < -40 || safety_w > 40) {
        pc.king_attack = true;
        pc.labels.push_back("King Attack");
    }

    // Pawn races in endgame
    if (pc.endgame) {
        bool white_passed = false;
        bool black_passed = false;
        for (uint8_t i = 0; i < 64; ++i) {
            pieces::Piece p = board.piece_at(pieces::Square(i));
            if (p.is_present() && p.type() == pieces::PieceType::Pawn) {
                // If it's a passed pawn (high evaluation weights connect connected, passed pawns)
                // For simplicity, if pawn has advanced past rank 4 (White) or rank 3 (Black)
                if (p.color() == pieces::Color::White && pieces::Square(i).rank() >= 4) white_passed = true;
                if (p.color() == pieces::Color::Black && pieces::Square(i).rank() <= 3) black_passed = true;
            }
        }
        if (white_passed && black_passed) {
            pc.pawn_race = true;
            pc.labels.push_back("Pawn Race");
        }
    }

    return pc;
}

FullAnalysisReport AnalysisTools::generate_report(board::Board& board, int depth) {
    FullAnalysisReport report;
    report.fen = board.to_fen();

    int orig_multipv = 1;
    try {
        orig_multipv = uci::OptionManager::get().get_spin("MultiPV");
    } catch (...) {}
    
    try {
        uci::OptionManager::get().set_option("MultiPV", "3");
    } catch (...) {}

    auto info = search::find_best_move(board, depth);

    try {
        uci::OptionManager::get().set_option("MultiPV", std::to_string(orig_multipv));
    } catch (...) {}

    report.best_move = info.best_move.to_string();
    report.evaluation = info.best_score;
    report.breakdown = get_evaluation_breakdown(board);
    report.alternatives = info.alternatives;
    report.explanation = explain_move(board, info.best_move);
    report.tactical_themes = detect_tactical_themes(board);
    report.classifications = classify_position(board);
    report.search_stats = info;

    int side_mult = (board.active_color() == pieces::Color::White) ? 1 : -1;
    if (side_mult * report.breakdown.material > 150) {
        report.strengths.push_back("Significant material advantage.");
    } else if (side_mult * report.breakdown.material < -150) {
        report.weaknesses.push_back("Material deficit.");
    }

    if (side_mult * report.breakdown.king_safety < -50) {
        report.weaknesses.push_back("Vulnerable king safety.");
    } else if (side_mult * report.breakdown.king_safety > 30) {
        report.strengths.push_back("Strong king safety and pawn shield.");
    }

    if (side_mult * report.breakdown.space > 30) {
        report.strengths.push_back("Superior space control.");
    } else if (side_mult * report.breakdown.space < -30) {
        report.weaknesses.push_back("Cramped position with space disadvantage.");
    }

    if (side_mult * report.breakdown.mobility > 30) {
        report.strengths.push_back("Highly active and mobile pieces.");
    } else if (side_mult * report.breakdown.mobility < -30) {
        report.weaknesses.push_back("Restricted mobility of pieces.");
    }

    if (side_mult * report.breakdown.pawn_structure < -30) {
        report.weaknesses.push_back("Weakened pawn structure (isolated, doubled, or backward pawns).");
    } else if (side_mult * report.breakdown.pawn_structure > 30) {
        report.strengths.push_back("Solid, well-connected pawn structure.");
    }

    if (report.strengths.empty()) {
        report.strengths.push_back("Structurally balanced and solid position.");
    }
    if (report.weaknesses.empty()) {
        report.weaknesses.push_back("No major positional weaknesses detected.");
    }

    return report;
}

static std::string escape_json_string(const std::string& s) {
    std::stringstream ss;
    for (char c : s) {
        if (c == '"') ss << "\\\"";
        else if (c == '\\') ss << "\\\\";
        else if (c == '\n') ss << "\\n";
        else if (c == '\t') ss << "\\t";
        else if (c == '\r') ss << "\\r";
        else ss << c;
    }
    return ss.str();
}

std::string AnalysisTools::report_to_json(const FullAnalysisReport& report) {
    std::stringstream ss;
    ss << "{\n"
       << "  \"fen\": \"" << escape_json_string(report.fen) << "\",\n"
       << "  \"best_move\": \"" << escape_json_string(report.best_move) << "\",\n"
       << "  \"evaluation\": " << report.evaluation << ",\n"
       << "  \"breakdown\": {\n"
       << "    \"material\": " << report.breakdown.material << ",\n"
       << "    \"pst\": " << report.breakdown.pst << ",\n"
       << "    \"pawn_structure\": " << report.breakdown.pawn_structure << ",\n"
       << "    \"mobility\": " << report.breakdown.mobility << ",\n"
       << "    \"piece_activity\": " << report.breakdown.piece_activity << ",\n"
       << "    \"king_safety\": " << report.breakdown.king_safety << ",\n"
       << "    \"space\": " << report.breakdown.space << ",\n"
       << "    \"threats\": " << report.breakdown.threats << ",\n"
       << "    \"endgame\": " << report.breakdown.endgame << ",\n"
       << "    \"tempo\": " << report.breakdown.tempo << ",\n"
       << "    \"total\": " << report.breakdown.total << "\n"
       << "  },\n"
       << "  \"alternatives\": [\n";
       
    for (size_t i = 0; i < report.alternatives.size(); ++i) {
        const auto& alt = report.alternatives[i];
        ss << "    {\n"
           << "      \"move\": \"" << escape_json_string(alt.move.to_string()) << "\",\n"
           << "      \"score\": " << alt.score << ",\n"
           << "      \"pv\": [";
        for (size_t j = 0; j < alt.pv.size(); ++j) {
            ss << "\"" << escape_json_string(alt.pv[j].to_string()) << "\"" << (j + 1 < alt.pv.size() ? ", " : "");
        }
        ss << "]\n"
           << "    }" << (i + 1 < report.alternatives.size() ? ",\n" : "\n");
    }
    
    ss << "  ],\n"
       << "  \"explanation\": {\n"
       << "    \"move\": \"" << escape_json_string(report.explanation.move) << "\",\n"
       << "    \"reasons\": [\n";
    for (size_t i = 0; i < report.explanation.reasons.size(); ++i) {
        ss << "      \"" << escape_json_string(report.explanation.reasons[i]) << "\"" << (i + 1 < report.explanation.reasons.size() ? ",\n" : "\n");
    }
    ss << "    ]\n"
       << "  },\n"
       << "  \"tactical_themes\": {\n"
       << "    \"fork\": " << (report.tactical_themes.fork ? "true" : "false") << ",\n"
       << "    \"pin\": " << (report.tactical_themes.pin ? "true" : "false") << ",\n"
       << "    \"skewer\": " << (report.tactical_themes.skewer ? "true" : "false") << ",\n"
       << "    \"discovered_attack\": " << (report.tactical_themes.discovered_attack ? "true" : "false") << ",\n"
       << "    \"double_attack\": " << (report.tactical_themes.double_attack ? "true" : "false") << ",\n"
       << "    \"deflection\": " << (report.tactical_themes.deflection ? "true" : "false") << ",\n"
       << "    \"decoy\": " << (report.tactical_themes.decoy ? "true" : "false") << ",\n"
       << "    \"x_ray\": " << (report.tactical_themes.x_ray ? "true" : "false") << ",\n"
       << "    \"back_rank_weakness\": " << (report.tactical_themes.back_rank_weakness ? "true" : "false") << ",\n"
       << "    \"remove_defender\": " << (report.tactical_themes.remove_defender ? "true" : "false") << ",\n"
       << "    \"zwischenzug\": " << (report.tactical_themes.zwischenzug ? "true" : "false") << ",\n"
       << "    \"descriptions\": [\n";
    for (size_t i = 0; i < report.tactical_themes.descriptions.size(); ++i) {
        ss << "      \"" << escape_json_string(report.tactical_themes.descriptions[i]) << "\"" << (i + 1 < report.tactical_themes.descriptions.size() ? ",\n" : "\n");
    }
    ss << "    ]\n"
       << "  },\n"
       << "  \"classifications\": {\n"
       << "    \"opening\": " << (report.classifications.opening ? "true" : "false") << ",\n"
       << "    \"middlegame\": " << (report.classifications.middlegame ? "true" : "false") << ",\n"
       << "    \"endgame\": " << (report.classifications.endgame ? "true" : "false") << ",\n"
       << "    \"closed_position\": " << (report.classifications.closed_position ? "true" : "false") << ",\n"
       << "    \"open_position\": " << (report.classifications.open_position ? "true" : "false") << ",\n"
       << "    \"semi_open\": " << (report.classifications.semi_open ? "true" : "false") << ",\n"
       << "    \"tactical\": " << (report.classifications.tactical ? "true" : "false") << ",\n"
       << "    \"strategic\": " << (report.classifications.strategic ? "true" : "false") << ",\n"
       << "    \"king_attack\": " << (report.classifications.king_attack ? "true" : "false") << ",\n"
       << "    \"pawn_race\": " << (report.classifications.pawn_race ? "true" : "false") << ",\n"
       << "    \"labels\": [\n";
    for (size_t i = 0; i < report.classifications.labels.size(); ++i) {
        ss << "      \"" << escape_json_string(report.classifications.labels[i]) << "\"" << (i + 1 < report.classifications.labels.size() ? ",\n" : "\n");
    }
    ss << "    ]\n"
       << "  },\n"
       << "  \"strengths\": [\n";
    for (size_t i = 0; i < report.strengths.size(); ++i) {
        ss << "    \"" << escape_json_string(report.strengths[i]) << "\"" << (i + 1 < report.strengths.size() ? ",\n" : "\n");
    }
    ss << "  ],\n"
       << "  \"weaknesses\": [\n";
    for (size_t i = 0; i < report.weaknesses.size(); ++i) {
        ss << "    \"" << escape_json_string(report.weaknesses[i]) << "\"" << (i + 1 < report.weaknesses.size() ? ",\n" : "\n");
    }
    ss << "  ],\n"
       << "  \"search_stats\": {\n"
       << "    \"nodes\": " << report.search_stats.nodes << ",\n"
       << "    \"nps\": " << (report.search_stats.time.count() > 0 ? (report.search_stats.nodes * 1000 / report.search_stats.time.count()) : 0) << ",\n"
       << "    \"time_ms\": " << report.search_stats.time.count() << ",\n"
       << "    \"cutoffs\": " << report.search_stats.cutoffs << ",\n"
       << "    \"tt_hits\": " << report.search_stats.tt_hits << "\n"
       << "  }\n"
       << "}\n";
       
    return ss.str();
}

} // namespace chess::validation
