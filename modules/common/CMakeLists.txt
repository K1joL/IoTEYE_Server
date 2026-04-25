add_library(ioteye_common STATIC
    src/adminOptions.cpp
    src/commands.cpp
)

target_include_directories(ioteye_common
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(ioteye_common
    PUBLIC
    Boost::program_options
)

target_compile_definitions(ioteye_common
    PUBLIC
    $<$<CONFIG:Debug>:ENABLE_LOGGING>
)

add_library(ioteye::common ALIAS ioteye_common)
