#include "serverResources.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "adminOptions.h"

std::unique_ptr<AdminOptions> OPTIONS = std::make_unique<AdminOptions>();
using namespace ioteye;

#define PINNUM "2"
#define PINTYPE "int"
#define PINDEFVAL "999"
#define DEVID "0"

std::string makeTestDevice() {
    auto newDevice =
        std::make_shared<Device>(Device::Builder()
                                     .setIntPin(std::stoi(PINNUM), std::stoi(PINDEFVAL))
                                     .setID(std::stoi(DEVID))
                                     .setMaxPins(5)
                                     .build());
    newDevice->generateToken();
    s_idDeviceMap.emplace(newDevice->getID(), newDevice);
    return newDevice->getToken();
}

class PinsResourcesTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_resource =
            std::make_unique<HttpResource>(std::make_shared<PinsResource>(), std::string("/test"));
        m_devtoken = makeTestDevice();
    }

    void TearDown() override {
        s_idDeviceMap.clear();
    }
    std::unique_ptr<HttpResource> m_resource;
    std::string m_devtoken;
};

class DeviceResourcesTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_resource = std::make_unique<HttpResource>(std::make_shared<DeviceResource>(),
                                                    std::string("/test"));
        m_devtoken = makeTestDevice();
    }

    void TearDown() override {
        s_idDeviceMap.clear();
    }
    std::unique_ptr<HttpResource> m_resource;
    std::string m_devtoken;
};

TEST_F(PinsResourcesTest, RenderPOST_CreatePin_Success) {
    HttpRequest req;
    req.setArg("pinNumber", "1");
    req.setArg("dataType", PINTYPE);
    req.setArg("token", m_devtoken);
    req.setArg("defValue", PINDEFVAL);
    req.setArg("cmd", "cp");

    auto response = m_resource->getHandler()->renderPOST(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::CREATED);
    ASSERT_EQ(response->getBody(), "Pin created!");
}

TEST_F(PinsResourcesTest, RenderPOST_CreatePin_Failure) {
    HttpRequest req;
    req.setArg("pinNumber", "1");
    req.setArg("dataType", PINTYPE);
    std::string invalid_token = m_devtoken;
    invalid_token.back() = 'i';
    req.setArg("token", invalid_token);
    req.setArg("defValue", PINDEFVAL);
    req.setArg("cmd", "cp");

    auto response = m_resource->getHandler()->renderPOST(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::UNAUTHORIZED);
    ASSERT_EQ(response->getBody(), "Auth failure!");
}

TEST_F(PinsResourcesTest, RenderGET_GetPin_Success) {
    HttpRequest req;
    req.setArg("pinNumber", PINNUM);
    req.setArg("token", m_devtoken);
    req.setArg("cmd", "pv");

    auto response = m_resource->getHandler()->renderGET(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::OK);
    std::string testResponse("PinValue=");
    testResponse += PINDEFVAL;
    ASSERT_EQ(response->getBody(), testResponse);
}

TEST_F(PinsResourcesTest, RenderGET_GetPin_Failure) {
    HttpRequest req;
    req.setArg("pinNumber", PINNUM);
    std::string invalid_token = m_devtoken;
    invalid_token.back() = 'i';
    req.setArg("token", invalid_token);
    req.setArg("cmd", "pv");

    auto response = m_resource->getHandler()->renderGET(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::UNAUTHORIZED);
    ASSERT_EQ(response->getBody(), "Auth failure!");
}

TEST_F(PinsResourcesTest, RenderPUT_UpdatePin_Success) {
    HttpRequest req;
    req.setArg("pinNumber", PINNUM);
    req.setArg("token", m_devtoken);
    req.setArg("value", PINDEFVAL);
    req.setArg("cmd", "up");

    auto response = m_resource->getHandler()->renderPUT(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::OK);
    ASSERT_EQ(response->getBody(), "Pin changed");
}

TEST_F(PinsResourcesTest, RenderPUT_UpdatePin_Failure) {
    HttpRequest req;
    req.setArg("pinNumber", PINNUM);
    std::string invalid_token = m_devtoken;
    invalid_token.back() = 'i';
    req.setArg("token", invalid_token);
    req.setArg("value", PINDEFVAL);
    req.setArg("cmd", "up");

    auto response = m_resource->getHandler()->renderPUT(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::UNAUTHORIZED);
    ASSERT_EQ(response->getBody(), "Auth failure!");
}

