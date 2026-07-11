#include "CSVWriter.h"
#include <fstream>
#include <iomanip>
#include <iostream>

namespace chess::compare {

bool CSVWriter::writeResults(const std::string& path, const std::vector<ComparisonResult>& results) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open CSV file for writing: " << path << std::endl;
        return false;
    }

    // Write header
    file << "FEN,Vanth Move,Stockfish Move,Vanth Eval,Stockfish Eval,Difference,Depth,Nodes,PV Agreement,Search Time\n";

    for (const auto& r : results) {
        file << "\"" << r.fen << "\",";
        file << r.vanth.bestMove << ",";
        file << r.stockfish.bestMove << ",";
        
        if (r.vanth.hasMate) {
            file << "mate " << r.vanth.mateScore << ",";
        } else {
            file << std::fixed << std::setprecision(2) << (r.vanth.evaluationCp / 100.0) << ",";
        }

        if (r.stockfish.hasMate) {
            file << "mate " << r.stockfish.mateScore << ",";
        } else {
            file << std::fixed << std::setprecision(2) << (r.stockfish.evaluationCp / 100.0) << ",";
        }

        file << std::fixed << std::setprecision(2) << r.evaluationDifference << ",";
        file << r.vanth.depth << ",";
        file << r.vanth.nodes << ",";
        file << r.pvPrefixLength << ",";
        file << r.vanth.searchTimeMs << "\n";
    }

    return true;
}

} // namespace chess::compare
