project(gaDataCore LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaDataCore SHARED
    ${GA_DATA_CORE_HEADERS}
    ${GA_DATA_CORE_SOURCES}
)
add_library(GeometryArtist::DataCore ALIAS gaDataCore)

target_compile_features(gaDataCore PUBLIC cxx_std_20)
target_include_directories(gaDataCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(gaDataCore PUBLIC GeometryArtist::Foundation)

set_target_properties(gaDataCore PROPERTIES
    DEBUG_POSTFIX "_d"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${GA_DATA_CORE_HEADERS}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PREFIX "Source Files"
    FILES ${GA_DATA_CORE_SOURCES}
)

# This target has no Qt-generated sources.
set_target_properties(gaDataCore PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF CXX_EXTENSIONS OFF)
