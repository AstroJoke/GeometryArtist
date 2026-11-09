project(gaFileIO LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaFileIOContracts INTERFACE)
add_library(GeometryArtist::FileIOContracts ALIAS gaFileIOContracts)
target_compile_features(gaFileIOContracts INTERFACE cxx_std_20)
target_include_directories(gaFileIOContracts INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(gaFileIOContracts INTERFACE GeometryArtist::DataCore)
target_sources(gaFileIOContracts INTERFACE ${GA_FILE_IO_CONTRACT_HEADERS})

add_library(gaFileIO SHARED
    ${GA_FILE_IO_HEADERS}
    ${GA_FILE_IO_SOURCES}
)
add_library(GeometryArtist::FileIO ALIAS gaFileIO)

target_compile_features(gaFileIO PUBLIC cxx_std_20)
target_include_directories(gaFileIO PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(gaFileIO
    PUBLIC
        GeometryArtist::FileIOContracts
        GeometryArtist::DataCore
        GeometryArtist::Foundation
    PRIVATE
        Qt6::Core
)

set_target_properties(gaFileIO PROPERTIES
    DEBUG_POSTFIX "_d"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${GA_FILE_IO_HEADERS}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PREFIX "Source Files"
    FILES ${GA_FILE_IO_SOURCES}
)

option(GA_FILE_IO_ENABLE_CGAL "Build the CGAL OBJ/OFF/PLY/STL plugin" ON)

add_subdirectory(plugins/common)
if(GA_FILE_IO_ENABLE_CGAL)
    add_subdirectory(plugins/mesh)
endif()

if(BUILD_TESTING)
    find_package(Threads REQUIRED)
    add_library(gaFileFailurePlugin MODULE ${GA_FILE_FAILURE_PLUGIN_SOURCES})
    target_link_libraries(gaFileFailurePlugin PRIVATE GeometryArtist::FileIO)
    add_executable(gaFilePluginLoaderTests ${GA_FILE_PLUGIN_LOADER_TEST_SOURCES})
    target_link_libraries(gaFilePluginLoaderTests PRIVATE GeometryArtist::FileIO Qt6::Core)
    add_dependencies(gaFilePluginLoaderTests gaFileFailurePlugin)
    add_test(NAME ga.io.plugin_loader COMMAND gaFilePluginLoaderTests $<TARGET_FILE:gaFileFailurePlugin>)
    set_tests_properties(ga.io.plugin_loader PROPERTIES TIMEOUT 20)

    add_executable(gaFileIOServiceTests ${GA_FILE_IO_SERVICE_TEST_SOURCES})
    target_link_libraries(gaFileIOServiceTests PRIVATE GeometryArtist::FileIO Threads::Threads)
    add_test(NAME ga.io.service COMMAND gaFileIOServiceTests)
    set_tests_properties(ga.io.service PROPERTIES TIMEOUT 30)

    add_executable(gaMeshFileCodecBridgeTests ${GA_MESH_FILE_BRIDGE_TEST_SOURCES})
    target_link_libraries(gaMeshFileCodecBridgeTests PRIVATE gaMeshFileCodecSupport Qt6::Core)
    add_test(NAME ga.io.mesh_bridge COMMAND gaMeshFileCodecBridgeTests)

    if(TARGET gaFileMeshCodecs)
        add_executable(gaMeshFileCodecTests ${GA_MESH_FILE_CODEC_TEST_SOURCES})
        target_compile_features(gaMeshFileCodecTests PRIVATE cxx_std_20)
        target_link_libraries(gaMeshFileCodecTests PRIVATE GeometryArtist::FileIO Qt6::Core)
        if(MSVC)
            target_compile_options(gaMeshFileCodecTests PRIVATE /utf-8)
        endif()
        add_dependencies(gaMeshFileCodecTests gaFileMeshCodecs)
        add_test(NAME ga.io.mesh_codecs COMMAND gaMeshFileCodecTests $<TARGET_FILE:gaFileMeshCodecs>)
    endif()
endif()
