set(RESONANCE_MID_SRCS
    midifilereader.cpp
    receiver.cpp)

add_library(resonance_mid STATIC ${RESONANCE_MID_SRCS})
target_link_libraries(resonance_mid PRIVATE resonance_common)
