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

#ifndef SERVER_RESOURCES_HPP
#define SERVER_RESOURCES_HPP

#include <common/adminOptions.hpp>
#include <common/types.hpp>
#include <device/device.hpp>
#include <expected>
#include <ioteyeserver.hpp>
#include <memory>
#include <persistence/historyLogger/pinHistoryLogger.hpp>
#include <unordered_map>

namespace ioteye {
class DeviceManager;

}  // namespace ioteye

namespace ioteye::resource {
using ao = ioteye::AdminOptions;

using std::cout;
using std::endl;

using DeviceManagerPtr = std::shared_ptr<DeviceManager>;
using HistoryLoggerPtr = std::shared_ptr<persistence::IPinHistoryLogger>;

#define HEADER_DEVICE_TYPE "X-Device-Type"

class ServerResource : public HttpResourceHandler {
public:
    ServerResource(DeviceManagerPtr deviceManager,
                   HistoryLoggerPtr historyLogger);

protected:
    /// @brief Token authentication. Checks whether a device with the specified
    /// token exists.
    /// @param token The device token with which to authenticate.
    /// @return DevicePtr on success, HttpStatusCode error on failure.
    std::expected<types::DevicePtr, HttpStatusCode> authCheck(
        const std::string& token);

protected:
    DeviceManagerPtr m_deviceManager;
    HistoryLoggerPtr m_historyLogger;
};

class PinsResource : public ServerResource {
public:
    using ServerResource::ServerResource;

    std::shared_ptr<HttpResponse> renderPOST(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderGET(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderPUT(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderDELETE(const HttpRequest& req) override;
};

class DeviceResource : public ServerResource {
public:
    using ServerResource::ServerResource;

    std::shared_ptr<HttpResponse> renderPOST(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderGET(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderPUT(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderDELETE(const HttpRequest& req) override;
};

class HistoryResource : public ServerResource {
public:
    using ServerResource::ServerResource;

    std::shared_ptr<HttpResponse> renderPOST(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderGET(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderPUT(const HttpRequest& req) override;
    std::shared_ptr<HttpResponse> renderDELETE(const HttpRequest& req) override;
};
}  // namespace ioteye::resource

#endif  // SERVER_RESOURCES_HPP