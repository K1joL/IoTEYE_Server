cmake_minimum_required (VERSION 3.18)

project (main VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

include_directories("Headers/")

set(SRC_LIST 
	Sources/functional.cpp 
	# Sources/user.cpp 
	Sources/serverResources.cpp 
	Sources/device.cpp)
set(HEAD_LIST 
	Headers/adminOptions.h
	Headers/functional.h 
	# Headers/user.h 
	Headers/serverResources.h 
	Headers/device.h)

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

add_compile_definitions(IoTeyeDEBUG)

add_executable(main main.cpp ${SRC_LIST})
target_link_libraries(main PRIVATE ioteyeserver jwt-cpp::jwt-cpp Threads::Threads asio::asio)
