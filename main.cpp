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
    user_resource user;
    // For next updates
    // ws.register_resource("/user/{userID}/pins/{pinNumber}/{cmd}", &pins); 
    ws.register_resource("/user/{userID}/pins", &pins); 
    ws.register_resource("/user", &user);
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