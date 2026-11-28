project(gaGeometryContracts LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaGeometryContracts INTERFACE)
add_library(GeometryArtist::GeometryContracts ALIAS gaGeometryContracts)
target_compile_features(gaGeometryContracts INTERFACE cxx_std_20)
target_include_directories(gaGeometryContracts INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(gaGeometryContracts INTERFACE GeometryArtist::DataCore)
target_sources(gaGeometryContracts INTERFACE ${GA_GEOMETRY_CONTRACT_HEADERS})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PREFIX "Domain Contracts/Geometry"
    FILES ${GA_GEOMETRY_CONTRACT_HEADERS}
)
