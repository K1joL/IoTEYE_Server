cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

project (main VERSION 1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23 -Wall")

include_directories("Headers/")

set(SRC_LIST 
	Sources/functional.cpp 
	Sources/user.cpp 
	Sources/serverResources.cpp 
	Sources/device.cpp)
set(HEAD_LIST 
	Headers/functional.h 
	Headers/user.h 
	Headers/serverResources.h 
	Headers/device.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_HTTPSERVER REQUIRED IMPORTED_TARGET
				  libhttpserver>=0.19)

                  
add_executable(main main.cpp ${SRC_LIST})
target_link_libraries(main PRIVATE PkgConfig::PC_HTTPSERVER)