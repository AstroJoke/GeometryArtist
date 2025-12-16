cmake_minimum_required(VERSION 3.20)

project(ga_scene)

file(GLOB_RECURSE SCENE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

file(GLOB_RECURSE SCENE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

file(GLOB_RECURSE SCENE_UI_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui"
)

add_library(${PROJECT_NAME}
    ${SCENE_SOURCES}
    ${SCENE_HEADERS}
    ${SCENE_UI_FILES}
    # *.qrc files can be added here if any
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Xml
        Qt6::Gui
        Qt6::PrintSupport
        Qt6::OpenGL
        Qt6::OpenGLWidgets
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_autogen/include_$<CONFIG>"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    DEBUG_POSTFIX "_d"
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

if(CMAKE_GENERATOR MATCHES "Visual Studio")

    source_group("Source Files" FILES ${SCENE_SOURCES})
    source_group("Header Files" FILES ${SCENE_HEADERS})
    source_group("Form Files" FILES ${SCENE_UI_FILES})

    source_group("Generated Files" REGULAR_EXPRESSION ".*autogen.*")
    source_group("Generated Files\\moc" REGULAR_EXPRESSION ".*[\\/]+moc_.*\\.(cpp|cxx)$")
    source_group("Generated Files\\ui" REGULAR_EXPRESSION ".*[\\/]+ui_.*\\.(h|hpp)$")
    source_group("Generated Files\\qrc" REGULAR_EXPRESSION ".*[\\/]+qrc_.*\\.(cpp|cxx)$")
    
endif()