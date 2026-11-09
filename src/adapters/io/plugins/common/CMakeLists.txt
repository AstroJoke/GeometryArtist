include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

add_library(gaMeshFileCodecSupport STATIC
    ${GA_MESH_FILE_SUPPORT_HEADERS}
    ${GA_MESH_FILE_SUPPORT_SOURCES}
)
target_compile_features(gaMeshFileCodecSupport PUBLIC cxx_std_20)
target_include_directories(gaMeshFileCodecSupport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(gaMeshFileCodecSupport PUBLIC GeometryArtist::FileIO PRIVATE Qt6::Core)
set_target_properties(gaMeshFileCodecSupport PROPERTIES POSITION_INDEPENDENT_CODE ON)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Mesh File Support"
    FILES ${GA_MESH_FILE_SUPPORT_HEADERS} ${GA_MESH_FILE_SUPPORT_SOURCES}
)
