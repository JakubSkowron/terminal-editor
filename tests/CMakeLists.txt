# terminal-editor tests

set(APP_NAME editor-tests)

set(APP_SOURCES
    main-tests.cpp
    textbuffer-tests.cpp
    zerrors-tests.cpp
    )

add_executable(${APP_NAME} ${APP_SOURCES})
SetCompilerOptions(${APP_NAME})

target_link_libraries(${APP_NAME} PRIVATE terminal-editor-library)
target_include_directories(${APP_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/third_party/catch2-2.5.0")

#find_package(Catch2 REQUIRED)
#target_link_libraries(${APP_NAME} Catch2::Catch2)
#include(CTest)
#include(Catch)
#catch_discover_tests(${APP_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    #set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-e pgpu -c TestData1.5/basic/config.json")
    set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
