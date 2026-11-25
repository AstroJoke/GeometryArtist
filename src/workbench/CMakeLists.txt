project(gaWorkbench LANGUAGES CXX)

find_package(Threads REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaWorkbenchContracts INTERFACE)
add_library(GeometryArtist::WorkbenchContracts ALIAS gaWorkbenchContracts)
target_compile_features(gaWorkbenchContracts INTERFACE cxx_std_20)
target_include_directories(gaWorkbenchContracts INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/workflow/include
)
target_link_libraries(gaWorkbenchContracts INTERFACE GeometryArtist::DataCore)
if(GA_BUILD_FILE_IO)
    target_include_directories(gaWorkbenchContracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/use_cases/io/include)
    target_link_libraries(gaWorkbenchContracts INTERFACE GeometryArtist::FileIOContracts)
endif()
target_sources(gaWorkbenchContracts INTERFACE ${GA_WORKBENCH_PUBLIC_HEADERS})

add_library(gaWorkbenchRuntime STATIC ${GA_WORKBENCH_SOURCES})
add_library(GeometryArtist::WorkbenchRuntime ALIAS gaWorkbenchRuntime)
target_compile_features(gaWorkbenchRuntime PUBLIC cxx_std_20)
target_link_libraries(gaWorkbenchRuntime PUBLIC GeometryArtist::WorkbenchContracts PRIVATE Threads::Threads)
set_target_properties(gaWorkbenchRuntime PROPERTIES
    POSITION_INDEPENDENT_CODE ON CXX_EXTENSIONS OFF AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
