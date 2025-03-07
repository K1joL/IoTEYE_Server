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