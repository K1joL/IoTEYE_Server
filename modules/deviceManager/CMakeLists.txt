add_library(ioteye_device_manager INTERFACE)

target_include_directories(ioteye_device_manager
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ioteye_device_manager
    INTERFACE
    ioteye::utils
    ioteye::common
    ioteye::device
    ioteye::persistence
    Threads::Threads
)

add_library(ioteye::device_manager ALIAS ioteye_device_manager)
