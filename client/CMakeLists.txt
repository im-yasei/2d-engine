add_executable(2d-engine
  src/main.cpp
  src/physics.cpp
  src/client-server.cpp
  ../jsoncpp_amalgamated/jsoncpp.cpp
)

target_include_directories(2d-engine PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/client/include
    ${CMAKE_SOURCE_DIR}/jsoncpp_amalgamated
    ${IMGUI_DIR}
)

target_link_libraries(2d-engine
  PRIVATE
  engine
)
