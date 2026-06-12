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

#include <common/logging.hpp>
#include <common/types.hpp>
#include <deviceManager/deviceManager.hpp>

namespace ioteye {

using namespace server::debug;
using namespace types;

DeviceManager::DeviceManager(std::shared_ptr<utils::ProcessorPool> pool) : m_pool(pool) {
}

DevicePtr DeviceManager::createDevice(const DeviceParams& params) {
    auto dev = std::make_shared<Device>(params.outdatedDelay, params.offlineDelay, params.deadDelay,
                                        params.maxPins, params.deleteAfterDeath);

    return createDevice(dev);
}

types::DevicePtr DeviceManager::createDevice(types::DevicePtr newDevice) {
    if (!newDevice) {
        logStatus("[DeviceManager] Failed to create device");
        return nullptr;
    }
    DeviceID id = newDevice->getDeviceID();
    // Set DEAD callback
    newDevice->setOnStateChange(
        [this, id](DeviceState state) { this->onDeviceStateChange(id, state); });

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto [it, inserted] = m_devices.emplace(id, newDevice);
        if (!inserted) {
            logStatus("[DeviceManager] Device insert failed, id already exists: ", id);
            return nullptr;
        }
    }

    m_pool->registerObject(newDevice);
    return newDevice;
}

bool DeviceManager::deleteDevice(DeviceID id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_devices.find(id);
    // Assume already deleted
    if (it == m_devices.end()) {
        logStatus("[DeviceManager] Device was not found to delete!");
        return false;
    }

    auto foundDevice = it->second;
    // Any result successful for us
    if (!m_pool->removeObject(foundDevice->getObjID()))
        logStatus("[DeviceManager] Device already deleted from pool!");
    if (m_devices.erase(foundDevice->getDeviceID()))
        logStatus("[DeviceManager] Device already deleted from map!");

    return true;
}

bool DeviceManager::deleteDevice(types::DevicePtr device) {
    if (!device)
        return false;
    return deleteDevice(device->getDeviceID());
}

DevicePtr DeviceManager::findById(DeviceID id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_devices.find(id);
    return it != m_devices.end() ? it->second : nullptr;
}

DevicePtr DeviceManager::findByToken(const Token& token) const {
    DevicePtr result;
    try {
        uint64_t devID = getDeviceIdByToken(token);
        if (devID == NullDeviceID)
            return nullptr;

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_devices.find(devID);
        if (it == m_devices.end())
            return nullptr;
        if (it->second->getToken() != token)
            return nullptr;
        result = it->second;
    } catch (const std::exception& e) {
        logStatus("[DeviceManager] JWT decode failure: ", e.what());
    }
    return result;
}

size_t DeviceManager::getSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices.size();
}

void DeviceManager::onDeviceStateChange(DeviceID id, DeviceState state) {
    logStatus("[DeviceManager] DeviceStatus #", id, " is ", toString(state));
    if (state != DeviceState::DEAD)
        return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_devices.find(id);
    if (it == m_devices.end())
        return;
    if (it->second->isDeleteAfterDeath()) {
        m_pool->removeObject(it->second->getObjID());
        m_devices.erase(it);
    }
}

DeviceID DeviceManager::getDeviceIdByToken(Token token) const {
    try {
        const auto decodedToken = jwt::decode(token);
        uint64_t devID = std::stoul(decodedToken.get_payload_claim("deviceID").as_string());
        return devID;
    } catch (const std::exception& e) {
        logStatus("[DeviceManager] JWT exception thrown: ", e.what());
        return NullDeviceID;
    }
}

const types::DeviceMap& DeviceManager::getDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices;
}

}  // namespace ioteye