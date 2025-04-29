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

#include "file_manager.h"
#include "adminOptions.h"

using json = nlohmann::json;

namespace ioteye {

bool DeviceFileManager::saveFile() {
    json j;
    for (const auto& pair : m_devicesMap) {
        json device;
        device["id"] = pair.second->getID();
        device["token"] = pair.second->getToken();
        device["maxPins"] = pair.second->getMaxPins();
        device["pinsCounter"] = pair.second->pinsCreated();
        // Save virtual pins data
        json intPins = json::array();
        for (const auto& pin : pair.second->getIntPins())
            intPins.push_back({pin.first, pin.second});
        device["intPins"] = intPins;

        json doublePins = json::array();
        for (const auto& pin : pair.second->getDoublePins()) {
            doublePins.push_back({pin.first, pin.second});
        }
        device["doublePins"] = doublePins;

        json stringPins = json::array();
        for (const auto& pin : pair.second->getStringPins()) {
            stringPins.push_back({pin.first, pin.second});
        }
        device["stringPins"] = stringPins;
        j.push_back(device);
    }
    m_fileHandler->writeSync(j.dump(4));
    return true;
}

bool DeviceFileManager::loadFile() {
    std::string fileContent;
    m_fileHandler->readAll(fileContent);
    json j = json::parse(fileContent);
    int count = 0;
    for (const auto& device : j) {
        std::unordered_map<uint16_t, int> intPins;
        std::unordered_map<uint16_t, double> doublePins;
        std::unordered_map<uint16_t, std::string> stringPins;
        std::unordered_map<uint16_t, uint8_t> pinsType;

        for (const auto& pin : device["intPins"]) {
            pinsType[pin[0]] = Device::ContainerID::INTID;
            intPins[pin[0]] = pin[1];
        }
        for (const auto& pin : device["doublePins"]) {
            pinsType[pin[0]] = Device::ContainerID::DOUBLEID;
            doublePins[pin[0]] = pin[1];
        }
        for (const auto& pin : device["stringPins"]) {
            pinsType[pin[0]] = Device::ContainerID::STRINGID;
            stringPins[pin[0]] = pin[1];
        }

        DevicePtr newDevice =
            std::make_shared<Device>(Device::Builder()
                                         .setID(device["id"])
                                         .setToken(device["token"])
                                         .setMaxPins(device["maxPins"])
                                         .setIntPinMap(std::move(intPins))
                                         .setDoublePinMap(std::move(doublePins))
                                         .setStringPinMap(std::move(stringPins))
                                         .setPinsTypeMap(std::move(pinsType))
                                         .setOfflineDelay(OPTIONS->getOfflineDelay())
                                         .setOutdatedDelay(OPTIONS->getOutdatedDelay())
                                         .build());
        // Initialize other relevant data if necessary
        auto emplaceIt = m_devicesMap.emplace(newDevice->getID(), newDevice);
        // server::debug::log(newDevice->getPinsTypes().size());
        // server::debug::log(newDevice->getIntPins().size());
        // server::debug::log(newDevice->getStringPins().size());
        // server::debug::log(newDevice->getDoublePins().size());
        ++count;
        ioteye::server::debug::log("Device loading ", count, '/', j.size(),
                                   emplaceIt.second ? " Success" : "Failed");
    }
    return true;
}

FileHandler::~FileHandler() {
    closeFile();
}

void FileHandler::writeAsync(const std::string& line) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push(line);
    m_queueCondition.notify_one();  // Notify the worker thread
}

bool FileHandler::writeSync(const std::string& line) {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    if (m_fileStream.is_open()) {
        m_fileStream << line;
        return m_fileStream.good();
    }
    return false;
}

bool FileHandler::readLine(std::string& line) {
    std::lock_guard<std::mutex> lock(m_fileMutex);

    m_fileStream.clear();
    // Save the current file pointer position
    auto originalPos = m_fileStream.tellg();

    // Move to the current read pos of the file
    m_fileStream.seekg(m_currentReadPos);

    if (m_fileStream.is_open() && std::getline(m_fileStream, line)) {
        m_currentReadPos = m_fileStream.tellg();
        return true;
    } else {
        // Handle errors and EOF
        if (m_fileStream.eof()) {
            // Reset position
            m_currentReadPos = 0;
            return false;  // EOF reached
        } else {
            std::cerr << "Error reading line in readLineStaticPosition."
                      << std::endl;
            return false;  // Error occurred
        }
    }
}

bool FileHandler::readAll(std::string& content) {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    // Save the current file pointer position
    auto originalPos = m_fileStream.tellg();
    // Move to the beginning of the file
    m_fileStream.seekg(0, std::ios::beg);
    // Read the entire file
    content = std::string((std::istreambuf_iterator<char>(m_fileStream)),
                          std::istreambuf_iterator<char>());
    // Restore the original file pointer position
    m_fileStream.seekg(originalPos);

    return m_fileStream.good();
}

void FileHandler::closeFile() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_shutdown = true;              // Signal the worker thread to stop
        m_queueCondition.notify_all();  // Wake up the worker thread
    }
    m_fileThread.join();  // Wait for the worker thread to finish
    // Close the file stream
    if (m_fileStream.is_open())
        m_fileStream.close();
}

void FileHandler::openFile(const std::string& filename,
                           std::ios_base::openmode mode) {
    if (filename.empty())
        throw std::invalid_argument("File path cannot be empty!");
    m_fileStream.open(m_filename, mode);
    if (!m_fileStream.is_open())
        throw std::runtime_error("Failed to open file: " + m_filename);
    m_fileThread = std::thread(&FileHandler::fileWorker, this);
}

void FileHandler::openFile(const std::string& filename) {
    openFile(filename, m_mode);
}

void FileHandler::fileWorker() {
    while (true) {
        std::string line;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            // Wait for a new task or shutdown signal
            m_queueCondition.wait(
                lock, [this] { return !m_queue.empty() || m_shutdown; });

            // If shutdown is requested and the queue is empty, exit the loop
            if (m_shutdown && m_queue.empty())
                break;

            // Get the next task from the queue
            if (!m_queue.empty()) {
                line = m_queue.front();
                m_queue.pop();
            }
        }

        // Perform the file I/O operation
        if (!line.empty()) {
            std::lock_guard<std::mutex> lock(m_fileMutex);
            m_fileStream << line;
            if (!m_fileStream.good()) {
                std::cerr << "Failed to write to file: " << m_filename
                          << std::endl;
            }
        }
    }
}

}  // namespace ioteye