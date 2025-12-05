cmake_minimum_required(VERSION 3.10)
project(2d-engine)

# find_package(PkgConfig REQUIRED)
# pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# find_package(jsoncpp REQUIRED)
add_subdirectory(SFML-2.6.2)
add_subdirectory(engine)
add_subdirectory(client)
add_subdirectory(tests)
# add_subdirectory(interface)
