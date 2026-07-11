#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

namespace chess::compare {

class Logger {
public:
    static void init(const std::string& path);
    static void log(const std::string& message);
    static void close();

private:
    static std::ofstream m_logFile;
    static std::mutex m_mutex;
    static std::string m_path;
};

} // namespace chess::compare

#endif // LOGGER_H
