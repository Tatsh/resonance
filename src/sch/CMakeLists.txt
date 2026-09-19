set(RESONANCE_SCH_SRCS
    cmdid.cpp
    command.cpp
    tempomap.cpp
    tick.cpp
    tickclock.cpp
    timedcommand.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature carries, which the call sites prove the routine takes. The warning is
# therefore reporting a property of the original rather than a defect here, and
# it is disabled for these sources only.
set_source_files_properties(
    command.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_sch STATIC ${RESONANCE_SCH_SRCS})
target_link_libraries(resonance_sch PRIVATE resonance_common)
