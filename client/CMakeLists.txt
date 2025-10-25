add_executable(2d-engine
    src/main.cpp
)

target_include_directories(2d-engine PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/jsoncpp/include
)

target_link_libraries(2d-engine
  PRIVATE
 # ${JSONCPP_LIBRARIES}
  jsoncpp_static
  engine
)