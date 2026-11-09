set(GA_POLICY_EXAMPLE_HEADERS
    include/GAAlgorithmTypes.h
    include/GAAlgorithmPropertyPolicy.h
    include/GAAlgorithmSerializationPolicy.h
    include/GAAlgorithmObservationPolicy.h
    include/GAAlgorithmAssembly.h
    include/GAAlgorithmPlugin.h
)
set(GA_POLICY_EXAMPLE_LOADER_SOURCES src/GAAlgorithmPlugin.cpp)
set(GA_POLICY_EXAMPLE_HOST_SOURCES src/main.cpp)
set(GA_POLICY_EXAMPLE_COMPOSITE_SOURCES plugins/GACompositeAlgorithm.cpp)
set(GA_POLICY_EXAMPLE_DEPENDENCY_TEST_SOURCES tests/GACompositePluginTests.cpp)
set(GA_POLICY_EXAMPLE_PLUGIN_SOURCES plugins/GAExampleAlgorithm.cpp)
