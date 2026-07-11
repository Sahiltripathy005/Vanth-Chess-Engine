#define _CRT_SECURE_NO_WARNINGS
#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>

namespace chess::compare {

std::ofstream Logger::m_logFile;
std::mutex Logger::m_mutex;
std::string Logger::m_path;

void Logger::init(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_path = path;
    m_logFile.open(path, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Warning: Could not open log file: " << path << std::endl;
    }
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    std::string formatted = "[" + ss.str() + "] " + message;
    
    // Write to file
    if (m_logFile.is_open()) {
        m_logFile << formatted << "\n";
        m_logFile.flush();
    }
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

} // namespace chess::compare
