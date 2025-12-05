enable_testing()

add_executable(2d-engine-tests
    test.cpp
    test_physics.cpp
    test_json.cpp
    ../engine/src/engine.cpp
    ../client/src/physics.cpp
    ../jsoncpp_amalgamated/jsoncpp.cpp
)

target_include_directories(2d-engine-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/client/include
    ${CMAKE_SOURCE_DIR}/jsoncpp_amalgamated
)

target_link_libraries(
    2d-engine-tests 
    gtest 
    gtest_main 
    gmock 
    pthread
    sfml-graphics
    sfml-window
    sfml-system
    )

add_test(NAME Tests COMMAND 2d-engine-tests)
