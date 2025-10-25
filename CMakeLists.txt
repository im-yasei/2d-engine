cmake_minimum_required(VERSION 4.1)
project(2d-engine)

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# find_package(jsoncpp REQUIRED)

add_subdirectory(SFML-2.6.2)
add_subdirectory(jsoncpp)
add_subdirectory(engine)
add_subdirectory(client)