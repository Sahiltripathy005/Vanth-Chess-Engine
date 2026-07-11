#include "ComparisonRunner.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <chrono>
#include "Logger.h"

namespace chess::compare {

ComparisonRunner::ComparisonRunner(const std::string& vanthPath, const std::string& stockfishPath)
    : m_vanthPath(vanthPath), m_stockfishPath(stockfishPath) {}

ComparisonRunner::~ComparisonRunner() {
    if (m_vanth) m_vanth->quit();
    if (m_stockfish) m_stockfish->quit();
}

bool ComparisonRunner::initialize() {
    Logger::log("Initializing engines...");
    
    m_vanth = std::make_unique<UCIProcess>(m_vanthPath);
    if (!m_vanth->start()) {
        Logger::log("ERROR: Failed to start Vanth at path: " + m_vanthPath);
        return false;
    }
    Logger::log("Vanth started successfully.");

    m_stockfish = std::make_unique<UCIProcess>(m_stockfishPath);
    if (!m_stockfish->start()) {
        Logger::log("ERROR: Failed to start Stockfish at path: " + m_stockfishPath);
        return false;
    }
    Logger::log("Stockfish started successfully.");

    return true;
}

ComparisonResult ComparisonRunner::comparePosition(const std::string& fen, int depth, int movetime_ms) {
    ComparisonResult res;
    res.fen = fen;

    Logger::log("Comparing position: " + fen);

    // Check if processes are running, restart if they crashed
    if (!m_vanth->isRunning()) {
        Logger::log("WARNING: Vanth process is not running. Restarting...");
        m_vanth->stop();
        if (!m_vanth->start()) {
            Logger::log("ERROR: Failed to restart Vanth");
            res.notes = "Vanth crashed/failed to start";
            return res;
        }
    }
    if (!m_stockfish->isRunning()) {
        Logger::log("WARNING: Stockfish process is not running. Restarting...");
        m_stockfish->stop();
        if (!m_stockfish->start()) {
            Logger::log("ERROR: Failed to restart Stockfish");
            res.notes = "Stockfish crashed/failed to start";
            return res;
        }
    }

    // Run searches
    EngineResult vanthRes;
    vanthRes.engineName = "Vanth";
    try {
        Logger::log("Running Vanth search...");
        vanthRes = runEngineSearch(*m_vanth, fen, depth, movetime_ms);
        vanthRes.engineName = "Vanth";
    } catch (const std::exception& e) {
        Logger::log("Exception searching with Vanth: " + std::string(e.what()));
        res.notes = "Vanth search failed: " + std::string(e.what());
    }

    EngineResult sfRes;
    sfRes.engineName = "Stockfish";
    try {
        Logger::log("Running Stockfish search...");
        sfRes = runEngineSearch(*m_stockfish, fen, depth, movetime_ms);
        sfRes.engineName = "Stockfish";
    } catch (const std::exception& e) {
        Logger::log("Exception searching with Stockfish: " + std::string(e.what()));
        res.notes = "Stockfish search failed: " + std::string(e.what());
    }

    // Timeout / Hang handling
    if (vanthRes.bestMove.empty()) {
        Logger::log("WARNING: Vanth timed out or returned no move. Restarting process...");
        m_vanth->stop();
        m_vanth->start();
        res.notes += " [Vanth timeout]";
    }
    if (sfRes.bestMove.empty()) {
        Logger::log("WARNING: Stockfish timed out or returned no move. Restarting process...");
        m_stockfish->stop();
        m_stockfish->start();
        res.notes += " [Stockfish timeout]";
    }

    res.vanth = vanthRes;
    res.stockfish = sfRes;

    if (vanthRes.bestMove.empty() || sfRes.bestMove.empty()) {
        return res;
    }

    // Move Match
    res.sameMove = (vanthRes.bestMove == sfRes.bestMove);

    // Move matches Stockfish PV
    res.moveMatchesPV = false;
    for (const auto& mv : sfRes.pv) {
        if (vanthRes.bestMove == mv) {
            res.moveMatchesPV = true;
            break;
        }
    }

    // Eval difference calculation
    if (vanthRes.hasMate && sfRes.hasMate) {
        res.evaluationDifference = 0.0;
        res.notes += "Both found mate. Vanth: mate in " + std::to_string(vanthRes.mateScore) + 
                     ", Stockfish: mate in " + std::to_string(sfRes.mateScore);
    } else if (vanthRes.hasMate && !sfRes.hasMate) {
        res.evaluationDifference = 99.9;
        res.notes += "Vanth found mate in " + std::to_string(vanthRes.mateScore) + " but Stockfish score is " + 
                     std::to_string(sfRes.evaluationCp) + " cp";
    } else if (!vanthRes.hasMate && sfRes.hasMate) {
        res.evaluationDifference = 99.9;
        res.notes += "Stockfish found mate in " + std::to_string(sfRes.mateScore) + " but Vanth score is " + 
                     std::to_string(vanthRes.evaluationCp) + " cp (Missed mate)";
    } else {
        res.evaluationDifference = std::abs(vanthRes.evaluationCp - sfRes.evaluationCp) / 100.0;
    }

    // PV prefix agreement
    int agreement = 0;
    for (size_t i = 0; i < std::min(vanthRes.pv.size(), sfRes.pv.size()); ++i) {
        if (vanthRes.pv[i] == sfRes.pv[i]) {
            agreement++;
        } else {
            break;
        }
    }
    res.pvPrefixLength = agreement;

    return res;
}

void ComparisonRunner::printConsoleReport(const ComparisonResult& res) {
    std::cout << "=========================================================\n";
    std::cout << "Position FEN: " << res.fen << "\n";
    std::cout << "---------------------------------------------------------\n";
    
    auto printEngine = [](const EngineResult& eng) {
        std::cout << eng.engineName << "\n";
        std::cout << "  Best Move : " << (eng.bestMove.empty() ? "None (Failed)" : eng.bestMove) << "\n";
        if (eng.hasMate) {
            std::cout << "  Eval      : mate in " << eng.mateScore << "\n";
        } else {
            double eval = eng.evaluationCp / 100.0;
            std::cout << "  Eval      : " << (eval >= 0.0 ? "+" : "") << std::fixed << std::setprecision(2) << eval << "\n";
        }
        std::cout << "  Depth     : " << eng.depth << "\n";
        std::cout << "  Nodes     : " << eng.nodes << "\n";
        std::cout << "  NPS       : " << eng.nps << "\n";
        std::cout << "  PV        : ";
        for (const auto& mv : eng.pv) {
            std::cout << mv << " ";
        }
        std::cout << "\n";
    };

    printEngine(res.vanth);
    std::cout << "---------------------------------------------------------\n";
    printEngine(res.stockfish);
    std::cout << "---------------------------------------------------------\n";
    std::cout << "Move Match            : " << (res.sameMove ? "YES" : "NO") << "\n";
    if (res.vanth.hasMate || res.stockfish.hasMate) {
        std::cout << "Evaluation Difference : N/A (Mate involved)\n";
    } else {
        std::cout << "Evaluation Difference : " << std::fixed << std::setprecision(2) << res.evaluationDifference << "\n";
    }
    std::cout << "PV Prefix Agreement   : " << res.pvPrefixLength << " plies\n";
    if (!res.notes.empty()) {
        std::cout << "Notes                 : " << res.notes << "\n";
    }
    std::cout << "=========================================================\n\n";
}

EngineResult ComparisonRunner::runEngineSearch(UCIProcess& process, const std::string& fen, int depth, int movetime_ms) {
    EngineResult result;
    result.engineName = process.getPath();
    
    process.newGame();
    process.setPosition(fen);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    if (depth > 0) {
        process.goDepth(depth);
    } else {
        process.goMovetime(movetime_ms);
    }

    // Read lines until "bestmove" is received
    std::string line;
    while (true) {
        line = process.readLine(30000); // 30 second timeout
        if (line.empty()) {
            break;
        }
        
        parseUCILine(line, result);
        if (line.rfind("bestmove", 0) == 0) {
            break;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.searchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    return result;
}

void ComparisonRunner::parseUCILine(const std::string& line, EngineResult& result) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    
    if (token == "bestmove") {
        iss >> result.bestMove;
    } else if (token == "info") {
        std::string key;
        while (iss >> key) {
            if (key == "depth") {
                iss >> result.depth;
            } else if (key == "score") {
                std::string scoreType;
                iss >> scoreType;
                if (scoreType == "cp") {
                    iss >> result.evaluationCp;
                    result.hasMate = false;
                } else if (scoreType == "mate") {
                    iss >> result.mateScore;
                    result.hasMate = true;
                }
            } else if (key == "nodes") {
                iss >> result.nodes;
            } else if (key == "nps") {
                iss >> result.nps;
            } else if (key == "hashfull") {
                iss >> result.hashfull;
            } else if (key == "pv") {
                result.pv.clear();
                std::string move;
                while (iss >> move) {
                    result.pv.push_back(move);
                }
                break; // pv is always the last field in the info line
            }
        }
    }
}

} // namespace chess::compare
