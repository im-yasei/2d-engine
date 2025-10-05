add_library(engine STATIC
    src/engine.cpp
)
target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(engine PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
)
