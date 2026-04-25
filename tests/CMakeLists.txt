include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

function(add_ioteye_test TEST_SOURCE)
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})

    target_link_libraries(${TEST_NAME}
        PRIVATE
        ioteye::common
        ioteye::utils
        ioteye::device
        ioteye::persistence
        ioteye::server
        GTest::gtest_main
        jwt-cpp::jwt-cpp
        nlohmann_json::nlohmann_json
        Threads::Threads
        asio::asio
        ${Boost_LIBRARIES}
    )

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_ioteye_test(device_test.cpp)
add_ioteye_test(file_manager_test.cpp)
add_ioteye_test(functional_test.cpp)
add_ioteye_test(serverResources_test.cpp)
