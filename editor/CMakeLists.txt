# terminal-editor target

set(APP_NAME terminal-editor)

set(APP_SOURCES
    main.cpp
    )

add_executable(${APP_NAME} ${APP_SOURCES})
target_link_libraries(${APP_NAME} PRIVATE terminal-editor-library text_ui Threads::Threads)
SetCompilerOptions(${APP_NAME})

if(WIN32)
    #set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "ala")
    set_target_properties(${APP_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
