# text_ui target

set(LIB_NAME text_ui)

set(LIB_SOURCES
    screen_functions.h
    screen_functions.cpp

    terminal_size.h
    terminal_size.cpp

    console_reader.h
    console_reader.cpp
    
    terminal_io.h
    terminal_io.cpp
    
    screen_buffer.h
    screen_buffer.cpp

    window.h
    window.cpp

    editor_window.h
    editor_window.cpp
    )

add_library(${LIB_NAME} ${LIB_SOURCES})
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third_party/tl")
target_link_libraries(${LIB_NAME} PUBLIC utilities terminal-editor-library)
SetCompilerOptions(${LIB_NAME})

