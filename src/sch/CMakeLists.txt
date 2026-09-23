set(RESONANCE_SCH_SRCS
    barsequencer.cpp
    cmdid.cpp
    command.cpp
    sequencercmd.cpp
    tempomap.cpp
    tick.cpp
    tickclock.cpp
    timedcommand.cpp)

add_library(resonance_sch STATIC ${RESONANCE_SCH_SRCS})
target_link_libraries(resonance_sch PRIVATE resonance_common)
