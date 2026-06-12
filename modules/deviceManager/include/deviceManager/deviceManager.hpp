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

#ifndef IOTEYE_DEVICE_MANAGER_HPP
#define IOTEYE_DEVICE_MANAGER_HPP

#include <common/types.hpp>
#include <device/device.hpp>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utils/processorPool.hpp>

namespace ioteye {

/// @brief Manages devices and their time processors. Maintains a map of
///        devices keyed by their IDs.
/// @note All public methods are thread-safe.
class DeviceManager {
public:
    /// @brief Constructs a DeviceManager object.
    /// @param pool Pointer to the Processor Pool used for managing device
    /// tasks. Must not be a nullptr.
    DeviceManager(std::shared_ptr<utils::ProcessorPool> pool);

    /// @brief Creates a new device and registers it in the processor pool.
    /// @param params Parameters used to initialize the device.
    /// @return A pointer to the newly created device, or nullptr if creation
    ///         fails or a device with the same ID already exists.
    types::DevicePtr createDevice(const types::DeviceParams& params);

    /// @brief Creates a new device and registers it in the processor pool.
    /// @param newDevice A device pointer used to initialize the device.
    /// @return A pointer to the newly created device, or nullptr if creation
    ///         fails or a device with the same ID already exists.
    types::DevicePtr createDevice(types::DevicePtr newDevice);

    /// @brief Deletes a device from the manager and the processor pool.
    /// @param id DeviceID of the device to be deleted.
    /// @return True if the device was successfully deleted or if it was already
    ///         deleted (not found). False if removal from the pool fails.
    /// @note Please note that ObjID and DeviceID may vary. So it's better to
    /// use getDeviceID().
    bool deleteDevice(types::DeviceID id);

    /// @brief Deletes a device from the manager and the processor pool.
    /// @param device Pointer of the device to be deleted.
    /// @return True if the device was successfully deleted or if it was already
    ///         deleted (not found). False if removal from the pool fails.
    bool deleteDevice(types::DevicePtr device);

    /// @brief Finds a device by its unique ID.
    /// @param id The ID of the device to find.
    /// @return A pointer to the device if found, or nullptr otherwise.
    types::DevicePtr findById(types::DeviceID id) const;

    /// @brief Finds a device by its JWT token.
    /// @param token The JWT token containing the device ID in its payload.
    /// @return A pointer to the device if the token is valid and the device is
    ///         found, or nullptr if decoding fails, the token is invalid, or
    ///         the device does not exist.
    types::DevicePtr findByToken(const types::Token& token) const;

    /// @brief Gets the total number of devices currently managed.
    /// @return The number of devices in the manager.
    size_t getSize() const;

    /// @brief
    /// @return
    const types::DeviceMap& getDevices() const;

private:
    /// @brief Callback invoked when a device's state changes.
    /// @param id The ID of the device whose state changed.
    /// @param state The new state of the device.
    /// @note If the new state is DEAD, the device is automatically removed
    ///       from the processor pool and deleted from the manager.
    void onDeviceStateChange(types::DeviceID id, types::DeviceState state);

    /// @brief Extracts the device ID from a JWT token's payload.
    /// @param token The JWT token to be parsed.
    /// @return The extracted device ID, or NullDeviceID if the "deviceID"
    ///         claim is not present or if the token cannot be decoded.
    types::DeviceID getDeviceIdByToken(types::Token token) const;

private:
    /// @brief Pointer to the processor pool.
    std::shared_ptr<utils::ProcessorPool> m_pool;

    /// @brief Mutex used to synchronize access to the device map.
    mutable std::mutex m_mutex;

    /// @brief Map of device pointers, keyed by DeviceID.
    types::DeviceMap m_devices;
};

}  // namespace ioteye

#endif  // IOTEYE_DEVICE_MANAGER_HPP
