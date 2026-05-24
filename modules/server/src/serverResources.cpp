/*# MIT License

# Copyright (c) 2025 Shults Bogdan aka K1joL

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/

#include <common/adminOptions.hpp>
#include <common/commands.hpp>
#include <common/logging.hpp>
#include <server/serverResources.hpp>
namespace ioteye::resource {
using namespace ioteye::server::debug;
using ioteye::HttpStatusCode;
using namespace ioteye::types;

std::unordered_map<DeviceID, DevicePtr> s_idDeviceMap;

PinsResource::PinsResource(
    std::shared_ptr<persistence::IPinHistoryLogger> historyLogger)
    : m_pinHistoryLogger(historyLogger) {
}

std::shared_ptr<HttpResponse> PinsResource::renderPOST(const HttpRequest& req) {
    logStatus("pins POST\n", req.getArgs());

    // Getting request arguments
    PinId pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string dataType{req.getArg("dataType")};
    std::string token{req.getArg("token")};
    std::string value{req.getArg("value")};
    std::string cmd{req.getArg("cmd")};
    std::string content = "PinValue=";

    // existence checks
    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::CREATE_PIN:

            if ((device->second->addPin(pinNumber, dataType, value)) == 0)
                return std::make_shared<HttpResponse>(HttpStatusCode::CREATED,
                                                      "Pin created!");
            else
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Pin already exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> PinsResource::renderGET(const HttpRequest& req) {
    logStatus("PINS GET\n", req.getArgs());

    // Getting request arguments
    PinId pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string token{req.getArg("token")};
    std::string cmd{req.getArg("cmd")};
    std::string value{};
    // existence checks
    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::GET_PIN:
            if ((value = device->second->getPin(pinNumber)) != "")
                return std::make_shared<HttpResponse>(HttpStatusCode::OK,
                                                      "PinValue=" + value);
            else
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Pin does not exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> PinsResource::renderPUT(const HttpRequest& req) {
    logStatus("PINS PUT\n", req.getArgs());

    // Getting request arguments
    PinId pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string token{req.getArg("token")};
    std::string value{req.getArg("value")};
    std::string cmd{req.getArg("cmd")};

    // existence checks
    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::UPDATE_PIN:
            if ((device->second->changePin(pinNumber, value)) == 0) {
                m_pinHistoryLogger->record(*device->second);
                return std::make_shared<HttpResponse>(HttpStatusCode::OK,
                                                      "Pin changed");
            } else
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Pin does not exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> PinsResource::renderDELETE(
    const HttpRequest& req) {
    logStatus("PINS DELETE\n", req.getArgs());

    // Getting request arguments
    PinId pinNumber = std::stoi(req.getArg("pinNumber"));
    std::string token{req.getArg("token")};
    std::string cmd{req.getArg("cmd")};

    // existence checks
    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DELETE_PIN:
            if ((device->second->removePin(pinNumber)) == 0)
                return std::make_shared<HttpResponse>(HttpStatusCode::OK,
                                                      "Pin deleted");
            else
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Pin does not exists!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderPOST(
    const HttpRequest& req) {
    logStatus("DEVICE POST\n Request Arguments:\n", req.getArgs());

    std::string deviceType = req.getHeaderValue(HEADER_DEVICE_TYPE);
    std::string cmd{req.getArg("cmd")};
    std::string payload{"token="};
    DevicePtr newDevice;
    bool deleteAfterDeath = false;
    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::REGISTER_DEVICE:
            // Create new Device
            if (deviceType == "Device")
                deleteAfterDeath = true;
            newDevice = std::make_shared<ioteye::Device>(
                ao::getOptions().getOutdatedDelay(),
                ao::getOptions().getOfflineDelay(),
                ao::getOptions().getDeadDelay(), ao::getOptions().getMaxPins(),
                deleteAfterDeath);
            payload += newDevice->getToken();
            if (newDevice != nullptr) {
                logStatus(
                    s_idDeviceMap.emplace(newDevice->getID(), newDevice).second
                        ? "Device inserted!"
                        : "Device failed to insert!");
                auto it = s_idDeviceMap.find(newDevice->getID());
                logStatus("Inserted device token: ",
                          it != s_idDeviceMap.end() ? it->second->getToken()
                                                    : "Nothing");
                logStatus("DeviceMap Size: ", s_idDeviceMap.size());
                return std::make_shared<HttpResponse>(HttpStatusCode::CREATED,
                                                      payload);
            } else
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Something went wrong!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderGET(
    const HttpRequest& req) {
    logStatus("DEVICE GET\n", req.getArgs());

    // Getting request arguments
    std::string cmd{req.getArg("cmd")};
    std::string token{req.getArg("token")};
    DeviceState deviceStatus = DeviceState::MAX_STATE;

    // existence check
    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DEVICE_STATUS:
            deviceStatus = device->second->getState();
            return std::make_shared<HttpResponse>(
                HttpStatusCode::OK,
                "devStatus=" + std::to_string(deviceStatus));
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderPUT(
    const HttpRequest& req) {
    logStatus("DEVICE PUT\n", req.getArgs());

    // Getting request arguments
    std::string cmd{req.getArg("cmd")};
    std::string token{req.getArg("token")};

    // existence check
    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DEVICE_STATUS_UPDATE:
            device->second->ping();
            return std::make_shared<HttpResponse>(HttpStatusCode::OK,
                                                  "Device status updated!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

std::shared_ptr<HttpResponse> DeviceResource::renderDELETE(
    const HttpRequest& req) {
    logStatus("DEVICE DELETE\n", req.getArgs());

    // Getting request arguments
    std::string cmd{req.getArg("cmd")};
    std::string token{req.getArg("token")};

    // existence check
    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    // auto temp = device->second;
    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DELETE_DEVICE:
            s_idDeviceMap.erase(device);
            // delete temp;
            return std::make_shared<HttpResponse>(HttpStatusCode::OK,
                                                  "Device deleted!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
}

HistoryResource::HistoryResource(
    std::shared_ptr<persistence::IPinHistoryLogger> historyLogger)
    : m_pinHistoryLogger(historyLogger) {
}

std::shared_ptr<HttpResponse> HistoryResource::renderPOST(const HttpRequest&) {
    return createMethodNotAllowed("GET");
}

std::shared_ptr<HttpResponse> HistoryResource::renderPUT(const HttpRequest&) {
    return createMethodNotAllowed("GET");
}

std::shared_ptr<HttpResponse> HistoryResource::renderDELETE(
    const HttpRequest&) {
    return createMethodNotAllowed("GET");
}

std::shared_ptr<HttpResponse> HistoryResource::renderGET(
    const HttpRequest& req) {
    logStatus("HISTORY GET\n", req.getArgs());

    std::string token{req.getArg("token")};
    std::string sinceStr{req.getArg("since")};
    std::string untilStr{req.getArg("until")};
    std::string limitStr{req.getArg("limit")};

    DeviceIter device;
    switch (authCheck(token, device)) {
        case HttpStatusCode::BAD_REQUEST:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Device doesn`t exist!");
        case HttpStatusCode::UNAUTHORIZED:
            return std::make_shared<HttpResponse>(HttpStatusCode::UNAUTHORIZED,
                                                  "Auth failure!");
        case HttpStatusCode::OK:
            break;
        default:
            break;
    }
    if (device == s_idDeviceMap.end())
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    int64_t since = 0;
    int64_t until = 2147483647LL;
    uint32_t limit = 1000;
    try {
        if (!sinceStr.empty())
            since = std::stoll(sinceStr);
        if (!untilStr.empty())
            until = std::stoll(untilStr);
        if (!limitStr.empty())
            limit = static_cast<uint32_t>(std::stoul(limitStr));
    } catch (...) {
        return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                              "Invalid history parameters");
    }

    const std::string body =
        m_pinHistoryLogger->query(device->second->getID(), since, until, limit);
    return std::make_shared<HttpResponse>(HttpStatusCode::OK, body);
}

uint16_t authCheck(const std::string& token, DeviceIter& deviceIter) {
    try {
        const auto decodedToken = jwt::decode(token);
        uint64_t devID =
            std::stoul(decodedToken.get_payload_claim("deviceID").as_string());
        auto device = s_idDeviceMap.find(devID);
        if (device == s_idDeviceMap.end())
            return HttpStatusCode::BAD_REQUEST;
        if (device->second->getToken() != token)
            return HttpStatusCode::UNAUTHORIZED;
        deviceIter = device;
    } catch (std::exception& e) {
        std::cerr << "JWT Failure: " << e.what() << std::endl;
        return HttpStatusCode::BAD_REQUEST;
    }
    return HttpStatusCode::OK;
}
}  // namespace ioteye::resource