#ifndef UCI_PROCESS_H
#define UCI_PROCESS_H

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace chess::compare {

template <typename T>
class SafeQueue {
public:
    void push(const T& val) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(val);
        m_cond.notify_one();
    }

    bool pop(T& val, int timeout_ms) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            if (m_cond.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return !m_queue.empty(); })) {
                val = m_queue.front();
                m_queue.pop();
                return true;
            }
            return false;
        }
        val = m_queue.front();
        m_queue.pop();
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<T> empty;
        std::swap(m_queue, empty);
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
};

class UCIProcess {
public:
    UCIProcess(const std::string& path);
    ~UCIProcess();

    bool start();
    void stop();

    bool sendCommand(const std::string& command);
    std::string readLine(int timeout_ms = 10000);
    
    // Reads output until we see a line starting with target prefix
    std::string readUntil(const std::string& prefix, int timeout_ms = 10000);
    
    // Helper to send ready/isready check
    bool waitReady(int timeout_ms = 10000);

    void newGame();
    void setPosition(const std::string& fen);
    void goDepth(int depth);
    void goMovetime(int ms);
    void quit();
    
    bool isRunning() const { return m_running; }
    const std::string& getPath() const { return m_path; }

private:
    void readerLoop();

    std::string m_path;
    std::atomic<bool> m_running{false};

    HANDLE m_hChildProcess = NULL;
    HANDLE m_hChildStdinWrite = NULL;
    HANDLE m_hChildStdoutRead = NULL;

    std::thread m_readerThread;
    SafeQueue<std::string> m_lineQueue;
};

} // namespace chess::compare

#endif // UCI_PROCESS_H
