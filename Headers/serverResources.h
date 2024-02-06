#ifndef SERVER_RESOURCES_H
#define SERVER_RESOURCES_H

#include <httpserver.hpp>
#include <iostream>
#include <unordered_map>

#include "functional.h"
#include "user.h"

using namespace httpserver;

using std::cout;
using std::endl;

namespace ioteyeServer
{
    enum COMMANDS
    {
        NON_COMMAND = 0,
        REGISTER_USER = 'r' + 'u',              //231
        REGISTER_DEVICE = 'r' + 'd',            //214
        CREATE_PIN = 'c' + 'p',                 //211
        DEVICE_STATUS = 'd' + 's',              //215
        DEVICE_STATUS_UPDATE = 'u' + 's',       //232
        UPDATE_PIN = 'u' + 'p',                 //229
        DELETE_PIN = 'd' + 'p',                 //212
        GET_PIN = 'p' + 'v',                    //230
        COMMANDS_MAX = 9
    };
}

static std::unordered_map<std::string, ioteyeUser::User*> s_userPins;
static std::unordered_map<uint64_t, ioteyeUser::User*> s_userDevices;

class pins_resource : public http_resource
{
    public:
        std::shared_ptr<http_response> render_POST(const http_request& req);
        std::shared_ptr<http_response> render_GET(const http_request &req);
        std::shared_ptr<http_response> render_PUT(const http_request& req);
        std::shared_ptr<http_response> render_DELETE(const http_request& req);
};

class device_resource : public http_resource
{
    public:
        std::shared_ptr<http_response> render_POST(const http_request& req);
        std::shared_ptr<http_response> render_GET(const http_request& req);
};

class user_resource : public http_resource
{
    public:
        std::shared_ptr<http_response> render_POST(const http_request& req);
        std::shared_ptr<http_response> render_GET(const http_request& req);
};

#endif //SERVER_RESOURCES_H