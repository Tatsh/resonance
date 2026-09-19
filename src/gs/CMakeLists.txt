set(RESONANCE_GS_SRCS
    effector.cpp
    ghostnoteseffector.cpp
    mixer.cpp
    multimuse.cpp
    multimuseplayer.cpp
    musesynth.cpp
    notepitcher.cpp
    scratcher.cpp
    voxer.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature declares. The call sites prove the routine takes that parameter. The
# warning therefore reports a property of the original rather than a defect here,
# and it is disabled for these sources only.
set_source_files_properties(
    effector.cpp
    ghostnoteseffector.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_gs STATIC ${RESONANCE_GS_SRCS})
target_link_libraries(resonance_gs PRIVATE resonance_common)
