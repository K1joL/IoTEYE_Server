#ifndef SERVER_RESOURCES_H
#define SERVER_RESOURCES_H

#include <ioteyeserver.hpp>
#include <memory>
#include <unordered_map>

#include "device.h"
#include "functional.h"

using namespace ioteye;

using std::cout;
using std::endl;

namespace ioteye {
enum COMMANDS {
    NON_COMMAND = 0,
    REGISTER_DEVICE = 'r' + 'd',       // 214
    DELETE_DEVICE = 'd' + 'd',         // 200
    DEVICE_STATUS = 'd' + 's',         // 215
    DEVICE_STATUS_UPDATE = 'u' + 's',  // 232
    CREATE_PIN = 'c' + 'p',            // 211
    UPDATE_PIN = 'u' + 'p',            // 229
    DELETE_PIN = 'd' + 'p',            // 212
    GET_PIN = 'p' + 'v',               // 230
    COMMANDS_MAX = 9
};
}

using DevicePtr = std::shared_ptr<ioteye::Device>;
static std::unordered_map<uint64_t, DevicePtr> s_idDeviceMap;

using DeviceIter = std::unordered_map<uint64_t, DevicePtr>::iterator;
uint16_t authCheck(const std::string& token, DeviceIter& deviceIter);

class PinsResource : public HttpResourceHandler {
public:
    std::shared_ptr<HttpResponse> renderPOST(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderGET(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderPUT(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderDELETE(const HttpRequest& req) override;
};

class DeviceResource : public HttpResourceHandler {
public:
    std::shared_ptr<HttpResponse> renderPOST(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderGET(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderPUT(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderDELETE(const HttpRequest& req) override;
};

#endif  // SERVER_RESOURCES_H