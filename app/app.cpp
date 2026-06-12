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

#include <common/adminOptions.hpp>
#include <common/logging.hpp>
#include <common/types.hpp>
#include <deviceManager/deviceManager.hpp>
#include <iostream>
#include <ioteyeserver.hpp>
#include <persistence/fileManager.hpp>
#include <persistence/historyLogger/pinHistoryLogger.hpp>
#include <server/serverResources.hpp>
#include <utils/processorPool.hpp>
#include <vector>

using std::cout;
using std::endl;

int main(int argc, char** argv) {
    try {
        ioteye::AdminOptions::getOptions(argc, argv);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    // Load devices from file
    std::shared_ptr<ioteye::FileHandler> devicesJsonHandler;
    std::shared_ptr<ioteye::DeviceFileManager> deviceJsonManager;
    std::shared_ptr<ioteye::DeviceManager> deviceManager;
    std::shared_ptr<ioteye::utils::ProcessorPool> processorPool;
    try {
        processorPool = std::make_shared<ioteye::utils::ProcessorPool>();
        deviceManager = std::make_shared<ioteye::DeviceManager>(processorPool);
        devicesJsonHandler =
            std::make_shared<ioteye::FileHandler>("devices.json", std::ios::in | std::ios::out);
        deviceJsonManager = std::make_shared<ioteye::DeviceFileManager>(devicesJsonHandler);
        auto devicesToLoad = deviceJsonManager->loadFile();
        if (devicesToLoad.empty())
            std::cout << "Failed to load devices from file." << std::endl;
        for (auto& device : devicesToLoad) {
            deviceManager->createDevice(std::move(device));
        }
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << "\nDevice information will not be loaded!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    const auto& opts = ioteye::AdminOptions::getOptions();
    ioteye::persistence::PinHistoryConfig cfg;
    cfg.intervalMs = opts.getHistoryIntervalMs();
    cfg.maxSamples = opts.getHistoryMaxSamples();
    cfg.filePath = opts.getHistoryFile();
    auto pinHistoryLogger = std::make_shared<ioteye::persistence::PinHistoryLogger>(cfg);

    auto pins = std::make_shared<ioteye::resource::PinsResource>(deviceManager, pinHistoryLogger);
    auto devices =
        std::make_shared<ioteye::resource::DeviceResource>(deviceManager, pinHistoryLogger);
    auto history =
        std::make_shared<ioteye::resource::HistoryResource>(deviceManager, pinHistoryLogger);
    ioteye::Webserver ws =
        ioteye::Webserver::Builder()
            .setTcpPort(8080)
            .setUdpOn()
            .setUdpPort(8081)
            // CREATE PIN
            .setResource("/devices/{token}/pins/{cmd}/{pinNumber}/{dataType}/{value}", pins, "cp")
            // GET PIN or DELETE PIN
            .setResource("/devices/{token}/pins/{cmd}/{pinNumber}", pins, "pv,dp")
            // UPDATE PIN
            .setResource("/devices/{token}/pins/{cmd}/{pinNumber}/{value}", pins, "up")
            // REGISTER DEVICE
            .setResource("/devices/{cmd}", devices, "rd")
            // GET DEVICE STATUS or UPDATE DEVICE STATUS or DELETE DEVICE
            .setResource("/devices/{token}/{cmd}", devices, "ds,us,dd")
            // PIN HISTORY (since/until unix sec, limit)
            .setResource("/devices/{token}/history/{since}/{until}/{limit}", history, "gh")
            .build();
    ws.start();
    char key;
    while (true) {
        key = getchar();
        // ESC
        if (key == 27) {
            ws.shutdown();
            break;
        }
        if (key == 'm') {
            std::cout << deviceManager->getSize() << std::endl;
        }
    }
    // Save devices to file
    try {
        if (devicesJsonHandler == nullptr)
            devicesJsonHandler =
                std::make_shared<ioteye::FileHandler>("devices.json", std::ios::in | std::ios::out);
        if (deviceJsonManager == nullptr)
            deviceJsonManager = std::make_shared<ioteye::DeviceFileManager>(devicesJsonHandler);
        if (!deviceJsonManager->saveFile(deviceManager->getDevices()))
            std::cout << "Failed to save device information." << std::endl;
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << "\nDevice information will not be saved!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}