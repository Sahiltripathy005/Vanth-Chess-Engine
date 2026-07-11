#include "compare/ComparisonRunner.h"
#include "compare/PositionLoader.h"
#include "compare/CSVWriter.h"
#include "compare/Logger.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>

using namespace chess::compare;

void printUsage();

int main(int argc, char* argv[]) {
    std::string fen;
    std::string positionsFile;
    std::string pgnFile;
    int depth = 10;
    int movetime = 0;
    std::string stockfishPath = "D:\\requirements\\Stockfish\\stockfish\\stockfish.exe";
    std::string vanthPath = "engine/build/benchmarks/Release/chess_benchmarks.exe"; // default
    std::string csvPath = "comparison_results.csv";
    std::string logPath = "comparison.log";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--fen" && i + 1 < argc) {
            fen = argv[++i];
        } else if (arg == "--positions" && i + 1 < argc) {
            positionsFile = argv[++i];
        } else if (arg == "--pgn" && i + 1 < argc) {
            pgnFile = argv[++i];
        } else if (arg == "--depth" && i + 1 < argc) {
            depth = std::stoi(argv[++i]);
            movetime = 0; // Depth overrides movetime
        } else if (arg == "--movetime" && i + 1 < argc) {
            movetime = std::stoi(argv[++i]);
            depth = 0; // Movetime overrides depth
        } else if (arg == "--stockfish" && i + 1 < argc) {
            stockfishPath = argv[++i];
        } else if (arg == "--vanth" && i + 1 < argc) {
            vanthPath = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            csvPath = argv[++i];
        } else if (arg == "--log" && i + 1 < argc) {
            logPath = argv[++i];
        }
    }

    // Initialize Logger
    Logger::init(logPath);
    Logger::log("=========================================================");
    Logger::log("Starting Vanth vs Stockfish Comparison Harness");
    Logger::log("=========================================================");
    Logger::log("Vanth path: " + vanthPath);
    Logger::log("Stockfish path: " + stockfishPath);

    // Load positions
    std::vector<std::string> fens;
    if (!fen.empty()) {
        fens = PositionLoader::loadFEN(fen);
    } else if (!positionsFile.empty()) {
        fens = PositionLoader::loadFromFile(positionsFile);
    } else if (!pgnFile.empty()) {
        fens = PositionLoader::loadFromPGN(pgnFile);
    } else {
        // Default to start position if nothing is specified
        fens = { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
        Logger::log("No position specified. Defaulting to start position.");
    }

    if (fens.empty()) {
        Logger::log("ERROR: No positions loaded. Exiting.");
        Logger::close();
        return 1;
    }

    Logger::log("Loaded " + std::to_string(fens.size()) + " positions to evaluate.");

    // Initialize comparison runner
    ComparisonRunner runner(vanthPath, stockfishPath);
    if (!runner.initialize()) {
        Logger::log("ERROR: Failed to initialize comparison runner. Check paths and logs.");
        Logger::close();
        return 1;
    }

    std::vector<ComparisonResult> results;
    int sameMoveCount = 0;
    int processedCount = 0;
    double totalEvalDiff = 0.0;
    int totalPVAgreement = 0;
    int validEvalCount = 0;

    for (size_t idx = 0; idx < fens.size(); ++idx) {
        std::cout << "\nEvaluating position " << (idx + 1) << " / " << fens.size() << "...\n";
        
        ComparisonResult res = runner.comparePosition(fens[idx], depth, movetime);
        
        // Print console report immediately
        runner.printConsoleReport(res);

        results.push_back(res);
        processedCount++;

        if (res.vanth.bestMove.empty() || res.stockfish.bestMove.empty()) {
            continue; // Skip statistics for failed searches
        }

        if (res.sameMove) sameMoveCount++;
        
        if (!res.vanth.hasMate && !res.stockfish.hasMate) {
            totalEvalDiff += res.evaluationDifference;
            validEvalCount++;
        }
        totalPVAgreement += res.pvPrefixLength;
    }

    // Write CSV results
    Logger::log("Writing CSV results to: " + csvPath);
    if (!CSVWriter::writeResults(csvPath, results)) {
        Logger::log("ERROR: Failed to write CSV file.");
    }

    // Print summary stats
    double agreementPct = (processedCount > 0) ? (double)sameMoveCount / processedCount * 100.0 : 0.0;
    double avgEvalDiff = (validEvalCount > 0) ? totalEvalDiff / validEvalCount : 0.0;
    double avgPVAgreement = (processedCount > 0) ? (double)totalPVAgreement / processedCount : 0.0;

    std::cout << "\n=========================================================\n";
    std::cout << "SUMMARY STATISTICS\n";
    std::cout << "=========================================================\n";
    std::cout << "Positions Evaluated      : " << processedCount << "\n";
    std::cout << "Best Move Agreement      : " << sameMoveCount << " / " << processedCount 
              << " (" << std::fixed << std::setprecision(2) << agreementPct << "%)\n";
    std::cout << "Average Eval Difference  : " << std::fixed << std::setprecision(2) << avgEvalDiff << " pawns\n";
    std::cout << "Average PV Agreement     : " << std::fixed << std::setprecision(2) << avgPVAgreement << " plies\n";
    std::cout << "=========================================================\n\n";

    Logger::log("Comparison process finished successfully.");
    Logger::close();
    return 0;
}

void printUsage() {
    std::cout << "Vanth Stockfish Comparison Harness\n\n";
    std::cout << "Usage:\n";
    std::cout << "  vanth_compare [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --fen <fen>            Single FEN position to evaluate\n";
    std::cout << "  --positions <file>     Text file containing FENs, one per line\n";
    std::cout << "  --pgn <file>           PGN file to convert and evaluate positions\n";
    std::cout << "  --depth <depth>        Search depth limit (default: 10)\n";
    std::cout << "  --movetime <ms>        Search time limit per move in milliseconds\n";
    std::cout << "  --stockfish <path>     Path to Stockfish executable (default: D:\\requirements\\Stockfish\\stockfish\\stockfish.exe)\n";
    std::cout << "  --vanth <path>         Path to Vanth executable (default: engine/build/benchmarks/Release/chess_benchmarks.exe)\n";
    std::cout << "  --csv <file>           Output CSV filepath (default: comparison_results.csv)\n";
    std::cout << "  --log <file>           Output log filepath (default: comparison.log)\n";
}
