include("${CMAKE_CURRENT_SOURCE_DIR}/source.cmake")
add_executable(gaCgalBooleanUseCaseTests ${GA_CGAL_USE_CASE_TEST_SOURCES})
target_link_libraries(gaCgalBooleanUseCaseTests PRIVATE GeometryArtist::CgalGeometry GeometryArtist::WorkbenchRuntime)
target_include_directories(gaCgalBooleanUseCaseTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../src/adapters/geometry/cgal/tests")
set_target_properties(gaCgalBooleanUseCaseTests PROPERTIES
    CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(MSVC)
    target_compile_options(gaCgalBooleanUseCaseTests PRIVATE /utf-8)
endif()
add_test(NAME ga.geometry.cgal_workbench COMMAND gaCgalBooleanUseCaseTests)
set_tests_properties(ga.geometry.cgal_workbench PROPERTIES TIMEOUT 120)
