#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "device.h"
#include "functional.h"

namespace ioteye {
class FileHandler {
public:
    static bool saveDevices(const std::unordered_map<uint64_t, DevicePtr>& devices,
                            const std::string& filename);
    static bool loadDevices(std::unordered_map<uint64_t, DevicePtr>& devices, const std::string& filename);
};
}  // namespace ioteye

#endif  // FILE_HANDLER_H