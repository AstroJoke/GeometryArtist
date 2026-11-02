# -----------------------------------------------------------------------------
# File placement guide: paths are relative to this module. List files for this target, not every file in the directory. See the root source.cmake for general header rules.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Public component registry/factory API headers in include/; not individual plugin implementations.
# -----------------------------------------------------------------------------
set(GA_COMPONENT_RUNTIME_HEADERS
    include/GAComponentTypes.h
    include/GAComponentRuntime.h
)

# -----------------------------------------------------------------------------
# Component runtime .cpp implementations compiled into gaComponentRuntime.
# -----------------------------------------------------------------------------
set(GA_COMPONENT_RUNTIME_SOURCES
    src/GAComponentRuntime.cpp
)
