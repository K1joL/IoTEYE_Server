add_library(ioteye_server STATIC
    src/serverResources.cpp
)

target_include_directories(ioteye_server
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ioteye_server
    PUBLIC
    ioteye::common
    ioteye::device
    # ioteye::device_manager
    ioteye::persistence
    PRIVATE
    ioteyeserver::ioteyeserver
    jwt-cpp::jwt-cpp
)

target_compile_definitions(ioteye_server
    PUBLIC
    $<$<CONFIG:Debug>:ENABLE_LOGGING>
)

add_library(ioteye::server ALIAS ioteye_server)
