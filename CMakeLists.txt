cmake_minimum_required(VERSION 4.1)
project(2d-engine)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-sfml/imconfig-SFML.h
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/imconfig.h
    COPYONLY
)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui CACHE PATH "Path to ImGui" FORCE)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "don't search for SFML" FORCE)

add_subdirectory(thirdparty/SFML-2.6.2)
add_subdirectory(engine)
add_subdirectory(client)
add_subdirectory(thirdparty/imgui-sfml)
