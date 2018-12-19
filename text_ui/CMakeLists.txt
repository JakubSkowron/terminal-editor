# text_ui target

set(LIB_TEXT_UI text_ui)

set(LIB_TEXT_UI_SOURCES
    screen_functions.h
    screen_functions.cpp

    terminal_size.h
    terminal_size.cpp

    terminal_io.h
    terminal_io.cpp
    )

add_library(${LIB_TEXT_UI} ${LIB_TEXT_UI_SOURCES})
SetCompilerOptions(${LIB_TEXT_UI})

