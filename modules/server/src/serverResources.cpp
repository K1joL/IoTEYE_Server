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
#include <deviceManager/deviceManager.hpp>
#include <expected>
#include <limits>
#include <server/serverResources.hpp>

namespace ioteye::resource {
using namespace ioteye::server::debug;
using ioteye::HttpStatusCode;
using namespace ioteye::types;

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
    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::CREATE_PIN:

            if ((device->addPin(pinNumber, dataType, value)) == 0)
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
    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::GET_PIN:
            if ((value = device->getPin(pinNumber)) != "")
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
    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::UPDATE_PIN:
            if ((device->changePin(pinNumber, value)) == 0) {
                m_historyLogger->record(*device);
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
    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DELETE_PIN:
            if ((device->removePin(pinNumber)) == 0)
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
    DeviceParams params;
    bool deleteAfterDeath = false;

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::REGISTER_DEVICE: {
            // Create new Device
            if (deviceType == "Device")
                deleteAfterDeath = true;
            params = {.outdatedDelay = ao::getOptions().getOutdatedDelay(),
                      .offlineDelay = ao::getOptions().getOfflineDelay(),
                      .deadDelay = ao::getOptions().getDeadDelay(),
                      .maxPins = ao::getOptions().getMaxPins(),
                      .deleteAfterDeath = deleteAfterDeath};

            auto insertedDevice = m_deviceManager->createDevice(params);
            if (!insertedDevice) {
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Device was not inserted to map!");
            }
            logStatus("Device was inserted!");

            if (insertedDevice->getToken().empty()) {
                m_deviceManager->deleteDevice(insertedDevice->getObjID());
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Device token was not generated! Device will be deleted, "
                    "try again, please.");
            }
            logStatus("Inserted device token: ", insertedDevice->getToken());
            logStatus("DeviceMap Size: ", m_deviceManager->getSize());

            payload += insertedDevice->getToken();
            return std::make_shared<HttpResponse>(HttpStatusCode::CREATED,
                                                  payload);
            break;
        }
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
    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DEVICE_STATUS:
            deviceStatus = device->getState();
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
    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DEVICE_STATUS_UPDATE:
            device->ping();
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
    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    switch (ioteye::GetCommandCode(cmd)) {
        case ioteye::DELETE_DEVICE:
            m_deviceManager->deleteDevice(device);
            return std::make_shared<HttpResponse>(HttpStatusCode::OK,
                                                  "Device deleted!");
            break;
        default:
            return std::make_shared<HttpResponse>(HttpStatusCode::BAD_REQUEST,
                                                  "Wrong command!");
    }
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

    auto result = authCheck(token);
    if (!result.has_value()) {
        switch (result.error()) {
            case HttpStatusCode::BAD_REQUEST:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::BAD_REQUEST, "Device doesn`t exist!");
            case HttpStatusCode::UNAUTHORIZED:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::UNAUTHORIZED, "Auth failure!");
            default:
                return std::make_shared<HttpResponse>(
                    HttpStatusCode::INTERNAL_SERVER_ERROR,
                    "Internal auth error");
        }
    }

    auto& device = result.value();
    if (!device)
        return std::make_shared<HttpResponse>(
            HttpStatusCode::INTERNAL_SERVER_ERROR, "Something went wrong");

    int64_t since = 0;
    int64_t until = std::numeric_limits<int64_t>::max();
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
        m_historyLogger->query(device->getDeviceID(), since, until, limit);
    return std::make_shared<HttpResponse>(HttpStatusCode::OK, body);
}

std::expected<types::DevicePtr, HttpStatusCode> ServerResource::authCheck(
    const std::string& token) {
    try {
        DevicePtr device = m_deviceManager->findByToken(token);
        if (!device) {
            return std::unexpected(HttpStatusCode::UNAUTHORIZED);
        }
        return device;
    } catch (const std::exception& e) {
        std::cerr << "[ServerResource] Auth failure: " << e.what() << '\n';
        return std::unexpected(HttpStatusCode::INTERNAL_SERVER_ERROR);
    }
}

ServerResource::ServerResource(DeviceManagerPtr deviceManager,
                               HistoryLoggerPtr historyLogger)
    : m_deviceManager(deviceManager), m_historyLogger(historyLogger) {
}

}  // namespace ioteye::resource