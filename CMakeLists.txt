cmake_minimum_required(VERSION 3.2)
project(2d-engine)

find_package(SFML 2.6.1 COMPONENTS graphics window system REQUIRED)

add_subdirectory(client)
add_subdirectory(engine)
