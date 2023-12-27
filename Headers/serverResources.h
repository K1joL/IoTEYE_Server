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
        REGISTER_USER = 'r' + 'u',
        REGISTER_DEVICE = 'r' + 'd',
        CREATE_PIN = 'c' + 'p',
        DEVICE_STATUS = 'd' + 's',
        UPDATE_PIN = 'u' + 'p',
        DELETE_PIN = 'd' + 'p',
        GET_PIN = 'p' + 'v',
        COMMANDS_MAX = 8
    };
}

static std::unordered_map<std::string, ioteyeUser::User*> userPins;

class pins_resource : public http_resource
{
    public:
        std::shared_ptr<http_response> render_POST(const http_request& req);
        // std::shared_ptr<http_response> render_GET(const http_request &req);
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