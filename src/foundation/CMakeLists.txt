project(gaFoundation LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaFoundation INTERFACE)
add_library(GeometryArtist::Foundation ALIAS gaFoundation)

target_compile_features(gaFoundation INTERFACE cxx_std_20)
target_include_directories(gaFoundation INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_sources(gaFoundation INTERFACE ${GA_FOUNDATION_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${GA_FOUNDATION_HEADERS}
)
