cmake_minimum_required(VERSION 3.20)

project(GeometryArtist_Main)

file(GLOB_RECURSE APP_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

file(GLOB_RECURSE APP_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

file(GLOB_RECURSE APP_UI_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui"
)

set(CMAKE_AUTOUIC_SEARCH_PATHS 
    "${CMAKE_CURRENT_SOURCE_DIR}/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/main_window"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/dialogs"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/widgets"
)

message("=== UI Configuration ===")
message(STATUS "CMAKE_AUTOUIC: ${CMAKE_AUTOUIC}")
message(STATUS "UI Files found: ${APP_UI_FILES}")

add_executable(GeometryArtist_Main
    main.cpp
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${APP_UI_FILES}
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

target_include_directories(GeometryArtist_Main
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${UI_GENERATED_DIR}"
        "${MOC_GENERATED_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/GeometryArtist_Main_autogen/include_$<CONFIG>"
)

set_target_properties(GeometryArtist_Main PROPERTIES
    DEBUG_POSTFIX "_d"
    AUTOUIC_OUTPUT_DIRECTORY "${UI_GENERATED_DIR}"
    AUTOMOC_OUTPUT_DIRECTORY "${MOC_GENERATED_DIR}"
    AUTORCC_OUTPUT_DIRECTORY "${GENERATED_DIR}/qrc"
)

target_compile_definitions(GeometryArtist_Main
    PRIVATE
    $<$<CONFIG:Debug>:ENABLE_OPENGL_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:ENABLE_OPENGL_DEBUG=0>
)

if(CMAKE_GENERATOR MATCHES "Visual Studio")

    source_group("Source Files" FILES main.cpp)
    
    if(APP_SOURCES)
        source_group("Source Files" FILES ${APP_SOURCES})
    endif()
    
    if(APP_HEADERS)
        source_group("Header Files" FILES ${APP_HEADERS})
    endif()
    
    if(APP_UI_FILES)
        source_group("Form Files" FILES ${APP_UI_FILES})
    endif()
    
    source_group("Generated Files" REGULAR_EXPRESSION ".*autogen.*")
    source_group("Generated Files\\moc" REGULAR_EXPRESSION ".*[\\/]+moc_.*\\.(cpp|cxx)$")
    source_group("Generated Files\\ui" REGULAR_EXPRESSION ".*[\\/]+ui_.*\\.(h|hpp)$")
    source_group("Generated Files\\qrc" REGULAR_EXPRESSION ".*[\\/]+qrc_.*\\.(cpp|cxx)$")
    
    set_target_properties(GeometryArtist_Main PROPERTIES
        FOLDER "Applications"
        VS_STARTUP_PROJECT GeometryArtist_Main
    )
endif()