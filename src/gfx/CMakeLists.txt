set(RESONANCE_GFX_SRCS
    gfxdevice.cpp
    vramtable.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature carries, which the call sites prove the routine takes. The warning is
# therefore reporting a property of the original rather than a defect here, and
# it is disabled for these sources only.
set_source_files_properties(
    vramtable.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_gfx STATIC ${RESONANCE_GFX_SRCS})
target_link_libraries(resonance_gfx PRIVATE resonance_common)
