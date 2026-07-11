#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include "ComparisonResult.h"
#include <string>
#include <vector>

namespace chess::compare {

class CSVWriter {
public:
    static bool writeResults(const std::string& path, const std::vector<ComparisonResult>& results);
};

} // namespace chess::compare

#endif // CSV_WRITER_H
