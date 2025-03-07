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

include_directories("include/")

set(SRC_LIST
    src/functional.cpp
    src/serverResources.cpp
    src/device.cpp
    src/file_handler.cpp)
    
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
target_link_libraries(main PRIVATE ioteyeserver jwt-cpp::jwt-cpp nlohmann_json::nlohmann_json Threads::Threads asio::asio)

# Conditional inclusion of client project
if(IOTEYE_BUILD_CLIENT)
    add_subdirectory(client)
endif()
