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

#include <iostream>
#include <ioteyeserver.hpp>

#include "adminOptions.h"
#include "file_manager.h"
#include "functional.h"
#include "serverResources.h"

using std::cout;
using std::endl;

std::unique_ptr<AdminOptions> OPTIONS = std::make_unique<AdminOptions>();

int main(int argc, char** argv) {
    OPTIONS->init(argc, argv);
    // Load devices from file
    std::shared_ptr<ioteye::FileHandler> devicesJsonHandler;
    std::shared_ptr<ioteye::DeviceFileManager> deviceJsonManager;
    try {
        devicesJsonHandler = std::make_shared<ioteye::FileHandler>(
            "devices.json", std::ios::in | std::ios::out);
        deviceJsonManager = std::make_shared<ioteye::DeviceFileManager>(
            devicesJsonHandler, s_idDeviceMap);
        if (!deviceJsonManager->loadFile())
            std::cout << "Failed to load devices from file." << std::endl;
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << "\nDevice information will not be loaded!"
                  << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    auto pins = std::make_shared<PinsResource>();
    auto devices = std::make_shared<DeviceResource>();
    ioteye::Webserver ws =
        ioteye::Webserver::Builder()
            .setTcpPort(8080)
            .setUdpOn()
            .setUdpPort(8081)
            // CREATE PIN
            .setResource(
                "/devices/{token}/pins/{pinNumber}/{dataType}/{defValue}/{cmd}",
                pins)
            // GET PIN or DELETE PIN
            .setResource("/devices/{token}/pins/{pinNumber}/{cmd}", pins)
            // UPDATE PIN
            .setResource("/devices/{token}/pins/{pinNumber}/{value}/{cmd}",
                         pins)
            // REGISTER DEVICE
            .setResource("/devices/{cmd}", devices)
            // GET DEVICE STATUS or UPDATE DEVICE STATUS or DELETE DEVICE
            .setResource("/devices/{token}/{cmd}", devices)
            .build();
    ws.start();
    char key;
    while (true) {
        key = getchar();
        if (key == 27) {
            ws.shutdown();
            break;
        }
        if (key == 'm') {
            std::cout << s_idDeviceMap.size() << std::endl;
        }
    }
    // Save devices to file
    try {
        if (devicesJsonHandler == nullptr)
            devicesJsonHandler = std::make_shared<ioteye::FileHandler>(
                "devices.json", std::ios::in | std::ios::out);
        if (deviceJsonManager == nullptr)
            deviceJsonManager = std::make_shared<ioteye::DeviceFileManager>(
                devicesJsonHandler, s_idDeviceMap);
        if (!deviceJsonManager->saveFile())
            std::cout << "Failed to save device information." << std::endl;
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << "\nDevice information will not be saved!"
                  << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}