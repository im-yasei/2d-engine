cmake_minimum_required(VERSION 4.1)
project(2d-engine)

find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
find_package(jsoncpp REQUIRED)

add_subdirectory(client)
add_subdirectory(engine)

