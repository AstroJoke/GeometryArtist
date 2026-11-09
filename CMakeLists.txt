cmake_minimum_required(VERSION 3.20)

project(GeometryArtist 
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Geometry Artist - A geometry visualization tool")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GA_BUILD_APP "Build the existing Qt application" ON)
option(GA_BUILD_FILE_IO "Build file services and codec plugins" ON)

# File IO requires Qt Core for dynamic loading and transactional file output.
if(GA_BUILD_FILE_IO AND NOT GA_BUILD_APP)
    find_package(Qt6 REQUIRED COMPONENTS Core)
endif()

# Disable both optional subsystems for a Qt-free core build.
if(GA_BUILD_APP)
    # Qt 6 settings
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    # find Qt 6
    find_package(Qt6 REQUIRED COMPONENTS
        Core
        Xml
        Gui
        Widgets
        OpenGL
        PrintSupport
        OpenGLWidgets
    )

endif()

if(MSVC)
    add_compile_options(/MP /wd4819 /permissive-)
    
    set(CMAKE_EXE_LINKER_FLAGS 
        "${CMAKE_EXE_LINKER_FLAGS} /MACHINE:X64 /NODEFAULTLIB:LIBCMT.lib"
    )

    set(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SUBSYSTEM:CONSOLE"
    )

    add_compile_definitions(
        _WIN32
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
    )
    
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)
include(CTest)

# Foundational targets have no GUI or geometry-kernel dependency.
add_subdirectory(src/foundation)
add_subdirectory(src/data)
add_subdirectory(src/runtime)
if(GA_BUILD_FILE_IO)
    add_subdirectory(src/adapters/io)
endif()
if(BUILD_TESTING)
    add_subdirectory(tests/core_base)
endif()

if(GA_BUILD_APP)
    add_subdirectory(src/scene_core/ga_scene)
    add_subdirectory(src/app)
endif()

if(GA_BUILD_APP AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GeometryArtist_Main)
endif()
