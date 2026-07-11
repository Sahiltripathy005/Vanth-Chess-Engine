#ifndef COMPARISON_RESULT_H
#define COMPARISON_RESULT_H

#include "EngineResult.h"
#include <string>

namespace chess::compare {

struct ComparisonResult {
    std::string fen;
    EngineResult vanth;
    EngineResult stockfish;
    bool sameMove = false;
    bool moveMatchesPV = false;
    double evaluationDifference = 0.0;
    int pvPrefixLength = 0;
    std::string winner;
    std::string notes;
};

} // namespace chess::compare

#endif // COMPARISON_RESULT_H
