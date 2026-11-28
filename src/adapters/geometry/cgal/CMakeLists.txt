include("${CMAKE_CURRENT_SOURCE_DIR}/source.cmake")
find_package(CGAL 5.6 REQUIRED)
find_package(Threads REQUIRED)

# Only this concrete adapter links CGAL; neutral contracts never depend on it or Qt.
add_library(gaCgalGeometry STATIC ${GA_CGAL_GEOMETRY_PUBLIC_HEADERS} ${GA_CGAL_GEOMETRY_PRIVATE_HEADERS} ${GA_CGAL_GEOMETRY_SOURCES})
add_library(GeometryArtist::CgalGeometry ALIAS gaCgalGeometry)
target_include_directories(gaCgalGeometry PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gaCgalGeometry PUBLIC GeometryArtist::GeometryContracts PRIVATE CGAL::CGAL Threads::Threads)
set_target_properties(gaCgalGeometry PROPERTIES
    CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(MSVC)
    target_compile_options(gaCgalGeometry PRIVATE /utf-8)
endif()
if(BUILD_TESTING)
    add_executable(gaCgalGeometryTests ${GA_CGAL_GEOMETRY_TEST_SOURCES})
    target_link_libraries(gaCgalGeometryTests PRIVATE GeometryArtist::CgalGeometry Threads::Threads)
    set_target_properties(gaCgalGeometryTests PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
        AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    if(MSVC)
        target_compile_options(gaCgalGeometryTests PRIVATE /utf-8)
    endif()
    add_test(NAME ga.geometry.cgal COMMAND gaCgalGeometryTests)
    set_tests_properties(ga.geometry.cgal PROPERTIES TIMEOUT 120)
endif()
