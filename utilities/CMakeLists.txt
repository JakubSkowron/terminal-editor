# utilities target

set(LIB_NAME utilities)

set(LIB_SOURCES
    zerrors.h
    zlogging.h
    zpreprocessor.h
    zstr.h
    file_utilities.h
    file_utilities.cpp

    NatvisFile.natvis
    )

add_library(${LIB_NAME} ${LIB_SOURCES})
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
SetCompilerOptions(${LIB_NAME})
