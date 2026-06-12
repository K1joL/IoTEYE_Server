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

#ifndef IOTEYE_COMMON_IOTEYE_LOGGING_HPP
#define IOTEYE_COMMON_IOTEYE_LOGGING_HPP

#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#ifdef ENABLE_LOGGING
#include <chrono>
#include <sstream>
#endif  // !ENABLE_LOGGING

namespace ioteye::server::debug {
enum LogLevel { STATUS, INFO, WARNING, ERROR };

inline std::string getLevelString(LogLevel level) {
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

// << operator overload specifically for std::unordered_map
template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, V>& map) {
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

#ifdef ENABLE_LOGGING
static std::mutex logMutex;
static auto start = std::chrono::high_resolution_clock::now();

inline std::chrono::duration<double> getTimestamp() {
    return std::chrono::high_resolution_clock::now() - start;
}

inline void printPrefix(LogLevel level) {
    std::cout << '[' << getTimestamp().count() << "] [" << getLevelString(level) << "] ";
}

template <typename... Args>
inline void log(LogLevel level, Args&&... args) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    printPrefix(level);
    std::cout << oss.str();
    std::cout.flush();
}

template <typename... Args>
inline void logln(LogLevel level, Args&&... args) {
    log(level, std::forward<Args>(args)..., '\n');
}

template <typename... Args>
inline void logStatus(Args&&... args) {
    logln(LogLevel::STATUS, std::forward<Args>(args)...);
}

#else
template <typename... Args>
inline void log(Args&&...) {
    // Dummy code to prevent unused parameter warning
}

template <typename... Args>
inline void logStatus(Args&&...) {
    // Dummy code to prevent unused parameter warning
}

template <typename... Args>
inline void logln(Args&&...) {
    // Dummy code to prevent unused parameter warning
}
#endif

}  // namespace ioteye::server::debug

#endif  // IOTEYE_COMMON_IOTEYE_LOGGING_HPP