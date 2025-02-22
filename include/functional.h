#ifndef IOTEYE_FUNCTIONAL_H
#define IOTEYE_FUNCTIONAL_H

#include <string>
#ifdef ENABLE_LOGGING
#include <iostream>
#include <sstream>
#include <unordered_map>
#endif  // !ENABLE_LOGGING

namespace ioteye {
uint8_t GetCommandCode(const std::string &cmd);
}

namespace ioteye::server::debug {
#ifdef ENABLE_LOGGING
template <typename... Args>
inline void log(Args &&...args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    std::cout << "LOG: " << oss.str() << std::endl;
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

#else
template <typename... Args>
inline void log(Args &&...args) {
    // Dummy code to prevent unused parameter warning
    (void)std::initializer_list<int>{(std::forward<Args>(args), 0)...};
}
#endif

}  // namespace ioteye::server::debug

#endif  // IOTEYE_FUNCTIONAL_H