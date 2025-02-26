#include <iostream>
#include <ioteyeserver.hpp>

#include "adminOptions.h"
#include "file_handler.h"
#include "functional.h"
#include "serverResources.h"

using std::cout;
using std::endl;

std::unique_ptr<AdminOptions> OPTIONS = std::make_unique<AdminOptions>();

int main(int argc, char** argv) {
    OPTIONS->init(argc, argv);
    // Load devices from file
    if (!ioteye::FileHandler::loadDevices(s_idDeviceMap, "devices.json"))
        std::cout << "Failed to load devices from file." << std::endl;

    auto pins = std::make_shared<PinsResource>();
    auto devices = std::make_shared<DeviceResource>();
    ioteye::Webserver ws =
        ioteye::Webserver::Builder()
            .setTcpPort(8080)
            .setUdpOn()
            .setUdpPort(8081)
            .setResource("/devices/{token}/pins/{pinNumber}/{dataType}/{defValue}/{cmd}", pins)  // CREATE PIN
            .setResource("/devices/{token}/pins/{pinNumber}/{cmd}", pins)          // GET PIN && DELETE PIN
            .setResource("/devices/{token}/pins/{pinNumber}/{value}/{cmd}", pins)  // UPDATE PIN
            .setResource("/devices/{cmd}", devices)                                // REGISTER DEVICE
            .setResource("/devices/{token}/{cmd}",
                         devices)  // GET DEVICE STATUS && UPDATE DEVICE && DELETE DEVICE
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
    if (!ioteye::FileHandler::saveDevices(s_idDeviceMap, "devices.json")) {
        std::cout << "Failed to save devices to file." << std::endl;
    }
    return 0;
}