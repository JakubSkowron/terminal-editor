# terminal-editor-library target

set(LIB_NAME terminal-editor-library)

set(LIB_SOURCES
    text_buffer.h
    text_buffer.cpp

    text_parser.h
    text_parser.cpp

    text_renderer.h
    text_renderer.cpp

    editor_config.h
    editor_config.cpp

    zwcwidth.h
    )

# @todo Using mk_wcwidth under Linux is a temporary workaround.
#if(WIN32)
    list(APPEND LIB_SOURCES "${CMAKE_SOURCE_DIR}/third_party/mk_wcswidth/mk_wcswidth/mk_wcwidth.h")
    list(APPEND LIB_SOURCES "${CMAKE_SOURCE_DIR}/third_party/mk_wcswidth/mk_wcswidth/mk_wcwidth.cpp")
#endif()

add_library(${LIB_NAME} ${LIB_SOURCES})
#set_target_properties(${LIB_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third_party/gsl-2.0.0")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third_party/mk_wcswidth")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third_party/tl")
target_include_directories(${LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/third_party/nlohmann-json-3.4.0")
target_link_libraries(${LIB_NAME} PUBLIC utilities)
SetCompilerOptions(${LIB_NAME})
