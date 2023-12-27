#include "serverResources.h"

std::shared_ptr<http_response> pins_resource::render_POST(const http_request &req)
{
    // for (auto &e : req.get_args())
    // {
    //     std::cout << e.first << ": " << static_cast<std::string>(e.second) << std::endl;
    // }
    cout << "POST" << endl;
    uint8_t pinNumber = 0;
    std::string dataType{};
    std::string userID{};
    std::string value{};
    std::string cmd{};
    std::string content = "PinValue=";
    auto user = userPins.begin();

    //command check
    cmd = req.get_arg("cmd");
    switch(func::GetCommandCode(cmd))
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
        user = userPins.find(userID);
        if (user != userPins.end())
            if ((user->second->addPin(pinNumber, dataType, value)) == 0)
                return std::shared_ptr<http_response>(new string_response("Pin created!", 201));
            else
                return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
        else
            return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));
        break;

    case ioteyeServer::GET_PIN:
        // Getting request arguments
        userID = req.get_arg("userID");
        pinNumber = std::stoi(req.get_arg("pinNumber"));

        // existence checks
        user = userPins.find(userID);
        if (user != userPins.end())
            if ((value = user->second->getPin(pinNumber)) != "")
            {
                content += value;
                return std::shared_ptr<http_response>(new string_response(content, 200));
            }
            else
                return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
        else
            return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));
        break;
    default:
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    }    
}

std::shared_ptr<http_response> pins_resource::render_PUT(const http_request &req)
{
    cout << "PUT" << endl;
    uint8_t pinNumber = 0;
    std::string userID{};
    std::string value{};
    std::string cmd{};

    cmd = req.get_arg("cmd");
    if(func::GetCommandCode(cmd) != ioteyeServer::UPDATE_PIN)
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    //Getting request arguments
    userID = req.get_arg("userID");
    pinNumber = std::stoi(req.get_arg("pinNumber"));
    value = req.get_arg("value");

    //existence checks
    auto user = userPins.find(userID);
    if(user != userPins.end())
        if((user->second->changePin(pinNumber, value)) == 0)
            return std::shared_ptr<http_response>(new string_response("Pin changed", 200));
        else 
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
    else 
        return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));
}

std::shared_ptr<http_response> pins_resource::render_DELETE(const http_request &req)
{
    cout << "DELETE" << endl;
    uint8_t pinNumber = 0;
    std::string userID{};
    std::string cmd{};

    cmd = req.get_arg("cmd");
    if(func::GetCommandCode(cmd) != ioteyeServer::DELETE_PIN)
        return std::shared_ptr<http_response>(new string_response("Wrong command!", 400));
    //Getting request arguments
    userID = req.get_arg("userID");
    pinNumber = std::stoi(req.get_arg("pinNumber"));

    //existence checks
    auto user = userPins.find(userID);
    if(user != userPins.end())
        if((user->second->removePin(pinNumber)) == 0)
            return std::shared_ptr<http_response>(new string_response("Pin deleted", 200));
        else 
            return std::shared_ptr<http_response>(new string_response("Pin does not exists!", 400));
    else 
        return std::shared_ptr<http_response>(new string_response("User does not exists!", 400));
}

std::shared_ptr<http_response> user_resource::render_POST(const http_request &req)
{
    cout << "user POST" << endl;
    std::string userID = req.get_arg("customID");
    cout << "userid " << userID << endl;

    if(userID != "")
        if(userPins.find(userID) == userPins.end())
            userPins.emplace(userID, new ioteyeUser::User{userID});
        else std::shared_ptr<http_response>(new string_response("This ID already exists!", 400));
    else
    {
        // find unoccupied ID
        uint64_t newId = 0;
        do
        {
            newId = ioteyeUser::User::getNextID();
            cout << "newId " << newId << endl;

        } while (userPins.find(std::to_string(newId)) != userPins.end());
        
        userPins.emplace(std::to_string(newId), new ioteyeUser::User{std::to_string(newId)});
        // using userID variable to return response
        userID = std::to_string(newId);
    }

    return std::shared_ptr<http_response>(new string_response("userID=" + userID, 200));
}

std::shared_ptr<http_response> user_resource::render_GET(const http_request &req)
{
    return std::shared_ptr<http_response>();
}

std::shared_ptr<http_response> device_resource::render_POST(const http_request &req)
{
    
    return std::shared_ptr<http_response>();
}

std::shared_ptr<http_response> device_resource::render_GET(const http_request &req)
{

    return std::shared_ptr<http_response>();
}
