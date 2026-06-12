#include <chrono>
#include <common/logging.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <persistence/historyLogger/pinHistoryLogger.hpp>
#include <vector>

namespace ioteye::persistence {

using json = nlohmann::json;
using namespace ioteye::types;
using namespace ioteye::server::debug;

PinHistoryLogger::PinHistoryLogger(PinHistoryConfig config) : m_config(std::move(config)) {
}

int64_t PinHistoryLogger::nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void PinHistoryLogger::record(const ioteye::Device& device) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto id = device.getDeviceID();
    const auto now = nowMs();
    auto& last = m_lastRecordMs[id];
    if (now - last < static_cast<int64_t>(m_config.intervalMs))
        return;
    last = now;
    appendSnapshot(device);
    trimDeviceIfNeeded(id);
}

void PinHistoryLogger::appendSnapshot(const ioteye::Device& device) {
    json pins = json::object();
    for (const auto& [num, val] : device.getIntPins())
        pins[std::to_string(num)] = val;  // store as number, not string
    for (const auto& [num, val] : device.getDoublePins())
        pins[std::to_string(num)] = val;
    for (const auto& [num, val] : device.getStringPins())
        pins[std::to_string(num)] = val;

    json row;
    row["ts"] = nowMs();
    row["device_id"] = device.getDeviceID();
    row["pins"] = std::move(pins);

    std::ofstream out(m_config.filePath, std::ios::app);
    if (!out.is_open()) {
        logln(LogLevel::ERROR, "PinHistoryLogger: cannot open ", m_config.filePath);
        return;
    }
    out << row.dump() << '\n';
}

void PinHistoryLogger::trimDeviceIfNeeded(types::DeviceID deviceId) {
    std::ifstream in(m_config.filePath);
    if (!in.is_open())
        return;

    std::vector<std::string> keep;
    std::vector<std::string> device_lines;
    std::string line;

    // Single pass - separate this device's lines from others
    while (std::getline(in, line)) {
        if (line.empty())
            continue;
        try {
            auto row = json::parse(line);
            if (row.value("device_id", types::DeviceID{0}) == deviceId)
                device_lines.push_back(line);
            else
                keep.push_back(line);
        } catch (...) {
            keep.push_back(line);  // preserve unparseable lines
        }
    }
    in.close();

    // Only rewrite if this device is over limit
    if (device_lines.size() <= m_config.maxSamples)
        return;

    const size_t excess = device_lines.size() - m_config.maxSamples;
    device_lines.erase(device_lines.begin(), device_lines.begin() + excess);

    std::ofstream out(m_config.filePath, std::ios::trunc);
    if (!out.is_open())
        return;
    for (const auto& l : keep)
        out << l << '\n';
    for (const auto& l : device_lines)
        out << l << '\n';
}

std::string PinHistoryLogger::query(types::DeviceID deviceId, int64_t since, int64_t until,
                                    uint32_t limit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    const uint32_t cap = limit > 0 ? limit : 1000;

    json result = json::array();
    std::ifstream in(m_config.filePath);
    if (!in.is_open())
        return result.dump();

    std::vector<json> matched;
    matched.reserve(cap);
    std::string line;

    while (std::getline(in, line)) {
        if (line.empty())
            continue;
        try {
            auto row = json::parse(line);
            if (row.value("device_id", types::DeviceID{0}) != deviceId)
                continue;
            const int64_t ts = row.value("ts", int64_t{0});
            if (ts < since || ts > until)
                continue;
            matched.push_back({{"ts", ts}, {"pins", row.value("pins", json::object())}});
        } catch (...) {
            continue;
        }
    }

    if (matched.size() > cap)
        matched.erase(matched.begin(), matched.begin() + (matched.size() - cap));

    for (auto& s : matched)
        result.push_back(std::move(s));
    return result.dump();
}

}  // namespace ioteye::persistence