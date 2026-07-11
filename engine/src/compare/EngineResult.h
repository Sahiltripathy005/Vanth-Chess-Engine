#ifndef ENGINE_RESULT_H
#define ENGINE_RESULT_H

#include <string>
#include <vector>

namespace chess::compare {

struct EngineResult {
    std::string engineName;
    std::string bestMove;
    int evaluationCp = 0;
    int mateScore = 0;
    bool hasMate = false;
    int depth = 0;
    uint64_t nodes = 0;
    uint64_t nps = 0;
    int hashfull = 0;
    std::vector<std::string> pv;
    long long searchTimeMs = 0;
};

} // namespace chess::compare

#endif // ENGINE_RESULT_H
