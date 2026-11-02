project(gaComponentRuntime LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaComponentRuntime STATIC
    ${GA_COMPONENT_RUNTIME_HEADERS}
    ${GA_COMPONENT_RUNTIME_SOURCES}
)
add_library(GeometryArtist::ComponentRuntime ALIAS gaComponentRuntime)

target_compile_features(gaComponentRuntime PUBLIC cxx_std_20)
target_include_directories(gaComponentRuntime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(gaComponentRuntime PUBLIC GeometryArtist::Foundation)
set_target_properties(gaComponentRuntime PROPERTIES POSITION_INDEPENDENT_CODE ON)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "Component Runtime"
    FILES ${GA_COMPONENT_RUNTIME_HEADERS} ${GA_COMPONENT_RUNTIME_SOURCES}
)

# This target has no Qt-generated sources.
set_target_properties(gaComponentRuntime PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF CXX_EXTENSIONS OFF)
