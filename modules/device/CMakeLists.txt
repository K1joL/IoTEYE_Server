add_library(ioteye_device STATIC
    src/device.cpp
)

target_include_directories(ioteye_device
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ioteye_device
    PUBLIC
    ioteye::common
    ioteye::utils
    jwt-cpp::jwt-cpp
)

add_library(ioteye::device ALIAS ioteye_device)
