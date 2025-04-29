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

#ifndef IOTEYE_CLIENT_HPP
#define IOTEYE_CLIENT_HPP

#include <asio.hpp>
#include <iostream>
#include <sstream>
#include <string>

namespace ioteye {
using asio::ip::tcp;

enum HTTP_METHOD { GET, POST, PUT, DELETE };

struct Response {
    std::string body;
    std::string header;
    uint16_t statusCode;
};

class Client {
public:
    Client(const std::string& host, const std::string& port);

    Response sendRequest(uint8_t method, const std::string& endpoint);

    std::string registerNewDevice();
    uint16_t getDeviceStatus(const std::string& token);
    uint16_t updateDeviceStatus(const std::string& token);
    uint16_t deleteDevice(const std::string& token);
    uint16_t createVirtualPin(const std::string& token, const std::string& pinNumber,
                              const std::string& dataType, const std::string& defaultData);
    uint16_t writeVirtualPin(const std::string& token, const std::string& pinNumber,
                             const std::string& value);
    uint16_t deleteVirtualPin(const std::string& token, const std::string& pinNumber);
    std::string getVirtualPin(const std::string& token, const std::string& pinNumber);
    int getVirtualPinInt(const std::string& token, const std::string& pinNumber);
    double getVirtualPinDouble(const std::string& token, const std::string& pinNumber);

private:
    std::string sendTcpRequest(const std::string& request, tcp::resolver::results_type& endpoints);

// Commands
#define REGISTER_DEVICE "/rd"       // register_device
#define DELETE_DEVICE "/dd"         // delete_device
#define DEVICE_STATUS "/ds"         // device_status
#define DEVICE_STATUS_UPDATE "/us"  // device_status_update
#define CREATE_PIN "/cp"            // create_pin
#define UPDATE_PIN "/up"            // update_pin
#define DELETE_PIN "/dp"            // delete_pin
#define GET_PIN "/pv"               // get_pin

private:
    std::string m_host = {"127.0.0.1"};
    std::string m_port = {"8080"};
    asio::io_context m_ioContext;
    tcp::resolver m_resolver;
    tcp::resolver::results_type m_endpoints;
};

std::string extractValue(const std::string& responseText, const std::string& key);
Response parseHttpResponse(const std::string& httpResponse);
std::string getMethodStr(uint8_t method);
}  // namespace ioteye
#endif  // !IOTEYE_CLIENT_HPP