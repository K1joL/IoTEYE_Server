#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace ioteye {
class Device;
}

namespace ioteye::types {

// utils
using ms = std::chrono::milliseconds;
using loadt = uint8_t;
using objID = size_t;

// device
// Virtual pins data type IDs
enum ContainerID { INTID = 105, DOUBLEID = 100, STRINGID = 115 };
using delayMs = std::uint16_t;
using pinsQuantity = std::uint16_t;
using pinId = std::uint16_t;
using chronoClock = std::chrono::high_resolution_clock;
using DeviceID = std::uint64_t;
using pinsTypeMap = std::unordered_map<pinId, ContainerID>;
using pinsStringMap = std::unordered_map<pinId, std::string>;
using pinsIntMap = std::unordered_map<pinId, int>;
using pinsDoubleMap = std::unordered_map<pinId, double>;
using DevicePtr = std::shared_ptr<Device>;

struct DeviceParams {
    delayMs outdatedDelay = 30;
    delayMs offlineDelay = 60;
    delayMs deadDelay = 120;
    pinsQuantity maxPins = 255;
    bool deleteAfterDeath = false;
};

enum DeviceState : uint8_t { ONLINE = 0, OUTDATED = 1, OFFLINE = 2, DEAD = 3 };

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
    }
    return "UNKNOWN";
}

}  // namespace ioteye::types