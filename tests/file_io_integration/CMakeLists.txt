include(${CMAKE_CURRENT_LIST_DIR}/source.cmake)

add_executable(gaFileWorkbenchIntegrationTests ${GA_FILE_WORKBENCH_TEST_SOURCES})
target_compile_features(gaFileWorkbenchIntegrationTests PRIVATE cxx_std_20)
target_link_libraries(gaFileWorkbenchIntegrationTests PRIVATE
    GeometryArtist::FileIO GeometryArtist::WorkbenchRuntime Qt6::Core)
add_dependencies(gaFileWorkbenchIntegrationTests gaFileMeshCodecs)
set_target_properties(gaFileWorkbenchIntegrationTests PROPERTIES
    CXX_EXTENSIONS OFF AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(MSVC)
    target_compile_options(gaFileWorkbenchIntegrationTests PRIVATE /utf-8)
endif()
add_test(NAME ga.io.workbench_integration
    COMMAND gaFileWorkbenchIntegrationTests $<TARGET_FILE:gaFileMeshCodecs>)
set_tests_properties(ga.io.workbench_integration PROPERTIES TIMEOUT 30 LABELS "integration;io;workbench")
