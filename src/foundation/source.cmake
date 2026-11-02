# -----------------------------------------------------------------------------
# File placement guide: paths are relative to this module. List files for this target, not every file in the directory. See the root source.cmake for general header rules.
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Shared base interfaces/templates in include/. This INTERFACE target does not compile implementation .cpp files.
# -----------------------------------------------------------------------------
set(GA_FOUNDATION_HEADERS
    include/IGAObject.h
    include/IGARootObject.h
    include/IGAAssembly.h
)
