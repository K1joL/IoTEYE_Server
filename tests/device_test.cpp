#include <gtest/gtest.h>
#include "device.h"
using namespace ioteye;

TEST(DeviceTest, DefaultConstructor) {
    Device device;
    EXPECT_EQ(device.getState(), Device::OFFLINE);
    EXPECT_EQ(device.getMaxPins(), 255);
}

TEST(DeviceTest, GetID){
    Device device2;
    Device device3;
    EXPECT_EQ(device2.getID(), 2);
    EXPECT_EQ(device3.getID(), 3);
}

TEST(DeviceTest, ParameterizedConstructor) {
    Device device(1000, 2000, 50);
    EXPECT_EQ(device.getMaxPins(), 50);
}

TEST(DeviceTest, ChangeState) {
    Device device;
    device.changeState(Device::ONLINE);
    EXPECT_EQ(device.getState(), Device::ONLINE);
}

TEST(DeviceTest, DelaysTest){
    Device device;
    device.ping();
    // std::cout << "MeowTest" << std::endl; 
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    EXPECT_EQ(device.getState(), Device::ONLINE);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
    EXPECT_EQ(device.getState(), Device::OUTDATED);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500)); 
    EXPECT_EQ(device.getState(), Device::OFFLINE);
}

TEST(DeviceTest, GenerateToken) {
    Device device;
    device.generateToken();
    EXPECT_FALSE(device.getToken().empty());
}


TEST(DeviceTest, AddPin) {
    Device device;
    EXPECT_EQ(device.addPin(1, "int", "0"), 0);
    EXPECT_EQ(device.getIntPins().size(), 1);
}

TEST(DeviceTest, AddPinLimit){
    Device device(1000, 2000, 10);
    EXPECT_EQ(device.addPin(0, "int", "0"), 0);
    EXPECT_EQ(device.addPin(1, "int", "0"), 0);
    EXPECT_EQ(device.addPin(2, "int", "0"), 0);
    EXPECT_EQ(device.addPin(3, "int", "0"), 0);
    EXPECT_EQ(device.addPin(4, "int", "0"), 0);
    EXPECT_EQ(device.addPin(5, "int", "0"), 0);
    EXPECT_EQ(device.addPin(6, "int", "0"), 0);
    EXPECT_EQ(device.addPin(7, "int", "0"), 0);
    EXPECT_EQ(device.addPin(8, "int", "0"), 0);
    EXPECT_EQ(device.addPin(9, "int", "0"), 0);
    EXPECT_EQ(device.addPin(10, "int", "0"), 2);
}

TEST(DeviceTest, ChangePin) {
    Device device;
    device.addPin(1, "int", "0");
    EXPECT_EQ(device.changePin(1, "10"), 0);
    EXPECT_EQ(device.getIntPins().at(1), 10);
}

TEST(DeviceTest, RemovePin) {
    Device device;
    device.addPin(1, "int", "0");
    EXPECT_EQ(device.removePin(1), 0);
    EXPECT_EQ(device.getIntPins().size(), 0);
}

TEST(DeviceTest, GetPin) {
    Device device;
    device.addPin(1, "int", "0");
    EXPECT_EQ(device.getPin(1), "0");
}