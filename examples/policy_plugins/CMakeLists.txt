cmake_minimum_required(VERSION 3.20)
project(GAPolicyPluginExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)
include(${CMAKE_CURRENT_SOURCE_DIR}/source.cmake)

set(GA_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../src" CACHE PATH "GeometryArtist source directory")
find_package(Qt6 REQUIRED COMPONENTS Core)
add_subdirectory("${GA_SOURCE_ROOT}/foundation" foundation)
add_subdirectory("${GA_SOURCE_ROOT}/runtime" runtime)

add_library(gaPolicyExampleInterfaces INTERFACE)
target_include_directories(gaPolicyExampleInterfaces INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gaPolicyExampleInterfaces INTERFACE GeometryArtist::Foundation)

add_library(gaPolicyExampleLoader STATIC ${GA_POLICY_EXAMPLE_LOADER_SOURCES})
target_link_libraries(gaPolicyExampleLoader PUBLIC gaPolicyExampleInterfaces GeometryArtist::ComponentRuntime PRIVATE Qt6::Core)
set_target_properties(gaPolicyExampleLoader PROPERTIES POSITION_INDEPENDENT_CODE ON)

foreach(backend IN ITEMS Reference Standard)
    add_library(gaExample${backend} MODULE ${GA_POLICY_EXAMPLE_PLUGIN_SOURCES} ${GA_POLICY_EXAMPLE_HEADERS})
    target_link_libraries(gaExample${backend} PRIVATE gaPolicyExampleInterfaces GeometryArtist::ComponentRuntime)
    set_target_properties(gaExample${backend} PROPERTIES PREFIX "")
endforeach()
target_compile_definitions(gaExampleStandard PRIVATE GA_EXAMPLE_STANDARD)

add_library(gaExampleComposite MODULE ${GA_POLICY_EXAMPLE_COMPOSITE_SOURCES})
target_link_libraries(gaExampleComposite PRIVATE gaPolicyExampleLoader)
target_compile_definitions(gaExampleComposite PRIVATE GA_EXAMPLE_CHILD_FILENAME="$<TARGET_FILE_NAME:gaExampleReference>")
add_dependencies(gaExampleComposite gaExampleReference)

add_library(gaExampleBundled MODULE ${GA_POLICY_EXAMPLE_COMPOSITE_SOURCES} ${GA_POLICY_EXAMPLE_PLUGIN_SOURCES})
target_link_libraries(gaExampleBundled PRIVATE gaPolicyExampleInterfaces GeometryArtist::ComponentRuntime)
target_compile_definitions(gaExampleBundled PRIVATE GA_EXAMPLE_BUNDLED GA_EXAMPLE_EMBEDDED)
set_target_properties(gaExampleComposite gaExampleBundled PROPERTIES PREFIX "")

add_executable(gaPolicyExample ${GA_POLICY_EXAMPLE_HOST_SOURCES} ${GA_POLICY_EXAMPLE_HEADERS})
target_link_libraries(gaPolicyExample PRIVATE gaPolicyExampleLoader Qt6::Core)
add_dependencies(gaPolicyExample gaExampleReference gaExampleStandard)
if(BUILD_TESTING)
    add_executable(gaCompositePluginTests ${GA_POLICY_EXAMPLE_DEPENDENCY_TEST_SOURCES})
    target_link_libraries(gaCompositePluginTests PRIVATE gaPolicyExampleLoader Qt6::Core)
    add_dependencies(gaCompositePluginTests gaExampleComposite gaExampleBundled gaExampleStandard)
    add_test(NAME ga.example.module_owned_dependencies COMMAND gaCompositePluginTests
        $<TARGET_FILE:gaExampleComposite> $<TARGET_FILE:gaExampleBundled>
        $<TARGET_FILE:gaExampleReference> $<TARGET_FILE:gaExampleStandard>)
    set_tests_properties(ga.example.module_owned_dependencies PROPERTIES TIMEOUT 20)
    add_test(NAME ga.example.policy_plugins COMMAND gaPolicyExample
        $<TARGET_FILE:gaExampleReference> $<TARGET_FILE:gaExampleStandard>)
    set_tests_properties(ga.example.policy_plugins PROPERTIES TIMEOUT 20)
endif()
