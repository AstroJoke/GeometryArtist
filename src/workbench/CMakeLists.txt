project(gaWorkbench LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaWorkbenchContracts INTERFACE)
add_library(GeometryArtist::WorkbenchContracts ALIAS gaWorkbenchContracts)
target_compile_features(gaWorkbenchContracts INTERFACE cxx_std_20)
target_include_directories(gaWorkbenchContracts INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/workflow/include
)
target_link_libraries(gaWorkbenchContracts INTERFACE GeometryArtist::DataCore)
target_sources(gaWorkbenchContracts INTERFACE ${GA_WORKBENCH_PUBLIC_HEADERS})

add_library(gaWorkbenchRuntime STATIC ${GA_WORKBENCH_SOURCES})
add_library(GeometryArtist::WorkbenchRuntime ALIAS gaWorkbenchRuntime)
target_compile_features(gaWorkbenchRuntime PUBLIC cxx_std_20)
target_link_libraries(gaWorkbenchRuntime PUBLIC GeometryArtist::WorkbenchContracts)
set_target_properties(gaWorkbenchRuntime PROPERTIES
    POSITION_INDEPENDENT_CODE ON CXX_EXTENSIONS OFF AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
