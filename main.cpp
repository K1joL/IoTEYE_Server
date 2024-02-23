#include "serverResources.h"
#include "functional.h"
#include "adminOptions.h"

#include <httpserver.hpp>
#include <iostream>

using namespace httpserver;
using std::cout;
using std::endl;

AdminOptions *OPTIONS = new AdminOptions();

int main(int argc, char **argv)
{
    OPTIONS->init(argc, argv);
    webserver ws = create_webserver(8081);
    pins_resource pins;
    // user_resource user;
    device_resource devices;
    // register resources
    // resources without payload:
    ws.register_resource("/devices/{token}/pins/{pinNumber}/{dataType}/{defValue}/{cmd}", &pins); // CREATE PIN
    ws.register_resource("/devices/{token}/pins/{pinNumber}/{cmd}", &pins); // GET PIN && DELETE PIN
    ws.register_resource("/devices/{token}/pins/{pinNumber}/{value}/{cmd}", &pins); // UPDATE PIN
    ws.register_resource("/devices/{cmd}", &devices); // REGISTER DEVICE
    ws.register_resource("/devices/{token}/{cmd}", &devices); // GET DEVICE STATUS && UPDATE DEVICE && DELETE DEVICE
    // resources with payload: (ONLY for POST and PUT requests)
    ws.register_resource("/devices/pins", &pins); // CREATE PIN && UPDATE PIN
    ws.register_resource("/devices", &devices); // REGISTER DEVICE && UPDATE DEVICE
    ws.start(0);
    char key;
    while(true)
    {
        key = getchar();
        if(key == 27)
        {
            ws.sweet_kill();
            break;
        }
    }
    return 0;
}