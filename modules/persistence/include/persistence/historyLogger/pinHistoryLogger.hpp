#ifndef IOTEYE_PIN_HISTORY_LOGGER_HPP
#define IOTEYE_PIN_HISTORY_LOGGER_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "ipinHistoryLogger.hpp"

namespace ioteye::persistence {

struct PinHistoryConfig {
    uint32_t intervalMs = 2000;
    uint32_t maxSamples = 5000;  // per device
    std::string filePath = "pin_history.jsonl";
};

class PinHistoryLogger : public IPinHistoryLogger {
public:
    explicit PinHistoryLogger(PinHistoryConfig config = {});

    void record(const ioteye::Device& device) override;
    std::string query(types::DeviceID deviceId, int64_t since, int64_t until,
                      uint32_t limit) const override;

private:
    void appendSnapshot(const ioteye::Device& device);
    void trimDeviceIfNeeded(types::DeviceID deviceId);

    static int64_t nowMs();

    PinHistoryConfig m_config;
    mutable std::mutex m_mutex;
    std::unordered_map<types::DeviceID, int64_t> m_lastRecordMs;
};

}  // namespace ioteye::persistence

#endif