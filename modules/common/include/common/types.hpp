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

struct DeviceParams {
    DelayMs outdatedDelay = 30;
    DelayMs offlineDelay = 60;
    DelayMs deadDelay = 120;
    PinsQuantity maxPins = 255;
    bool deleteAfterDeath = false;
};

enum DeviceState : std::uint8_t {
    ONLINE = 0,
    OUTDATED = 1,
    OFFLINE = 2,
    DEAD = 3,
    MAX_STATE
};

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