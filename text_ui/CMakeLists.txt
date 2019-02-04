# text_ui target

set(LIB_NAME text_ui)

set(LIB_SOURCES
    screen_functions.h
    screen_functions.cpp

    terminal_size.h
    terminal_size.cpp

    terminal_io.h
    terminal_io.cpp
    )

add_library(${LIB_NAME} ${LIB_SOURCES})
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third_party/tl")
target_link_libraries(${LIB_NAME} PRIVATE terminal-editor-library)
SetCompilerOptions(${LIB_NAME})

