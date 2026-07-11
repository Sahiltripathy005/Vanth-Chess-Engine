#ifndef COMPARISON_RUNNER_H
#define COMPARISON_RUNNER_H

#include "UCIProcess.h"
#include "EngineResult.h"
#include "ComparisonResult.h"
#include <string>
#include <vector>
#include <memory>

namespace chess::compare {

class ComparisonRunner {
public:
    ComparisonRunner(const std::string& vanthPath, const std::string& stockfishPath);
    ~ComparisonRunner();

    bool initialize();
    
    ComparisonResult comparePosition(const std::string& fen, int depth, int movetime_ms);

    void printConsoleReport(const ComparisonResult& res);

private:
    EngineResult runEngineSearch(UCIProcess& process, const std::string& fen, int depth, int movetime_ms);
    void parseUCILine(const std::string& line, EngineResult& result);

    std::string m_vanthPath;
    std::string m_stockfishPath;
    
    std::unique_ptr<UCIProcess> m_vanth;
    std::unique_ptr<UCIProcess> m_stockfish;
};

} // namespace chess::compare

#endif // COMPARISON_RUNNER_H
