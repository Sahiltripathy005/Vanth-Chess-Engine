#ifndef POSITION_LOADER_H
#define POSITION_LOADER_H

#include <string>
#include <vector>

namespace chess::compare {

class PositionLoader {
public:
    static std::vector<std::string> loadFEN(const std::string& fen);
    static std::vector<std::string> loadFromFile(const std::string& path);
    static std::vector<std::string> loadFromPGN(const std::string& path);
};

} // namespace chess::compare

#endif // POSITION_LOADER_H