TEST_F(PinsResourcesTest, RenderDELETE_DeletePin_Success) {
    HttpRequest req;
    req.setArg("pinNumber", PINNUM);
    req.setArg("token", m_devtoken);
    req.setArg("cmd", "dp");

    auto response = m_resource->getHandler()->renderDELETE(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::OK);
    ASSERT_EQ(response->getBody(), "Pin deleted");
}

TEST_F(PinsResourcesTest, RenderDELETE_DeletePin_Failure) {
    HttpRequest req;
    req.setArg("pinNumber", PINNUM);
    std::string invalid_token = m_devtoken;
    invalid_token.back() = 'i';
    req.setArg("token", invalid_token);
    req.setArg("cmd", "dp");

    auto response = m_resource->getHandler()->renderDELETE(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::UNAUTHORIZED);
    ASSERT_EQ(response->getBody(), "Auth failure!");
}

TEST_F(DeviceResourcesTest, RenderPOST_RegisterDevice_Success) {
    HttpRequest req;
    req.setArg("cmd", "rd");
    s_idDeviceMap.clear();
    auto response = m_resource->getHandler()->renderPOST(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::CREATED);
    std::string testReponse = "token=";
    testReponse += s_idDeviceMap.begin()->second->getToken();
    ASSERT_EQ(response->getBody(), testReponse);
}

TEST_F(DeviceResourcesTest, RenderPOST_RegisterDevice_Failure) {
    HttpRequest req;
    req.setArg("cmd", "INVALID_COMMAND");

    auto response = m_resource->getHandler()->renderPOST(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::BAD_REQUEST);
    ASSERT_EQ(response->getBody(), "Wrong command!");
}

TEST_F(DeviceResourcesTest, RenderGET_DeviceStatus_Success) {
    HttpRequest req;
    req.setArg("cmd", "ds");
    req.setArg("token", m_devtoken);

    auto response = m_resource->getHandler()->renderGET(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::OK);
    ASSERT_EQ(response->getBody(), "devStatus=1");
}

TEST_F(DeviceResourcesTest, RenderGET_DeviceStatus_Failure) {
    HttpRequest req;
    req.setArg("cmd", "ds");
    std::string invalid_token = m_devtoken;
    invalid_token.back() = 'i';
    req.setArg("token", invalid_token);

    auto response = m_resource->getHandler()->renderGET(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::UNAUTHORIZED);
    ASSERT_EQ(response->getBody(), "Auth failure!");
}

TEST_F(DeviceResourcesTest, RenderPUT_DeviceStatusUpdate_Success) {
    HttpRequest req;
    req.setArg("cmd", "us");
    req.setArg("token", m_devtoken);

    auto response = m_resource->getHandler()->renderPUT(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::OK);
    ASSERT_EQ(response->getBody(), "Device status updated!");
}

TEST_F(DeviceResourcesTest, RenderPUT_DeviceStatusUpdate_Failure) {
    HttpRequest req;
    req.setArg("cmd", "us");
    std::string invalid_token = m_devtoken;
    invalid_token.back() = 'i';
    req.setArg("token", invalid_token);

    auto response = m_resource->getHandler()->renderPUT(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::UNAUTHORIZED);
    ASSERT_EQ(response->getBody(), "Auth failure!");
}

TEST_F(DeviceResourcesTest, RenderDELETE_DeleteDevice_Success) {
    HttpRequest req;
    req.setArg("cmd", "dd");
    req.setArg("token", m_devtoken);

    auto response = m_resource->getHandler()->renderDELETE(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::OK);
    ASSERT_EQ(response->getBody(), "Device deleted!");
}

TEST_F(DeviceResourcesTest, RenderDELETE_DeleteDevice_Failure) {
    HttpRequest req;
    req.setArg("cmd", "dd");
    std::string invalid_token = m_devtoken;
    invalid_token.back() = 'i';
    req.setArg("token", invalid_token);

    auto response = m_resource->getHandler()->renderDELETE(req);
    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::UNAUTHORIZED);
    ASSERT_EQ(response->getBody(), "Auth failure!");
}