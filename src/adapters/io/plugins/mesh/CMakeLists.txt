include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

find_package(CGAL 5.6 REQUIRED)

add_library(gaFileMeshCodecs MODULE ${GA_CGAL_FILE_HEADERS} ${GA_CGAL_FILE_SOURCES})
target_include_directories(gaFileMeshCodecs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(gaFileMeshCodecs PRIVATE gaMeshFileCodecSupport CGAL::CGAL)
set_target_properties(gaFileMeshCodecs PROPERTIES
    PREFIX ""
    DEBUG_POSTFIX "_d"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/file"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/file"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "mesh File Plugin"
    FILES ${GA_CGAL_FILE_HEADERS} ${GA_CGAL_FILE_SOURCES}
)
