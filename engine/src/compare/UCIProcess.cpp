#include "UCIProcess.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include "Logger.h"

namespace chess::compare {

UCIProcess::UCIProcess(const std::string& path) : m_path(path) {}

UCIProcess::~UCIProcess() {
    stop();
}

bool UCIProcess::start() {
    if (m_running) return true;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStdoutWrite = NULL;
    HANDLE hChildStdinRead = NULL;

    // Create a pipe for the child process's STDOUT
    if (!CreatePipe(&m_hChildStdoutRead, &hChildStdoutWrite, &saAttr, 0)) {
        return false;
    }
    // Ensure the read handle to the pipe for STDOUT is not inherited
    if (!SetHandleInformation(m_hChildStdoutRead, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(m_hChildStdoutRead);
        CloseHandle(hChildStdoutWrite);
        return false;
    }

    // Create a pipe for the child process's STDIN
    if (!CreatePipe(&hChildStdinRead, &m_hChildStdinWrite, &saAttr, 0)) {
        CloseHandle(m_hChildStdoutRead);
        CloseHandle(hChildStdoutWrite);
        return false;
    }
    // Ensure the write handle to the pipe for STDIN is not inherited
    if (!SetHandleInformation(m_hChildStdinWrite, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(m_hChildStdoutRead);
        CloseHandle(hChildStdoutWrite);
        CloseHandle(hChildStdinRead);
        CloseHandle(m_hChildStdinWrite);
        return false;
    }

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdError = hChildStdoutWrite;
    siStartInfo.hStdOutput = hChildStdoutWrite;
    siStartInfo.hStdInput = hChildStdinRead;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Wrap command line in quotes if it contains spaces and is not already quoted
    std::string cmdLine = m_path;
    if (cmdLine.find(' ') != std::string::npos && cmdLine.front() != '"') {
        cmdLine = "\"" + cmdLine + "\"";
    }

    // Create the child process
    BOOL bSuccess = CreateProcessA(
        NULL,
        const_cast<char*>(cmdLine.c_str()), // command line
        NULL,          // process security attributes
        NULL,          // primary thread security attributes
        TRUE,          // handles are inherited
        CREATE_NO_WINDOW, // creation flags
        NULL,          // use parent's environment
        NULL,          // use parent's current directory
        &siStartInfo,  // STARTUPINFO pointer
        &piProcInfo    // receives PROCESS_INFORMATION
    );

    // Close handles to the child's end of the pipes we no longer need in the parent
    CloseHandle(hChildStdoutWrite);
    CloseHandle(hChildStdinRead);

    if (!bSuccess) {
        CloseHandle(m_hChildStdoutRead);
        CloseHandle(m_hChildStdinWrite);
        m_hChildStdoutRead = NULL;
        m_hChildStdinWrite = NULL;
        return false;
    }

    m_hChildProcess = piProcInfo.hProcess;
    CloseHandle(piProcInfo.hThread); // We don't need the thread handle

    m_running = true;
    m_lineQueue.clear();

    // Start reader thread
    m_readerThread = std::thread(&UCIProcess::readerLoop, this);

    // Initialize UCI protocol
    sendCommand("uci");
    std::string uciRes = readUntil("uciok");
    if (uciRes.find("uciok") == std::string::npos) {
        stop();
        return false;
    }

    return true;
}

void UCIProcess::stop() {
    if (!m_running) return;

    m_running = false;

    // Close stdin write handle to signal EOF
    if (m_hChildStdinWrite != NULL) {
        CloseHandle(m_hChildStdinWrite);
        m_hChildStdinWrite = NULL;
    }

    // Terminate process if still active after a short wait
    if (m_hChildProcess != NULL) {
        DWORD exitCode;
        if (GetExitCodeProcess(m_hChildProcess, &exitCode) && exitCode == STILL_ACTIVE) {
            TerminateProcess(m_hChildProcess, 0);
        }
        CloseHandle(m_hChildProcess);
        m_hChildProcess = NULL;
    }

    // Close stdout read handle
    if (m_hChildStdoutRead != NULL) {
        CloseHandle(m_hChildStdoutRead);
        m_hChildStdoutRead = NULL;
    }

    // Join reader thread
    if (m_readerThread.joinable()) {
        m_readerThread.join();
    }
}

bool UCIProcess::sendCommand(const std::string& command) {
    if (!m_running || m_hChildStdinWrite == NULL) return false;

    std::string cmd = command + "\n";
    DWORD dwWritten;
    BOOL bSuccess = WriteFile(m_hChildStdinWrite, cmd.c_str(), (DWORD)cmd.length(), &dwWritten, NULL);
    return bSuccess && (dwWritten == cmd.length());
}

std::string UCIProcess::readLine(int timeout_ms) {
    std::string line;
    if (m_lineQueue.pop(line, timeout_ms)) {
        Logger::log("[" + m_path + " stdout]: " + line);
        return line;
    }
    return "";
}

std::string UCIProcess::readUntil(const std::string& prefix, int timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    std::string accumulated;
    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        int elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        int remaining = timeout_ms - elapsed;
        if (remaining <= 0) break;

        std::string line = readLine(remaining);
        if (line.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        accumulated += line + "\n";
        if (line.rfind(prefix, 0) == 0) { // starts_with
            return accumulated;
        }
    }

    // Drain whatever is left in the queue just in case
    std::string line;
    while (m_lineQueue.pop(line, 0)) {
        accumulated += line + "\n";
        if (line.rfind(prefix, 0) == 0) {
            return accumulated;
        }
    }

    return accumulated;
}

bool UCIProcess::waitReady(int timeout_ms) {
    sendCommand("isready");
    std::string response = readUntil("readyok", timeout_ms);
    return response.find("readyok") != std::string::npos;
}

void UCIProcess::newGame() {
    sendCommand("ucinewgame");
    waitReady();
}

void UCIProcess::setPosition(const std::string& fen) {
    sendCommand("position fen " + fen);
}

void UCIProcess::goDepth(int depth) {
    sendCommand("go depth " + std::to_string(depth));
}

void UCIProcess::goMovetime(int ms) {
    sendCommand("go movetime " + std::to_string(ms));
}

void UCIProcess::quit() {
    sendCommand("quit");
    stop();
}

void UCIProcess::readerLoop() {
    char buf[4096];
    DWORD dwRead;
    std::string accumulated;

    while (m_running && m_hChildStdoutRead != NULL) {
        BOOL bSuccess = ReadFile(m_hChildStdoutRead, buf, sizeof(buf) - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) {
            break;
        }

        buf[dwRead] = '\0';
        accumulated += buf;

        size_t pos;
        while ((pos = accumulated.find('\n')) != std::string::npos) {
            std::string line = accumulated.substr(0, pos);
            accumulated.erase(0, pos + 1);

            // Strip carriage returns if any
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            std::string trimmed = line;
            while (!trimmed.empty() && std::isspace(trimmed.back())) trimmed.pop_back();
            while (!trimmed.empty() && std::isspace(trimmed.front())) trimmed.erase(trimmed.begin());

            if (!trimmed.empty()) {
                m_lineQueue.push(line);
            }
        }
    }
    m_running = false;
}

} // namespace chess::compare
