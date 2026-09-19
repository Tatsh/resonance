set(RESONANCE_SYNTH_SRCS
    callbackxferbdtoiop.cpp
    callbackxferhdtoiop.cpp
    midi_main.cpp
    ps2hardsynth.cpp
    synth.cpp
    synthsustainer.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature carries, which the call sites prove the routine takes. The warning is
# therefore reporting a property of the original rather than a defect here, and
# it is disabled for these sources only.
set_source_files_properties(
    callbackxferbdtoiop.cpp
    callbackxferhdtoiop.cpp
    synth.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_synth STATIC ${RESONANCE_SYNTH_SRCS})
target_link_libraries(resonance_synth PRIVATE resonance_common)
