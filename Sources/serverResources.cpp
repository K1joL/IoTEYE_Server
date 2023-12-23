#include "serverResources.h"

std::unordered_map<std::string, ioteyeUser::User*> userPins;

std::shared_ptr<http_response> pins_resource::render_POST(const http_request &req)
{
    for (auto &e : req.get_args())
    {
        std::cout << e.first << ": " << static_cast<std::string>(e.second) << std::endl;
    }
    uint8_t pinNumber = 0;
    std::string dataType{};
    std::string userID{};
    std::string value{};
    std::string cmd{};

    cmd = req.get_arg("cmd");
    if(func::GetCommandCode(cmd) != ioteyeServer::CREATE_PIN)
    userID = req.get_arg("userID");
    pinNumber = std::stoi(req.get_arg("pinNumber"));
    dataType = req.get_arg("dataType");
    value = req.get_arg("defaultValue");
    userPins.emplace(userID, new ioteyeUser::User{userID});

    if (userPins.find(userID)->second->addPin(pinNumber, dataType, value))
        std::cerr << "Error" << endl
                  << "Data type: " << dataType << endl
                  << "pinNumber: " << (int)pinNumber << endl
                  << "defaultValue: " << static_cast<std::string>(value) << endl;

    return std::shared_ptr<http_response>(new string_response("Pin created!", 200));
}

std::shared_ptr<http_response> pins_resource::render_GET(const http_request &req)
{
    return std::shared_ptr<http_response>(new string_response("Pin value: ", 200, "text/plain"));
}
