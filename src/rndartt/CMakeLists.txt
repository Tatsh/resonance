set(RESONANCE_RNDARTT_SRCS
    abitmap.cpp
    acanvas.cpp
    acanvas15.cpp
    acanvas24.cpp
    acanvas32.cpp
    acanvas8.cpp
    acanvaslin15.cpp
    acanvaslin24.cpp
    acanvaslin32.cpp
    acanvaslin4.cpp
    acanvaslin8.cpp
    apalette.cpp
    arect.cpp
    arlereader.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature carries, which the call sites prove the routine takes. The warning is
# therefore reporting a property of the original rather than a defect here, and
# it is disabled for these sources only.
set_source_files_properties(
    acanvas32.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_rndartt STATIC ${RESONANCE_RNDARTT_SRCS})
target_link_libraries(resonance_rndartt PRIVATE resonance_common)
