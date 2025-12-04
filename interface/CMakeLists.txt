add_library(interface STATIC
    src/interface.cpp
)

target_include_directories(interface PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(interface PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
)
