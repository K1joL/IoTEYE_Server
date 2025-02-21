#include "client.hpp"

namespace ioteye {
Client::Client(const std::string &host, const std::string &port)
    : m_host(host), m_port(port), m_ioContext(), m_resolver(m_ioContext) {
    try {
        m_endpoints = m_resolver.resolve(m_host, m_port);
    } catch (const std::exception &e) {
        std::cerr << "Exception during endpoint resolution: " << e.what() << std::endl;
        throw;
    }
}

Response Client::sendRequest(uint8_t method, const std::string &endpoint) {
    try {
        std::stringstream requestStream;
        requestStream << getMethodStr(method) << " " << endpoint << " HTTP/1.1\r\n";
        requestStream << "Host: " << m_host << "\r\n";
        requestStream << "Content-Length: " << 0 << "\r\n";
        requestStream << "Content-Type: text/plain\r\n";
        requestStream << "Connection: close\r\n";
        requestStream << "\r\n";
        std::string request = requestStream.str();
        std::string response = sendTcpRequest(request, m_endpoints);
        std::cout << "Response: " << response << std::endl;

        return parseHttpResponse(response);

    } catch (const std::exception &e) {
        std::cerr << "Exception in sendRequest: " << e.what() << std::endl;
        return Response{"", "", 0};
    }
}

std::string Client::sendTcpRequest(const std::string &request, tcp::resolver::results_type &endpoints) {
    asio::ip::tcp::socket socket(m_ioContext);

    asio::connect(socket, endpoints);
    asio::write(socket, asio::buffer(request));

    asio::streambuf responseBuf;
    asio::error_code ec;
    asio::read_until(socket, responseBuf, "\r\n\r\n", ec);  // Read until end of headers

    if (ec && ec != asio::error::eof) {
        throw asio::system_error(ec);
    }

    std::istream responseStream(&responseBuf);
    std::string headerLine;
    std::string responseString;

    std::getline(responseStream, headerLine);
    if(headerLine != "\r")
        responseString += headerLine;
    while (std::getline(responseStream, headerLine) && headerLine != "\r") {
        responseString += '\n' + headerLine;
    }

    // Read the body
    std::stringstream bodyStream;
    bodyStream << responseStream.rdbuf(); // Read any data remaining in the streambuf

    // Keep reading from the socket until EOF or an error occurs
    while (ec != asio::error::eof) {
        char buffer[4096];
        size_t bytes_transferred = socket.read_some(asio::buffer(buffer), ec);

        if (bytes_transferred > 0) {
            bodyStream.write(buffer, bytes_transferred);
        }
        if (ec && ec != asio::error::eof) {
            throw asio::system_error(ec);
        }
    }

    return responseString + "\r\n\r\n" + bodyStream.str();
}

std::string extractValue(const std::string &responseText, const std::string &key) {
    size_t startPos = responseText.find_first_of(key + '=');
    if (startPos == std::string::npos)
        return "";
    startPos += key.length() + 1;
    size_t endPos = responseText.find_first_of(" \r\n", startPos);  // Find the end of the value
    if (endPos == std::string::npos) {
        return responseText.substr(startPos);
    }
    return responseText.substr(startPos, endPos - startPos);
}

Response parseHttpResponse(const std::string &httpResponse) {
    Response response;
    std::stringstream stream(httpResponse);
    std::string line;

    // 1. Parse the status line (e.g., "HTTP/1.1 200 OK")
    std::getline(stream, line);
    std::stringstream statusLineStream(line);
    std::string httpVersion;
    std::string statusCodeString;
    std::string statusText;

    statusLineStream >> httpVersion >> statusCodeString >>
        std::ws;  // Extract HTTP version and status code and discard leading whitespaces if any
    std::getline(statusLineStream, statusText);  // Read remaining text as statusText

    try {
        response.statusCode = std::stoi(statusCodeString);  // Convert string status code to int
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error parsing status code: " << e.what() << std::endl;
        response.statusCode = 0;
    } catch (const std::out_of_range &e) {
        std::cerr << "Status code out of range: " << e.what() << std::endl;
        response.statusCode = 0;
    }

    // 2. Parse the headers
    response.header = "";
    while (std::getline(stream, line) && !line.empty() && line != "\r") {
        // Header ends with blank line "\r\n" or "\n"
        response.header += line + "\n";
    }

    // 3. Parse the body (everything after the headers)
    std::stringstream bodyStream;
    while (std::getline(stream, line))
        // Reconstruct line endings, just in case.
        bodyStream << line << '\n';

    response.body = bodyStream.str();

    return response;
}

std::string getMethodStr(uint8_t method) {
    switch (method) {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case PUT:
            return "PUT";
        case DELETE:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

std::string Client::registerNewDevice() {
    std::string endpoint = "/devices";
    endpoint += REGISTER_DEVICE;

    Response response = sendRequest(POST, endpoint);
    if (response.statusCode == 0)
        return "";
    if (!response.body.empty()) {
        std::cout << "Register Response: " << response.body << std::endl;
        std::string token = extractValue(response.body, "token");
        return token;
    }
    return std::to_string(response.statusCode);
}

uint16_t Client::getDeviceStatus(const std::string &token) {
    std::string endpoint = "/devices";
    endpoint += '/' + token;
    endpoint += DEVICE_STATUS;

    Response response = sendRequest(GET, endpoint);
    if (response.statusCode == 0)
        return 0;
    if (!response.body.empty()) {
        std::cout << "Status Response: " << response.body << std::endl;
        std::string statusStr = extractValue(response.body, "devStatus");
        if (!statusStr.empty()) {
            uint16_t status = std::stoi(statusStr);
            std::cout << "Device is " << (status ? "Online" : "Offline") << std::endl;
            return status;
        }
    }

    return response.statusCode;
}

uint16_t Client::deleteDevice(const std::string &token) {
    std::string endpoint = "/devices";
    endpoint += '/' + token;
    endpoint += DELETE_DEVICE;

    return sendRequest(DELETE, endpoint).statusCode;
}
uint16_t Client::createVirtualPin(const std::string &token, const std::string &pinNumber,
                                  const std::string &dataType, const std::string &defaultData) {
    std::string endpoint = "/devices";
    endpoint += '/' + token;
    endpoint += "/pins";
    endpoint += '/' + pinNumber;
    endpoint += '/' + dataType;
    endpoint += '/' + defaultData;
    endpoint += CREATE_PIN;

    return sendRequest(POST, endpoint).statusCode;
}

uint16_t Client::writeVirtualPin(const std::string &token, const std::string &pinNumber,
                                 const std::string &value) {
    std::string endpoint = "/devices";
    endpoint += '/' + token;
    endpoint += "/pins";
    endpoint += '/' + pinNumber;
    endpoint += '/' + value;
    endpoint += UPDATE_PIN;

    return sendRequest(PUT, endpoint).statusCode;
}

uint16_t Client::deleteVirtualPin(const std::string &token, const std::string &pinNumber) {
    std::string endpoint = "/devices";
    endpoint += '/' + token;
    endpoint += "/pins";
    endpoint += '/' + pinNumber;
    endpoint += DELETE_PIN;

    return sendRequest(DELETE, endpoint).statusCode;
}

std::string Client::getVirtualPin(const std::string &token, const std::string &pinNumber) {
    std::string endpoint = "/devices";
    endpoint += '/' + token;
    endpoint += "/pins";
    endpoint += '/' + pinNumber;
    endpoint += GET_PIN;

    Response response = sendRequest(GET, endpoint);
    return extractValue(response.body, "PinValue");
}

int Client::getVirtualPinInt(const std::string &token, const std::string &pinNumber) {
    return std::stoi(getVirtualPin(token, pinNumber));
}

double Client::getVirtualPinDouble(const std::string &token, const std::string &pinNumber) {
    return std::stod(getVirtualPin(token, pinNumber));
}

}  // namespace ioteye
