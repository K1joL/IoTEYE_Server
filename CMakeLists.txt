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
project(ioteyeServer VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Conditional logging definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Logging enabled!")
    add_compile_definitions(ENABLE_LOGGING)
endif()

option(IOTEYE_BUILD_CLIENT "Build the client application" OFF)
option(IOTEYE_SERVER_ENABLE_TESTS "Enable tests compilation" OFF)

# --- External Dependencies ---
include(FetchContent)

# jwt-cpp
FetchContent_Declare(jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG 08bcf77a687fb06e34138e9e9fa12a4ecbe12332
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "disable building examples" FORCE)
FetchContent_MakeAvailable(jwt-cpp)

# System dependencies
find_package(asio REQUIRED)
find_package(ioteyeserver REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost 1.88.0 REQUIRED COMPONENTS program_options)
find_package(nlohmann_json REQUIRED)

# Global include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})

# --- Module Subdirectories ---
add_subdirectory(modules/common)
add_subdirectory(modules/utils)
add_subdirectory(modules/device)
add_subdirectory(modules/persistence)
# add_subdirectory(modules/device_manager)
add_subdirectory(modules/server)
add_subdirectory(app)
# --- Main Executable ---
# add_executable(main app/app.cpp)
# target_link_libraries(main PRIVATE
#     ioteye::common
#     ioteye::utils
#     ioteye::device
#     ioteye::persistence
#     # ioteye::device_manager
#     ioteye::server
#     ioteyeserver
#     asio::asio
#     Threads::Threads
#     ${Boost_LIBRARIES}
# )

# --- Tests ---
if(IOTEYE_SERVER_ENABLE_TESTS)
    if(MSVC)
        message(WARNING "Tests cannot be built with MSVC!")
    else()
        message(STATUS "Tests compilation is ON!")
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()
