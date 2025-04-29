#include "file_manager.h"

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>

#include "adminOptions.h"

#ifdef _WIN32
#include <io.h>  // For _mktemp_s
#else
#include <unistd.h>  // For mkstemp
#endif

#include "device.h"


std::unique_ptr<AdminOptions> OPTIONS = std::make_unique<AdminOptions>();
using json = nlohmann::json;

namespace ioteye {

class FileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary file for testing
#ifdef _WIN32
        char templateName[] = "tempfileXXXXXX";
        if (_mktemp_s(templateName, sizeof(templateName)) != 0) {
            perror("_mktemp_s");
            throw "cannot create file";
        }
#else
        char templateName[] = "/tmp/tempfileXXXXXX";
        int fd = mkstemp(templateName);
        if (fd == -1) {
            perror("mkstemp");
            throw "cannot create file";
        }
#endif

        fileHandler = std::make_shared<FileHandler>(
            templateName, std::ios::out | std::ios::in | std::ios::trunc);
        deviceFileManager =
            std::make_unique<DeviceFileManager>(fileHandler, devicesMap);
    }

    void TearDown() override {
        // Remove the temporary file
        std::cout << fileHandler.use_count() << std::endl;
        deviceFileManager.reset();
        std::cout << fileHandler.use_count() << std::endl;
        fileHandler.reset();
        std::this_thread::sleep_for(ms(100));
        std::remove(tempFile.c_str());
    }

    std::string tempFile;
    std::shared_ptr<FileHandler> fileHandler;
    std::unordered_map<uint64_t, DevicePtr> devicesMap;
    std::unique_ptr<DeviceFileManager> deviceFileManager;
};
// FileHandler Tests
TEST_F(FileManagerTest, WriteSync) {
    std::string testLine = "Test line";
    ASSERT_TRUE(fileHandler->writeSync(testLine));

    std::string readLine;
    ASSERT_TRUE(fileHandler->readLine(readLine));
    ASSERT_EQ(readLine, testLine);
}

TEST_F(FileManagerTest, WriteAsync) {
    std::string testLine = "Test line";
    fileHandler->writeAsync(testLine);

    // Give some time for the async write to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::string readLine;
    ASSERT_TRUE(fileHandler->readLine(readLine));
    ASSERT_EQ(readLine, testLine);
}

TEST_F(FileManagerTest, ReadAll) {
    std::string testContent = "Line 1\nLine 2\nLine 3";
    fileHandler->writeSync(testContent);

    std::string readContent;
    ASSERT_TRUE(fileHandler->readAll(readContent));
    ASSERT_EQ(readContent, testContent);
}

// DeviceFileManager Tests
TEST_F(FileManagerTest, SaveAndLoadFile) {
    // Create a device and add it to the devices map
    DevicePtr device = std::make_shared<Device>(
        Device::Builder().setID(1).setToken("token1").setMaxPins(10).build());
    devicesMap[1] = device;

    // Save the file
    ASSERT_TRUE(deviceFileManager->saveFile());

    // Clear the devices map
    devicesMap.clear();

    // Load the file
    ASSERT_TRUE(deviceFileManager->loadFile());

    // Check if the device is loaded correctly
    ASSERT_EQ(devicesMap.size(), 1);
    ASSERT_EQ(devicesMap[1]->getID(), 1);
    ASSERT_EQ(devicesMap[1]->getToken(), "token1");
    ASSERT_EQ(devicesMap[1]->getMaxPins(), 10);
}

}  // namespace ioteye
