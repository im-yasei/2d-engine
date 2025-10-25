add_executable(2d-engine
    src/main.cpp
  ../jsoncpp_amalgamated/jsoncpp.cpp
)

target_include_directories(2d-engine PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/jsoncpp_amalgamated
)

target_link_libraries(2d-engine
  PRIVATE
  engine
)
