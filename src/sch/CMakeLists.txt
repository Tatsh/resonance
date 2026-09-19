set(RESONANCE_SCH_SRCS
    cmdid.cpp
    command.cpp
    tempomap.cpp
    tick.cpp
    tickclock.cpp
    timedcommand.cpp)

add_library(resonance_sch STATIC ${RESONANCE_SCH_SRCS})
target_link_libraries(resonance_sch PRIVATE resonance_common)
