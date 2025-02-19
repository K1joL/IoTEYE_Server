#include <iostream>
#include <ioteyeserver.hpp>

#include "adminOptions.h"
#include "functional.h"
#include "serverResources.h"

using std::cout;
using std::endl;

AdminOptions *OPTIONS = new AdminOptions();

int main(int argc, char **argv) {
    OPTIONS->init(argc, argv);
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
                         devices)                // GET DEVICE STATUS && UPDATE DEVICE && DELETE DEVICE
            .build();
    ws.start();
    char key;
    while (true) {
        key = getchar();
        if (key == 27) {
            ws.shutdown();
            break;
        }
    }
    return 0;
}