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
    src/device.cpp)

# --- Find Dependencies ---
find_package(asio REQUIRED)
find_package(ioteyeserver REQUIRED)
find_package(Threads REQUIRED)

# --- Fetch jwt-cpp ---
include(FetchContent)
fetchcontent_declare(jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG 08bcf77a687fb06e34138e9e9fa12a4ecbe12332 # v0.7.0 release
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "disable building examples" FORCE)
fetchcontent_makeavailable(jwt-cpp)
# --- End Fetch jwt-cpp ---

add_executable(main main.cpp ${SRC_LIST})
target_link_libraries(main PRIVATE ioteyeserver jwt-cpp::jwt-cpp Threads::Threads asio::asio)

# Conditional inclusion of client project
if(IOTEYE_BUILD_CLIENT)
    add_subdirectory(client)
endif()
