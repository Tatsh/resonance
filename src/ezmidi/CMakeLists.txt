set(RESONANCE_EZMIDI_SRCS
    key.c
    midi_com.c
    midi_ent.c
    midi_hsyn.c
    notes.c)

add_library(resonance_ezmidi STATIC ${RESONANCE_EZMIDI_SRCS})
target_link_libraries(resonance_ezmidi PRIVATE resonance_common)
