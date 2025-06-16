# MIT License

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

cmake_minimum_required(VERSION 3.18)

project(ioteyeServer VERSION 2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Conditionally add logging definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Logging enabled!")
    add_compile_definitions(ENABLE_LOGGING)
endif()

option(IOTEYE_BUILD_CLIENT "Build the client application" OFF)
option(IOTEYE_SERVER_ENABLE_TESTS "Enable tests compilation" OFF)

include_directories("include/")

set(SRC_LIST
    src/functional.cpp
    src/serverResources.cpp
    src/device.cpp
    src/file_manager.cpp)

# --- Fetch jwt-cpp ---
include(FetchContent)
fetchcontent_declare(jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG 08bcf77a687fb06e34138e9e9fa12a4ecbe12332 # v0.7.0 release
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "disable building examples" FORCE)
fetchcontent_makeavailable(jwt-cpp)
# --- End Fetch jwt-cpp ---

# --- Find Dependencies ---
find_package(asio REQUIRED)
find_package(ioteyeserver REQUIRED)
find_package(Threads REQUIRED)

add_executable(main main.cpp ${SRC_LIST})

set(LIB_LIST
    ioteyeserver
    jwt-cpp::jwt-cpp
    nlohmann_json::nlohmann_json
    Threads::Threads
    asio::asio
)

target_link_libraries(main PRIVATE ${LIB_LIST})

# Conditional inclusion of client project
if(IOTEYE_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(IOTEYE_SERVER_ENABLE_TESTS)

    if(MSVC)
        message(WARNING "Tests cannot be built with MSVC!")
    else()
        # --- Tests ---
        message(STATUS "Tests compilation is ON!")
        enable_testing()

        # Add GoogleTest (you might need to adjust the path based on your installation)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG v1.14.0 # Or the version you prefer
        )
        FetchContent_MakeAvailable(googletest)

        # Function to add a test
        function(add_test_executable TEST_FILE)
            get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
            add_executable(${TEST_NAME} ${TEST_FILE} ${ARGN})
            target_link_libraries(${TEST_NAME}
                ${LIB_LIST}
                GTest::gtest_main
            )
            add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        endfunction()

        # Add individual tests
        add_test_executable(tests/device_test.cpp src/device.cpp)
        add_test_executable(tests/file_manager_test.cpp
            src/device.cpp src/file_manager.cpp)
        add_test_executable(tests/functional_test.cpp src/functional.cpp)
        add_test_executable(tests/serverResources_test.cpp
            src/device.cpp src/functional.cpp src/serverResources.cpp)
    endif()
endif()
