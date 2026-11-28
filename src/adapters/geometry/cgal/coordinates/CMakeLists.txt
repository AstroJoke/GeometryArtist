include("${CMAKE_CURRENT_SOURCE_DIR}/source.cmake")
find_package(CGAL 5.6 REQUIRED)
# The composition root selects this bridge; the base adapter does not depend on coordinates.
add_library(gaCgalCoordinates STATIC ${GA_CGAL_COORDINATE_PUBLIC_HEADERS} ${GA_CGAL_COORDINATE_PRIVATE_HEADERS} ${GA_CGAL_COORDINATE_SOURCES})
add_library(GeometryArtist::CgalCoordinates ALIAS gaCgalCoordinates)
target_include_directories(gaCgalCoordinates PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_link_libraries(gaCgalCoordinates PUBLIC GeometryArtist::CgalGeometry GeometryArtist::CoordinateContracts
    PRIVATE CGAL::CGAL GeometryArtist::CoordinateCore)
set_target_properties(gaCgalCoordinates PROPERTIES
    CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(MSVC)
    target_compile_options(gaCgalCoordinates PRIVATE /utf-8)
endif()
if(BUILD_TESTING)
    add_executable(gaCgalCoordinateAdapterTests ${GA_CGAL_COORDINATE_TEST_SOURCES})
    target_include_directories(gaCgalCoordinateAdapterTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../tests")
    target_link_libraries(gaCgalCoordinateAdapterTests PRIVATE GeometryArtist::CgalCoordinates GeometryArtist::CoordinateCore)
    set_target_properties(gaCgalCoordinateAdapterTests PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
        AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    if(MSVC)
        target_compile_options(gaCgalCoordinateAdapterTests PRIVATE /utf-8)
    endif()
    add_test(NAME ga.geometry.cgal_coordinates COMMAND gaCgalCoordinateAdapterTests)
    set_tests_properties(ga.geometry.cgal_coordinates PROPERTIES TIMEOUT 120)
endif()
