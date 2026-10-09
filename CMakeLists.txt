cmake_minimum_required(VERSION 3.20)

project(CleanUpService
    VERSION 1.0.0
    DESCRIPTION "Windows Service that deletes temporary files and executes disk cleanup on boot"
    LANGUAGES CXX
)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform validation
if(NOT WIN32)
    message(FATAL_ERROR "CleanUpService is a Windows Service and must be built on Windows.")
endif()

# Compiler flags
if(MSVC)
    add_compile_options(
        /W4          # High warning level
        /permissive- # Standards conformance
        /utf-8       # UTF-8 source and execution character sets
        /EHsc        # Standard C++ exception handling
    )
    add_compile_definitions(
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Project sources and headers
set(SOURCES
    src/main.cpp
    src/service.cpp
    src/cleanup.cpp
)

set(HEADERS
    include/service.hpp
    include/cleanup.hpp
)

add_executable(CleanUpService
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(CleanUpService PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Win32 system libraries:
# - Advapi32: Service Control Manager (SCM), registry, event logging
# - Shell32: Shell API (SHGetKnownFolderPath, SHEmptyRecycleBin)
# - Ole32: COM initialization (CoInitializeEx, CoTaskMemFree)
# - User32: Win32 message queue and utilities
target_link_libraries(CleanUpService PRIVATE
    Advapi32
    Shell32
    Ole32
    User32
)

# Console subsystem allows CLI installation/debugging arguments (--install, --uninstall, --run-now)
# while seamlessly supporting Windows Service Control Manager invocation.
set_target_properties(CleanUpService PROPERTIES
    WIN32_EXECUTABLE FALSE
    OUTPUT_NAME "CleanUpService"
)
