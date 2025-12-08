cmake_minimum_required(VERSION 3.20)

project(GeometryArtist_Main)

set(CMAKE_AUTOMOC ON)

file(GLOB APP_SOURCES 
    "src/*.cpp"
)

file(GLOB APP_HEADERS 
    "include/*.h"
)

file(GLOB APP_UI_FILES 
    "form/*.ui"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(GeometryArtist_Main
    main.cpp
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${APP_UI_FILES}
)

set_target_properties(GeometryArtist_Main PROPERTIES
    DEBUG_POSTFIX "_d"
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_include_directories(GeometryArtist_Main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(GeometryArtist_Main
        PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Xml
        Qt6::Gui
        Qt6::PrintSupport
        Qt6::OpenGL
        Qt6::OpenGLWidgets
)

if(CMAKE_GENERATOR MATCHES "Visual Studio")

    source_group("Source Files" FILES main.cpp)
    
    if(APP_SOURCES)
        source_group("Source Files\\core" FILES ${APP_SOURCES})
    endif()
    
    if(APP_HEADERS)
        source_group("Header Files" FILES ${APP_HEADERS})
    endif()
    
    if(APP_UI_FILES)
        source_group("Form Files" FILES ${APP_UI_FILES})
    endif()
    
    source_group("Generated Files" REGULAR_EXPRESSION ".*autogen.*")
    source_group("Generated Files\\moc" REGULAR_EXPRESSION ".*/moc_.*\\.cpp$|.*/moc_.*\\.cxx$")
    source_group("Generated Files\\ui" REGULAR_EXPRESSION ".*/ui_.*\\.h$|.*/ui_.*\\.hpp$")
    source_group("Generated Files\\qrc" REGULAR_EXPRESSION ".*/qrc_.*\\.cpp$|.*/qrc_.*\\.cxx$")
    
    # 设置解决方案文件夹
    set_target_properties(GeometryArtist_Main PROPERTIES
        FOLDER "Applications"
        VS_STARTUP_PROJECT "TRUE"
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(GeometryArtist_Main
        PRIVATE
        ENABLE_OPENGL_DEBUG = 1)
else()
    target_compile_definitions(GeometryArtist_Main
        PRIVATE
        ENABLE_OPENGL_DEBUG = 0)
endif() 
