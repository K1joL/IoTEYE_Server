/*# MIT License

# Copyright (c) 2025 Shults Bogdan aka K1joL

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/

#ifndef IOTEYE_FUNCTIONAL_H
#define IOTEYE_FUNCTIONAL_H

#include <mutex>
#include <string>
#ifdef ENABLE_LOGGING
#include <chrono>
#include <iostream>
#include <sstream>
#include <unordered_map>
#endif  // !ENABLE_LOGGING

namespace ioteye {
uint8_t GetCommandCode(const std::string& cmd);
}

namespace ioteye::server::debug {
#ifdef ENABLE_LOGGING
enum LogLevel { STATUS, INFO, WARNING, ERROR };

static std::mutex logMutex;
static auto start = std::chrono::high_resolution_clock::now();

std::string getLevelString(uint8_t level) {
    switch (level) {
        case LogLevel::STATUS:
            return "STATUS";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

inline std::chrono::duration<double> getTimestamp() {
    return std::chrono::high_resolution_clock::now() - start;
}

inline void printPrefix(uint8_t level) {
    std::cout << '[' << getTimestamp().count() << "] [" << getLevelString(level)
              << "] ";
}

template <typename... Args>
inline void log(Args&&... args) {
    log(LogLevel::STATUS, args);
}

template <typename... Args>
inline void logln(Args&&... args) {
    log(args, std::endl);
}

template <typename... Args>
inline void logln(uint8_t level, Args&&... args) {
    log(level, args, std::endl);
}

template <typename... Args>
inline void log(uint8_t level, Args&&... args) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    printPrefix();
    std::cout << oss.str();
}

// << operator overload specifically for std::unordered_map
template <typename K, typename V>
std::ostream& operator<<(std::ostream& os,
                         const std::unordered_map<K, V>& map) {
    os << "{";
    bool first = true;
    for (const auto& pair : map) {
        if (!first)
            os << ", ";
        first = false;
        os << pair.first << ": " << pair.second;
    }
    os << "}";
    return os;
}

#else
template <typename... Args>
inline void log(Args&&...) {
    // Dummy code to prevent unused parameter warning
}

template <typename... Args>
inline void logln(Args&&...) {
    // Dummy code to prevent unused parameter warning
}
#endif

}  // namespace ioteye::server::debug

#endif  // IOTEYE_FUNCTIONAL_H