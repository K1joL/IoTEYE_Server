#include "serverResources.h"

using namespace iotDebug;

std::shared_ptr<http_response> pins_resource::render_POST(const http_request &req)
{
    debugMessage("pins POST"); NEWLINE
    PAYLOAD_DEBUG(req.get_args())
    uint8_t pinNumber = 0;
    std::string dataType{};
    std::string userID{};
    std::string value{};
    std::string cmd{};
    std::string content = "PinValue=";
    auto user = s_userPins.begin();

    // command check
    cmd = req.get_arg("cmd");
    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::CREATE_PIN:
        // Getting request arguments
        userID = req.get_arg("userID");
        pinNumber = std::stoi(req.get_arg("pinNumber"));

        dataType = req.get_arg("dataType");
        value = req.get_arg("value");

        // Make new user
        // if(userPins.find(userID) == userPins.end())
        //     userPins.emplace(userID, new ioteyeUser::User{userID});

        // existence checks
        user = s_userPins.find(userID);
        if (user != s_userPins.end())
            if ((user->second->addPin(pinNumber, dataType, value)) == 0)
                return std::shared_ptr<http_response>(new string_response("Pin created!", 201));
            else
                return std::shared_ptr<http_response>(new string_response("Pin already exists!", 400));
        else
            return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));
        break;

    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}

std::shared_ptr<http_response> pins_resource::render_GET(const http_request &req)
{
    debugMessage("PINS GET"); NEWLINE
    PAYLOAD_DEBUG(req.get_args())
    uint8_t pinNumber = 0;
    std::string userID{};
    std::string value{};
    std::string cmd{};
    std::string content = "PinValue=";
    auto user = s_userPins.begin();

    // command check
    cmd = req.get_arg("cmd");
    if (func::GetCommandCode(cmd) != ioteyeServer::GET_PIN)
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));

    // Getting request arguments
    userID = req.get_arg("userID");
    pinNumber = std::stoi(req.get_arg("pinNumber"));

    // existence checks
    user = s_userPins.find(userID);
    if (user != s_userPins.end())
        if ((value = user->second->getPin(pinNumber)) != "")
        {
            content += value;
            return std::shared_ptr<http_response>(new string_response(content, 200));
        }
        else
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
    else
        return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));

    return std::shared_ptr<http_response>();
}

std::shared_ptr<http_response> pins_resource::render_PUT(const http_request &req)
{
    debugMessage("pins PUT"); NEWLINE
    PAYLOAD_DEBUG(req.get_args())
    uint8_t pinNumber = 0;
    std::string userID{};
    std::string value{};
    std::string cmd{};

    cmd = req.get_arg("cmd");
    if (func::GetCommandCode(cmd) != ioteyeServer::UPDATE_PIN)
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    // Getting request arguments
    userID = req.get_arg("userID");
    pinNumber = std::stoi(req.get_arg("pinNumber"));
    value = req.get_arg("value");

    // existence checks
    auto user = s_userPins.find(userID);
    if (user != s_userPins.end())
        if ((user->second->changePin(pinNumber, value)) == 0)
            return std::shared_ptr<http_response>(new string_response("Pin changed", 200));
        else
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
    else
        return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));
}

std::shared_ptr<http_response> pins_resource::render_DELETE(const http_request &req)
{
    debugMessage("pins DELETE"); NEWLINE
    uint8_t pinNumber = 0;
    std::string userID{};
    std::string cmd{};

    cmd = req.get_arg("cmd");
    if (func::GetCommandCode(cmd) != ioteyeServer::DELETE_PIN)
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    // Getting request arguments
    userID = req.get_arg("userID");
    pinNumber = std::stoi(req.get_arg("pinNumber"));

    // existence checks
    auto user = s_userPins.find(userID);
    if (user != s_userPins.end())
        if ((user->second->removePin(pinNumber)) == 0)
            return std::shared_ptr<http_response>(new string_response("Pin deleted", 200));
        else
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
    else
        return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));
}

std::shared_ptr<http_response> user_resource::render_POST(const http_request &req)
{
    debugMessage("user POST"); NEWLINE
    std::string userID = req.get_arg("customID");
    debugMessage("userid " + userID); NEWLINE

    if (userID != "")
        if (s_userPins.find(userID) == s_userPins.end())
            s_userPins.emplace(userID, new ioteyeUser::User{userID});
        else
            return std::shared_ptr<http_response>(new string_response("This ID already exists!", 400));
    else
    {
        // find unoccupied ID
        uint64_t newId = 0;
        do
        {
            newId = ioteyeUser::User::getNextID();

        } while (s_userPins.find(std::to_string(newId)) != s_userPins.end());

        s_userPins.emplace(std::to_string(newId), new ioteyeUser::User{std::to_string(newId)});
        // using userID variable to return response
        userID = std::to_string(newId);
    }
    debugMessage("newId " + userID); NEWLINE
    return std::shared_ptr<http_response>(new string_response("userID=" + userID, 200));
}

std::shared_ptr<http_response> user_resource::render_GET(const http_request &req)
{
    return std::shared_ptr<http_response>();
}

std::shared_ptr<http_response> device_resource::render_POST(const http_request &req)
{
    debugMessage("Device POST"); NEWLINE
    std::string cmd = req.get_arg("cmd");
    if (func::GetCommandCode(cmd) != ioteyeServer::REGISTER_DEVICE)
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));

    std::string userID = req.get_arg("userID");
    auto user = s_userPins.find(userID);

    if (user == s_userPins.end())
        return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));

    ioteyeDevice::Device *newDevice = new ioteyeDevice::Device();

    if (!user->second->addDevice(newDevice->getID(), newDevice))
        s_userDevices.emplace(newDevice->getID(), user->second);
    else
        return std::shared_ptr<http_response>(new string_response("Something went wrong!", 500));

    return std::shared_ptr<http_response>(new string_response("devID=" + std::to_string(newDevice->getID()), 201));
}

std::shared_ptr<http_response> device_resource::render_GET(const http_request &req)
{
    debugMessage("Device GET"); NEWLINE
    PAYLOAD_DEBUG(req.get_args())

    std::string cmd = req.get_arg("cmd");
    uint64_t devID = std::stoul(req.get_arg("devID"));
    std::string userID = req.get_arg("userID");
    uint8_t deviceStatus = 0;
    auto device = s_userDevices.find(devID);
    switch (func::GetCommandCode(cmd))
    {
    case ioteyeServer::DEVICE_STATUS:
        if (device != s_userDevices.end())
        {
            if (device->second->getID() == userID)
            {
                deviceStatus = device->second->getDeviceStatus(devID);
            }
            else
                return std::shared_ptr<http_response>(new string_response("This user doesnt have this device!", 403));
        }
        return std::shared_ptr<http_response>(new string_response("devStatus=" + std::to_string(deviceStatus), 200));
        break;
    case ioteyeServer::DEVICE_STATUS_UPDATE:
        if (device != s_userDevices.end())
        {
            if (device->second->getID() == userID)
                device->second->pingDevice(devID);
            else
                return std::shared_ptr<http_response>(new string_response("This user doesnt have this device!", 403));
        }
        return std::shared_ptr<http_response>(new string_response("Device status updated!", 200));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }
}
