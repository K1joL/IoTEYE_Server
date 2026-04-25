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

#ifndef IOTEYE_FILE_MANAGER_H
#define IOTEYE_FILE_MANAGER_H

#include <common/logging.hpp>
#include <condition_variable>
#include <device/device.hpp>
#include <fstream>
#include <logging.hpp>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>
#include <thread>

namespace ioteye {

class FileHandler {
public:
    FileHandler(const std::string& filename, std::ios_base::openmode mode)
        : m_filename(filename), m_mode(mode) {
        openFile(filename, mode);
    }

    ~FileHandler();
    void writeAsync(const std::string& line);
    bool writeSync(const std::string& line);
    bool readLine(std::string& line);
    bool readAll(std::string& content);
    void closeFile();
    void openFile(const std::string& filename, std::ios_base::openmode mode);
    void openFile(const std::string& filename);

private:
    void fileWorker();

    std::string m_filename;
    std::ios_base::openmode m_mode;
    std::thread m_fileThread;
    std::queue<std::string> m_queue;  // Write queue
    std::mutex m_queueMutex;          // Mutex to protect the write queue
    std::mutex m_fileMutex;           // Mutex to protect file access for reads
    std::condition_variable m_queueCondition;
    bool m_shutdown = false;
    std::fstream m_fileStream;
    std::streampos m_currentReadPos = 0;
};

class IFileManager {
public:
    explicit IFileManager(std::shared_ptr<FileHandler> fileHandler)
        : m_fileHandler(fileHandler) {
    }
    virtual ~IFileManager() = default;
    virtual bool saveFile() = 0;
    virtual bool loadFile() = 0;

protected:
    std::shared_ptr<FileHandler> m_fileHandler;
};

class DeviceFileManager : public IFileManager {
public:
    DeviceFileManager(std::shared_ptr<FileHandler> fileHandler,
                      std::unordered_map<uint64_t, DevicePtr>& devices)
        : IFileManager(fileHandler), m_devicesMap(devices) {
    }
    bool saveFile() override;
    bool loadFile() override;

private:
    std::unordered_map<uint64_t, DevicePtr>& m_devicesMap;
};

}  // namespace ioteye

#endif  // IOTEYE_FILE_MANAGER_H