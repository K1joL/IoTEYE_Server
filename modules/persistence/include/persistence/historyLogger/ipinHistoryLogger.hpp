#ifndef IOTEYE_INTERFACE_PIN_HISTORY_LOGGER_HPP
#define IOTEYE_INTERFACE_PIN_HISTORY_LOGGER_HPP

#include <cstdint>
#include <device/device.hpp>
#include <memory>
#include <string>

namespace ioteye::persistence {

class IPinHistoryLogger {
public:
    virtual ~IPinHistoryLogger() = default;

    virtual void record(const ioteye::Device& device) = 0;
    virtual std::string query(types::DeviceID deviceId, int64_t since, int64_t until,
                              uint32_t limit) const = 0;
};

}  // namespace ioteye::persistence

#endif  // IOTEYE_INTERFACE_PIN_HISTORY_LOGGER_HPP