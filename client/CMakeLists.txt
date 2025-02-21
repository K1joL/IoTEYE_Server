cmake_minimum_required(VERSION 3.18)

project(ioteyeClient VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Conditionally add logging definitions
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Logging enabled!")
    add_compile_definitions(IoTeyeDEBUG)
endif()

include_directories("include/")

set(SRC_LIST
    src/client.cpp
)

# --- Find Dependencies ---
find_package(asio REQUIRED)

add_executable(${PROJECT_NAME} client_main.cpp ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE ioteyeserver asio::asio)
