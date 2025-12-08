cmake_minimum_required(VERSION 3.20)

project(GeometryArtist 
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Geometry Artist - A geometry visualization tool")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

if(MSVC)
    add_compile_options(/MP)
    add_compile_options(/wd4819)
    add_compile_options(/permissive-)
    
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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src/app)

#add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_LIST_DIR}/source/app/src/main.cpp")
#add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/source/app/)
