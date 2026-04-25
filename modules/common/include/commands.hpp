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

#ifndef IOTEYE_COMMON_COMMANDS_HPP
#define IOTEYE_COMMON_COMMANDS_HPP

#include <stdint.h>

#include <string>

namespace ioteye {
enum COMMANDS {
    NON_COMMAND = 0,
    REGISTER_DEVICE = 'r' + 'd',       // 214
    DELETE_DEVICE = 'd' + 'd',         // 200
    DEVICE_STATUS = 'd' + 's',         // 215
    DEVICE_STATUS_UPDATE = 'u' + 's',  // 232
    CREATE_PIN = 'c' + 'p',            // 211
    UPDATE_PIN = 'u' + 'p',            // 229
    DELETE_PIN = 'd' + 'p',            // 212
    GET_PIN = 'p' + 'v',               // 230
    COMMANDS_MAX = 9
};
uint8_t GetCommandCode(const std::string& cmd);

}  // namespace ioteye

#endif  // IOTEYE_COMMON_COMMANDS_HPP
