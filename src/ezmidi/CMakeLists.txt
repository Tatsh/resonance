set(RESONANCE_EZMIDI_SRCS
    key.c
    midi_com.c
    midi_ent.c
    midi_hsyn.c
    notes.c)

add_library(resonance_ezmidi STATIC ${RESONANCE_EZMIDI_SRCS})
target_link_libraries(resonance_ezmidi PRIVATE resonance_common)

# The IOP server module links separately; see cmake/ezmidi-irx.cmake.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ezmidi-irx)
