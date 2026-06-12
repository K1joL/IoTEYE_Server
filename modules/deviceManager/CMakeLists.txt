add_library(ioteye_device_manager STATIC
    src/deviceManager.cpp
)

target_include_directories(ioteye_device_manager
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ioteye_device_manager
    PUBLIC
    ioteye::utils
    ioteye::common
    ioteye::device
    ioteye::persistence
    Threads::Threads
)

add_library(ioteye::device_manager ALIAS ioteye_device_manager)
