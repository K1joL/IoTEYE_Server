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

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>

namespace ioteye {
class Device;
}

namespace ioteye::types {

// utils
using ms = std::chrono::milliseconds;
using loadt = uint8_t;
using ObjID = size_t;

// device
// Virtual pins data type IDs
enum ContainerID { INTID = 105, DOUBLEID = 100, STRINGID = 115 };
using DelayMs = std::uint16_t;
using PinsQuantity = std::uint16_t;
using PinId = std::uint16_t;
using ChronoClock = std::chrono::high_resolution_clock;
using DeviceID = std::uint64_t;
using PinsTypeMap = std::unordered_map<PinId, ContainerID>;
using PinsStringMap = std::unordered_map<PinId, std::string>;
using PinsIntMap = std::unordered_map<PinId, int>;
using PinsDoubleMap = std::unordered_map<PinId, double>;
using DevicePtr = std::shared_ptr<Device>;
using Token = std::string;
using DeviceMap = std::unordered_map<types::DeviceID, types::DevicePtr>;

constexpr DeviceID NullDeviceID = std::numeric_limits<DeviceID>::max();

struct DeviceParams {
    DelayMs outdatedDelay = 30;
    DelayMs offlineDelay = 60;
    DelayMs deadDelay = 120;
    PinsQuantity maxPins = 255;
    bool deleteAfterDeath = false;
};

enum DeviceState : std::uint8_t { ONLINE = 0, OUTDATED = 1, OFFLINE = 2, DEAD = 3, MAX_STATE };

inline const char* toString(DeviceState s) {
    switch (s) {
        case ONLINE:
            return "ONLINE";
        case OUTDATED:
            return "OUTDATED";
        case OFFLINE:
            return "OFFLINE";
        case DEAD:
            return "DEAD";
        default:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

}  // namespace ioteye::types