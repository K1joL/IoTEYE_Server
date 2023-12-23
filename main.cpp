#include "functional.h"
#include "user.h"
#include "serverResources.h"

#include <httpserver.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>


using namespace httpserver;
using std::cout;
using std::endl;



int main(int argc, char **argv)
{
    webserver ws = create_webserver(8081);
    pins_resource pins;
    ws.register_resource("/IoTEYE_PINS", &pins);
    ws.start(0);
    char key;
    while(true)
    {
        key = getchar();
        if(key = 27)
        {
            ws.sweet_kill();
            break;
        }
    }
    return 0;
}