cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(TerminalEditor LANGUAGES CXX)
include(CMakeDependentOption)
enable_testing()

# Address and thread sanitizers can't be used together.
option(ENABLE_ADDRESS_SANITIZER "Enables address sanitizer")
cmake_dependent_option(ENABLE_THREAD_SANITIZER "Enables thread sanitizer (can't be used with address sanitizer)" OFF "NOT ENABLE_ADDRESS_SANITIZER" OFF)
option(ENABLE_UB_SANITIZER "Enables undefined behaviour sanitizer (also in release, with minimal runtime)")

if(MSVC)
else()
    if(ENABLE_ADDRESS_SANITIZER)
        # Sometimes doesn't work because -lasan is not linked.
        message(STATUS "Address sanitizer enabled.")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -O2")
        set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    endif()

    if(ENABLE_THREAD_SANITIZER)
        message(STATUS "Thread sanitizer enabled.")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=thread")
        set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=thread")
    endif()

    if(ENABLE_UB_SANITIZER)
        # Doesn't work on gcc 7 on Ubuntu 16.04 because there is a bug in gcc.
        message(STATUS "UB sanitizer enabled.")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=undefined")
        set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=undefined")

        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fsanitize=undefined -fsanitize-minimal-runtime")
        set(CMAKE_LINKER_FLAGS_RELEASE "${CMAKE_LINKER_FLAGS_RELEASE} -fsanitize=undefined -fsanitize-minimal-runtime")

        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fsanitize=undefined -fsanitize-minimal-runtime")
        set(CMAKE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_LINKER_FLAGS_RELWITHDEBINFO} -fsanitize=undefined -fsanitize-minimal-runtime")
    endif()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(USING_CLANG ON)
    message(STATUS "Compiling using Clang.")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
else()
    set(USING_CLANG OFF)
endif()

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
            #-Wmisleading-indentation    # (not supported on old gcc) warn if indentation implies blocks where blocks do not exist
            #-Wduplicated-cond           # (not supported on old gcc) warn if if / else chain has duplicated conditions
            #-Wduplicated-branches       # (not supported on old gcc) warn if if / else branches have duplicated code
            #-Wnull-dereference          # (not supported on old gcc) warn if a null dereference is detected
            -Wdouble-promotion          # warn if float is implicit promoted to double
            -Wformat=2                  # warn on security issues around functions that format output (ie printf)
            #-Wno-variadic-macros		# Don't complain about non-standard use of macros.
        )
        if (USING_CLANG)
            list(APPEND optionsList
                #-Wlifetime                  # (clang only) shows object lifetime issues
                -Wno-sign-conversion        # don't warn on sign conversions (this is very noisy)

                -stdlib=libc++              # Otherwise <variant> header is not being found. Also libc++ package needs to be installed on Ubuntu.
            )
            #target_link_options(${targetName} PRIVATE -stdlib=libc++)  # target_link_options is available since CMake 3.14 I think.
            target_link_libraries(${targetName} PRIVATE c++abi)
            target_link_libraries(${targetName} PRIVATE atomic)
        else()
            list(APPEND optionsList
                -Wlogical-op                # (gcc only) warn about logical operations being used where bitwise were probably wanted
                -Wuseless-cast              # (gcc only) warn if you perform a cast to the same type
            )
        endif()
    endif()
    target_compile_options(${targetName} PRIVATE ${optionsList})

    # As this is a public compile feature anything that links to target will also build with -std=c++17
    target_compile_features(${targetName} PUBLIC cxx_std_17)

    if(MSVC)
        # Get rid of annoying compiler warnings.
        target_compile_definitions(${targetName} PUBLIC _CRT_SECURE_NO_WARNINGS)
    
        # Get rid of some horrible crap in windows.h
        target_compile_definitions(${targetName} PUBLIC NOMINMAX NOGDI WIN32_LEAN_AND_MEAN VC_EXTRALEAN)
    
        # Build with unicode support
        target_compile_definitions(${targetName} PUBLIC UNICODE _UNICODE)
    endif()
endfunction()

add_subdirectory(utilities)
add_subdirectory(editorlib)
add_subdirectory(text_ui)
add_subdirectory(editor)
add_subdirectory(tests)

add_custom_target(LOOSE_FILES
                  echo "Nothing to build for LOOSE_FILES."
                  SOURCES
                    LICENSE
                    README.md
                    .travis.yml
                    appveyor.yml
                    editor-config.json
                    terminal_test_positions.sh
                    format_source.sh
                    .clang-format
                  )

if(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT terminal-editor)
endif()
