# =============================================================================
# Source-manifest guide
# =============================================================================
# This root manifest has no source entries. Keep each module's files in its
# own source.cmake; CMakeLists.txt owns targets, dependencies and build options.
#
# Ordinary .h/.hpp/.inl files included by C++ code do not have to be listed
# to be read by the compiler. Listing helps IDE organization and tools such
# as AUTOMOC; it does not make a header public, exported, or installed.
#
# PUBLIC_HEADERS / CONTRACT_HEADERS: add intended API headers, not internal
# helpers. These names are conventions; actual propagation is determined by
# how CMakeLists.txt uses the variables (for example target_sources INTERFACE).
#
# Private helpers: a normal included header may stay unlisted. For IDE display
# or AUTOMOC, add a PRIVATE_HEADERS list and attach it to its compiled target
# with target_sources(... PRIVATE ...); update source_group roots if needed.
# Merely declaring a new list does nothing until CMakeLists.txt consumes it.
#
# SOURCES: list enabled implementation units once per intended target.
# Do not blindly add unfinished scaffold .cpp files or vendor headers.
# TEST_SOURCES belong to test targets; another test main needs its own target.
#
# Qt headers containing Q_OBJECT should be explicitly attached to their
# owning target for reliable AUTOMOC discovery. .ui goes in UI_FILES.
# Do not manually list generated ui_*.h/moc_*.cpp; autogen manages them.
