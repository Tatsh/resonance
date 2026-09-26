# Fourteen sources of this directory are not built. Each one includes the embedded
# interpreter's own Python 2 headers. This tree records that interpreter's differences
# rather than including its headers. They are listed so every source is accounted for.
set(RESONANCE_SCRIPT_UNBUILT_SRCS
    apptunnelscript.cpp
    configquery.cpp
    cxx/object.cpp
    gamecheats.cpp
    hudscript.cpp
    hxcommands.cpp
    hxmodule.cpp
    input.cpp
    powerup.cpp
    pyshell.cpp
    recinfo.cpp
    scripteval.cpp
    scriptfunc.cpp
    scripthost.cpp
    testmap.cpp
    togglemode.cpp)

set(RESONANCE_SCRIPT_SRCS
    cmdpostscript.cpp
    sceneanim.cpp
    scriptcalltemplates.cpp
    scripttemplatemap.cpp
    testregistry.cpp)

add_library(resonance_script STATIC ${RESONANCE_SCRIPT_SRCS})
target_link_libraries(resonance_script PRIVATE resonance_common)
