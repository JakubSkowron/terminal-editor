cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(TerminalEditor LANGUAGES CXX)
enable_testing()

function(SetCompilerOptions targetName)
    # Recommended warnings mostly taken from https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md
    if(MSVC)
        set(optionsList
            /permissive-    # Enforces standards conformance.
            /Zc:__cplusplus # Set proper value of __cplusplus macro.

            #/Wx     # Treat warnings as errors
            /W4     # All reasonable warnings
            /w14242 # 'identfier': conversion from 'type1' to 'type1', possible loss of data
            /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
            /w14263 # 'function': member function does not override any base class virtual member function
            /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
            /w14287 # 'operator': unsigned/negative constant mismatch
            /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
            /w14296 # 'operator': expression is always 'boolean_value'
            /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
            /w14545 # expression before comma evaluates to a function which is missing an argument list
            /w14546 # function call before comma missing argument list
            /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
            /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
            /w14555 # expression has no effect; expected expression with side-effect
            /w14619 # pragma warning: there is no warning number 'number'
            /w14640 # Enable warning on thread un-safe static member initialization
            /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
            /w14905 # wide string literal cast to 'LPSTR'
            /w14906 # string literal cast to 'LPWSTR'
            /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        )
    else()
        set(optionsList
            #-Werror                     # Treat warnings as errors
            -Wall -Wextra               # reasonable and standard
            #-Wshadow                    # warn the user if a variable declaration shadows one from a parent context
            -Wnon-virtual-dtor          # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
            -Wold-style-cast            # warn for c-style casts
            -Wcast-align                # warn for potential performance problem casts
            -Wunused                    # warn on anything being unused
            -Woverloaded-virtual        # warn if you overload (not override) a virtual function
            #-Wpedantic                  # warn if non-standard C++ is used
            -Wconversion                # warn on type conversions that may lose data
            #-Wsign-conversion           # warn on sign conversions
            #-Wmisleading-indentation    # (not supported on old gcc) warn if indentation implies blocks where blocks do not exist
            #-Wduplicated-cond           # (not supported on old gcc) warn if if / else chain has duplicated conditions
            #-Wduplicated-branches       # (not supported on old gcc) warn if if / else branches have duplicated code
            -Wlogical-op                # warn about logical operations being used where bitwise were probably wanted
            #-Wnull-dereference          # (not supported on old gcc) warn if a null dereference is detected
            -Wuseless-cast              # warn if you perform a cast to the same type
            -Wdouble-promotion          # warn if float is implicit promoted to double
            -Wformat=2                  # warn on security issues around functions that format output (ie printf)
            #-Wlifetime                  # (clang only currently) shows object lifetime issues
            #-Wno-variadic-macros		# Don't complain about non-standard use of macros.
        )
    endif()
    target_compile_options(${targetName} PRIVATE ${optionsList})

    # As this is a public compile feature anything that links to target will also build with -std=c++14
    target_compile_features(${targetName} PUBLIC cxx_std_14)

    if(MSVC)
        # Get rid of annoying compiler warnings.
        target_compile_definitions(${targetName} PUBLIC _CRT_SECURE_NO_WARNINGS)
    endif()
endfunction()

# terminal-editor-library target

set(LIB_NAME terminal-editor-library)

set(LIB_SOURCES
    terminal_size.h
    format_source.sh

    text_buffer.h
    text_buffer.cpp

    text_renderer.h
    text_renderer.cpp

    editor_config.h
    editor_config.cpp

    file_utilities.h
    file_utilities.cpp

    zpreprocessor.h
    zerrors.h
    zlogging.h
    zwcwidth.h
    )

if(WIN32)
    list(APPEND LIB_SOURCES "${CMAKE_SOURCE_DIR}/third_party/mk_wcswidth/mk_wcswidth/mk_wcwidth.h")
    list(APPEND LIB_SOURCES "${CMAKE_SOURCE_DIR}/third_party/mk_wcswidth/mk_wcswidth/mk_wcwidth.cpp")
else()
    list(APPEND LIB_SOURCES terminal_size.cpp)
endif()

add_library(${LIB_NAME} ${LIB_SOURCES})
#set_target_properties(${LIB_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third_party/gsl-2.0.0")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/third_party/mk_wcswidth")
target_include_directories(${LIB_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/third_party/nlohmann-json-3.4.0")
SetCompilerOptions(${LIB_NAME})

# terminal-editor target

set(APP_NAME terminal-editor)

set(APP_SOURCES
    main.cpp

    README.md
    )

add_executable(${APP_NAME} ${APP_SOURCES})
target_link_libraries(${APP_NAME} PRIVATE ${LIB_NAME})
SetCompilerOptions(${APP_NAME})

# Tests

add_subdirectory(tests)

if(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT editor-tests)
endif()
