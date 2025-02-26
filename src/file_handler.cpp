#include "file_handler.h"

using json = nlohmann::json;

namespace ioteye {

bool FileHandler::saveDevices(const std::unordered_map<uint64_t, DevicePtr>& devices,
                              const std::string& filename) {
    json j;
    for (const auto& pair : devices) {
        json device;
        device["id"] = pair.second->getID();
        device["token"] = pair.second->getToken();
        device["maxPins"] = pair.second->m_maxPins;
        device["pinsCounter"] = pair.second->pinsCreated();
        // Save virtual pins data
        json intPins = json::array();
        for (const auto& pin : pair.second->m_intPins)
            intPins.push_back({pin.first, pin.second});
        device["intPins"] = intPins;

        json doublePins = json::array();
        for (const auto& pin : pair.second->m_doublePins) {
            doublePins.push_back({pin.first, pin.second});
        }
        device["doublePins"] = doublePins;

        json stringPins = json::array();
        for (const auto& pin : pair.second->m_stringPins) {
            stringPins.push_back({pin.first, pin.second});
        }
        device["stringPins"] = stringPins;
        j.push_back(device);
    }
    std::ofstream devicesFileOutput(filename);
    devicesFileOutput << j.dump(4);
    return true;
}

bool FileHandler::loadDevices(std::unordered_map<uint64_t, DevicePtr>& devices, const std::string& filename) {
    std::ifstream devicesFileInput(filename);
    if (!devicesFileInput.is_open()) {
        return false;
    }
    if (devicesFileInput.peek() == std::ifstream::traits_type::eof()) {
        return false;
    }
    json j;
    devicesFileInput >> j;
    int count = 0;
    for (const auto& device : j) {
        std::unordered_map<uint16_t, int> intPins;
        std::unordered_map<uint16_t, double> doublePins;
        std::unordered_map<uint16_t, std::string> stringPins;
        std::unordered_map<uint16_t, uint8_t> pinsType;

        for (const auto& pin : device["intPins"]) {
            pinsType[pin[0]] = Device::ContainerID::INTID;
            intPins[pin[0]] = pin[1];
        }
        for (const auto& pin : device["doublePins"]) {
            pinsType[pin[0]] = Device::ContainerID::DOUBLEID;
            doublePins[pin[0]] = pin[1];
        }
        for (const auto& pin : device["stringPins"]) {
            pinsType[pin[0]] = Device::ContainerID::STRINGID;
            stringPins[pin[0]] = pin[1];
        }

        DevicePtr newDevice = std::make_shared<Device>(Device::Builder()
                                                           .setID(device["id"])
                                                           .setToken(device["token"])
                                                           .setMaxPins(device["maxPins"])
                                                           .setIntPinMap(std::move(intPins))
                                                           .setDoublePinMap(std::move(doublePins))
                                                           .setStringPinMap(std::move(stringPins))
                                                           .setPinsTypeMap(std::move(pinsType))
                                                           .build());
        // Initialize other relevant data if necessary
        auto emplaceIt = devices.emplace(newDevice->getID(), newDevice);
        // server::debug::log(newDevice->getPinsTypes().size());
        // server::debug::log(newDevice->getIntPins().size());
        // server::debug::log(newDevice->getStringPins().size());
        // server::debug::log(newDevice->getDoublePins().size());
        ++count;
        ioteye::server::debug::log("Device loading ", count, '/', j.size(),
                                   emplaceIt.second ? " Success" : " Failed");
    }
    return true;
}
}  // namespace ioteye