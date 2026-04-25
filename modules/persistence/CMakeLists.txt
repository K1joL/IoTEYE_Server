add_library(ioteye_persistence STATIC
    src/fileManager.cpp
)

target_include_directories(ioteye_persistence
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ioteye_persistence
    PUBLIC
    ioteye::common
    ioteye::device
    PRIVATE
    nlohmann_json::nlohmann_json
)

add_library(ioteye::persistence ALIAS ioteye_persistence)
