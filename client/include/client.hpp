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