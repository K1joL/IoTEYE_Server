#include "serverResources.h"
#include "adminOptions.h"

using namespace iotDebug;

std::shared_ptr<http_response> pins_resource::render_POST(const http_request &req)
{
    debugMessage("pins POST");
    NEWLINE
    PAYLOAD_DEBUG(req.get_args())

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.get_arg("pinNumber"));
    std::string dataType{req.get_arg("dataType")};
    std::string token{req.get_arg("token")};
    std::string value{req.get_arg("defValue")};
    std::string cmd{req.get_arg("cmd")};
    std::string content = "PinValue=";

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device))
    {
    case 400:
        return std::shared_ptr<http_response>(new string_response("Device doesn`t exist!", 400));
    case 401:
        return std::shared_ptr<http_response>(new string_response("Auth failure!", 401));
    case 200:
        break;
    default:
        break;
    }
    if(device == s_idDeviceMap.end())
        return std::shared_ptr<http_response>(new string_response("Something went wrong", 500));

    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::CREATE_PIN:

        if ((device->second->addPin(pinNumber, dataType, value)) == 0)
            return std::shared_ptr<http_response>(new string_response("Pin created!", 201));
        else
            return std::shared_ptr<http_response>(new string_response("Pin already exists!", 400));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> pins_resource::render_GET(const http_request &req)
{
    debugMessage("PINS GET");
    NEWLINE
    PAYLOAD_DEBUG(req.get_args())

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.get_arg("pinNumber"));
    std::string token{req.get_arg("token")};
    std::string cmd{req.get_arg("cmd")};
    std::string value{};

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device))
    {
    case 400:
        return std::shared_ptr<http_response>(new string_response("Device doesn`t exist!", 400));
    case 401:
        return std::shared_ptr<http_response>(new string_response("Auth failure!", 401));
    case 200:
        break;
    default:
        break;
    }
    if(device == s_idDeviceMap.end())
        return std::shared_ptr<http_response>(new string_response("Something went wrong", 500));

    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::GET_PIN:
        if ((value = device->second->getPin(pinNumber)) != "")
            return std::shared_ptr<http_response>(new string_response("PinValue=" + value, 200));
        else
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> pins_resource::render_PUT(const http_request &req)
{
    debugMessage("pins PUT");
    NEWLINE
    PAYLOAD_DEBUG(req.get_args())

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.get_arg("pinNumber"));
    std::string token{req.get_arg("token")};
    std::string value{req.get_arg("value")};
    std::string cmd{req.get_arg("cmd")};

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device))
    {
    case 400:
        return std::shared_ptr<http_response>(new string_response("Device doesn`t exist!", 400));
    case 401:
        return std::shared_ptr<http_response>(new string_response("Auth failure!", 401));
    case 200:
        break;
    default:
        break;
    }
    if(device == s_idDeviceMap.end())
        return std::shared_ptr<http_response>(new string_response("Something went wrong", 500));

    switch (func::GetCommandCode(cmd))
    {

    case ioteyeServer::UPDATE_PIN:
        if ((device->second->changePin(pinNumber, value)) == 0)
            return std::shared_ptr<http_response>(new string_response("Pin changed", 200));
        else
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> pins_resource::render_DELETE(const http_request &req)
{
    debugMessage("pins DELETE");
    NEWLINE

    // Getting request arguments
    uint8_t pinNumber = std::stoi(req.get_arg("pinNumber"));
    std::string token{req.get_arg("token")};
    std::string cmd{req.get_arg("cmd")};

    // existence checks
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device))
    {
    case 400:
        return std::shared_ptr<http_response>(new string_response("Device doesn`t exist!", 400));
    case 401:
        return std::shared_ptr<http_response>(new string_response("Auth failure!", 401));
    case 200:
        break;
    default:
        break;
    }
    if(device == s_idDeviceMap.end())
        return std::shared_ptr<http_response>(new string_response("Something went wrong", 500));

    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::DELETE_PIN:
        if ((device->second->removePin(pinNumber)) == 0)
            return std::shared_ptr<http_response>(new string_response("Pin deleted", 200));
        else
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> device_resource::render_POST(const http_request &req)
{
    debugMessage("Device POST");
    NEWLINE

    std::string cmd{req.get_arg("cmd")};
    std::string payload{"token="};
    ioteyeDevice::Device *newDevice = nullptr;
    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::REGISTER_DEVICE:
        // Create new Device
        newDevice = new ioteyeDevice::Device(OPTIONS->getOutdatedDelay(),
                                             OPTIONS->getOfflineDelay(),
                                             OPTIONS->getMaxPins());
        payload += newDevice->getToken();
        if (newDevice != nullptr)
        {
            debugMessageln(s_idDeviceMap.emplace(newDevice->getID(), newDevice).first->first);
            debugMessageln(s_idDeviceMap.emplace(newDevice->getID(), newDevice).first->second->getToken());
            return std::shared_ptr<http_response>(new string_response(payload, 201));
        }
        else
            return std::shared_ptr<http_response>(new string_response("Something went wrong!", 500));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> device_resource::render_GET(const http_request &req)
{
    debugMessage("Device GET");
    NEWLINE
    PAYLOAD_DEBUG(req.get_args())

    // Getting request arguments
    std::string cmd{req.get_arg("cmd")};
    std::string token{req.get_arg("token")};
    uint8_t deviceStatus = 0;

    // existence check
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device))
    {
    case 400:
        return std::shared_ptr<http_response>(new string_response("Device doesn`t exist!", 400));
    case 401:
        return std::shared_ptr<http_response>(new string_response("Auth failure!", 401));
    case 200:
        break;
    default:
        break;
    }
    if(device == s_idDeviceMap.end())
        return std::shared_ptr<http_response>(new string_response("Something went wrong", 500));

    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::DEVICE_STATUS:
        deviceStatus = device->second->getState();
        return std::shared_ptr<http_response>(new string_response("devStatus=" + std::to_string(deviceStatus), 200));
        break;
    case ioteyeServer::DEVICE_STATUS_UPDATE:
        device->second->ping();
        return std::shared_ptr<http_response>(new string_response("Device status updated!", 200));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> device_resource::render_PUT(const http_request &req)
{
    debugMessage("Device GET");
    NEWLINE
    PAYLOAD_DEBUG(req.get_args())

    // Getting request arguments
    std::string cmd{req.get_arg("cmd")};
    std::string token{req.get_arg("token")};

    // existence check
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device))
    {
    case 400:
        return std::shared_ptr<http_response>(new string_response("Device doesn`t exist!", 400));
    case 401:
        return std::shared_ptr<http_response>(new string_response("Auth failure!", 401));
    case 200:
        break;
    default:
        break;
    }
    if(device == s_idDeviceMap.end())
        return std::shared_ptr<http_response>(new string_response("Something went wrong", 500));

    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::DEVICE_STATUS_UPDATE:
        device->second->ping();
        return std::shared_ptr<http_response>(new string_response("Device status updated!", 200));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> device_resource::render_DELETE(const http_request &req)
{
    debugMessage("Device DELETE");
    NEWLINE
    PAYLOAD_DEBUG(req.get_args())

    // Getting request arguments
    std::string cmd{req.get_arg("cmd")};
    std::string token{req.get_arg("token")};

    // existence check
    DeviceIter device = s_idDeviceMap.end();
    switch (authCheck(token, &device))
    {
    case 400:
        return std::shared_ptr<http_response>(new string_response("Device doesn`t exist!", 400));
    case 401:
        return std::shared_ptr<http_response>(new string_response("Auth failure!", 401));
    case 200:
        break;
    default:
        break;
    }
    if(device == s_idDeviceMap.end())
        return std::shared_ptr<http_response>(new string_response("Something went wrong", 500));

    auto temp = device->second;
    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::DELETE_DEVICE:
        s_idDeviceMap.erase(device);
        delete temp;
        return std::shared_ptr<http_response>(new string_response("Device deleted!", 200));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

uint16_t authCheck(const std::string &token, DeviceIter* deviceIter)
{
    const auto decodedToken = jwt::decode(token);
    uint64_t devID = std::stoul(decodedToken.get_payload_claim("deviceID").as_string());
    auto device = s_idDeviceMap.find(devID);
    if(deviceIter != nullptr)
        *deviceIter = device;
    if (device == s_idDeviceMap.end())
        return 400;
    if (device->second->getToken() != token)
        return 401;
    return 200;
}
