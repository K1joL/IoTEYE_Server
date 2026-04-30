add_library(ioteye_utils STATIC
    src/processorPool.cpp
    src/timerScheduler.cpp
)

target_include_directories(ioteye_utils
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ioteye_utils
    PUBLIC
    ioteye::common
    Threads::Threads
)

add_library(ioteye::utils ALIAS ioteye_utils)
