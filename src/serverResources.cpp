#include "serverResources.h"

#include "adminOptions.h"

using namespace ioteye::server::debug;

std::shared_ptr<HttpResponse> PinsResource::renderPOST(const HttpRequest &req) {
    log("pins POST\n", req.getArgs());

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string dataType{req.getArg("dataType")};
    std::string token{req.getArg("token")};
    std::string value{req.getArg("defValue")};
    std::string cmd{req.getArg("cmd")};
    std::string content = "PinValue=";

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device)) {
        case 400:
            return std::make_shared<HttpResponse>(400, "Device doesn`t exist!");
        case 401:
            return std::make_shared<HttpResponse>(401, "Auth failure!");
        case 200:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(500, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::CREATE_PIN:

            if ((device->second->addPin(pinNumber, dataType, value)) == 0)
                return std::make_shared<HttpResponse>(201, "Pin created!");
            else
                return std::make_shared<HttpResponse>(400, "Pin already exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> PinsResource::renderGET(const HttpRequest &req) {
    log("PINS GET\n", req.getArgs());

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string token{req.getArg("token")};
    std::string cmd{req.getArg("cmd")};
    std::string value{};

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device)) {
        case 400:
            return std::make_shared<HttpResponse>(400, "Device doesn`t exist!");
        case 401:
            return std::make_shared<HttpResponse>(401, "Auth failure!");
        case 200:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(500, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::GET_PIN:
            if ((value = device->second->getPin(pinNumber)) != "")
                return std::make_shared<HttpResponse>(200, "PinValue=" + value);
            else
                return std::make_shared<HttpResponse>(400, "Pin does not exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> PinsResource::renderPUT(const HttpRequest &req) {
    log("PINS PUT\n", req.getArgs());

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string token{req.getArg("token")};
    std::string value{req.getArg("value")};
    std::string cmd{req.getArg("cmd")};

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device)) {
        case 400:
            return std::make_shared<HttpResponse>(400, "Device doesn`t exist!");
        case 401:
            return std::make_shared<HttpResponse>(401, "Auth failure!");
        case 200:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(500, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::UPDATE_PIN:
            if ((device->second->changePin(pinNumber, value)) == 0)
                return std::make_shared<HttpResponse>(200, "Pin changed");
            else
                return std::make_shared<HttpResponse>(400, "Pin does not exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> PinsResource::renderDELETE(const HttpRequest &req) {
    log("PINS DELETE\n", req.getArgs());

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string token{req.getArg("token")};
    std::string cmd{req.getArg("cmd")};

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device)) {
        case 400:
            return std::make_shared<HttpResponse>(400, "Device doesn`t exist!");
        case 401:
            return std::make_shared<HttpResponse>(401, "Auth failure!");
        case 200:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(500, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DELETE_PIN:
            if ((device->second->removePin(pinNumber)) == 0)
                return std::make_shared<HttpResponse>(200, "Pin deleted");
            else
                return std::make_shared<HttpResponse>(400, "Pin does not exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderPOST(const HttpRequest &req) {
    log("DEVICE POST\n", req.getArgs());

    std::string cmd{req.getArg("cmd")};
    std::string payload{"token="};
    ioteye::Device *newDevice = nullptr;
    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::REGISTER_DEVICE:
            // Create new Device
            newDevice = new ioteye::Device(OPTIONS->getOutdatedDelay(), OPTIONS->getOfflineDelay(),
                                           OPTIONS->getMaxPins());
            payload += newDevice->getToken();
            if (newDevice != nullptr) {
                log(s_idDeviceMap.emplace(newDevice->getID(), newDevice).first->first);
                log(s_idDeviceMap.emplace(newDevice->getID(), newDevice).first->second->getToken());
                return std::make_shared<HttpResponse>(201, payload);
            } else
                return std::make_shared<HttpResponse>(500, "Something went wrong!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderGET(const HttpRequest &req) {
    log("DEVICE GET\n", req.getArgs());

    // Getting request arguments
    std::string cmd{req.getArg("cmd")};
    std::string token{req.getArg("token")};
    uint8_t deviceStatus = 0;

    // existence check
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device)) {
        case 400:
            return std::make_shared<HttpResponse>(400, "Device doesn`t exist!");
        case 401:
            return std::make_shared<HttpResponse>(401, "Auth failure!");
        case 200:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(500, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DEVICE_STATUS:
            deviceStatus = device->second->getState();
            return std::make_shared<HttpResponse>(200, "devStatus=" + std::to_string(deviceStatus));
            break;
        case ioteye::DEVICE_STATUS_UPDATE:
            device->second->ping();
            return std::make_shared<HttpResponse>(200, "Device status updated!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderPUT(const HttpRequest &req) {
    log("DEVICE PUT\n", req.getArgs());

    // Getting request arguments
    std::string cmd{req.getArg("cmd")};
    std::string token{req.getArg("token")};

    // existence check
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device)) {
        case 400:
            return std::make_shared<HttpResponse>(400, "Device doesn`t exist!");
        case 401:
            return std::make_shared<HttpResponse>(401, "Auth failure!");
        case 200:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(500, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DEVICE_STATUS_UPDATE:
            device->second->ping();
            return std::make_shared<HttpResponse>(200, "Device status updated!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderDELETE(const HttpRequest &req) {
    log("DEVICE DELETE\n", req.getArgs());

    // Getting request arguments
    std::string cmd{req.getArg("cmd")};
    std::string token{req.getArg("token")};

    // existence check
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device)) {
        case 400:
            return std::make_shared<HttpResponse>(400, "Device doesn`t exist!");
        case 401:
            return std::make_shared<HttpResponse>(401, "Auth failure!");
        case 200:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(500, "Something went wrong");

    auto temp = device->second;
    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DELETE_DEVICE:
            s_idDeviceMap.erase(device);
            delete temp;
            return std::make_shared<HttpResponse>(200, "Device deleted!");
            break;
        default:
            return std::make_shared<HttpResponse>(400, "Wrong command!");
    }
}

uint16_t authCheck(const std::string &token, DeviceIter *deviceIter) {
    const auto decodedToken = jwt::decode(token);
    uint64_t devID = std::stoul(decodedToken.get_payload_claim("deviceID").as_string());
    auto device = s_idDeviceMap.find(devID);
    if (deviceIter != nullptr)
        *deviceIter = device;
    if (device == s_idDeviceMap.end())
        return 400;
    if (device->second->getToken() != token)
        return 401;
    return 200;
}
