#ifndef SERVER_RESOURCES_H
#define SERVER_RESOURCES_H

/**
 * TODO:
 * Добавить функции администрирования сервера
 * Сделать авторизацию
 * 
*/

#include <httpserver.hpp>
#include <unordered_map>

#ifdef IoTeyeDEBUG
#include <iostream>
#endif // !IoTeyeDEBUG

#include "functional.h"
#include "device.h"

using namespace httpserver;

using std::cout;
using std::endl;

namespace ioteyeServer
{
    enum COMMANDS
    {
        NON_COMMAND = 0,
        REGISTER_DEVICE = 'r' + 'd',            //214
        DELETE_DEVICE = 'd' + 'd',              //200
        DEVICE_STATUS = 'd' + 's',              //215
        DEVICE_STATUS_UPDATE = 'u' + 's',       //232
        CREATE_PIN = 'c' + 'p',                 //211
        UPDATE_PIN = 'u' + 'p',                 //229
        DELETE_PIN = 'd' + 'p',                 //212
        GET_PIN = 'p' + 'v',                    //230
        COMMANDS_MAX = 9
    };
}

static std::unordered_map<uint64_t, ioteyeDevice::Device*> s_idDeviceMap;

using DeviceIter = std::unordered_map<uint64_t, ioteyeDevice::Device *>::iterator;
uint16_t authCheck(const std::string &token, DeviceIter* deviceIter = nullptr);

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
        std::shared_ptr<http_response> render_PUT(const http_request& req);
        std::shared_ptr<http_response> render_DELETE(const http_request& req);
};


#endif //SERVER_RESOURCES_H