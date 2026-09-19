set(RESONANCE_SYNTH_SRCS
    callbackxferbdtoiop.cpp
    callbackxferhdtoiop.cpp
    midi_main.cpp
    ps2hardsynth.cpp
    synth.cpp
    synthsustainer.cpp)

add_library(resonance_synth STATIC ${RESONANCE_SYNTH_SRCS})
target_link_libraries(resonance_synth PRIVATE resonance_common)
