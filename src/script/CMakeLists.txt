# The embedded interpreter's headers are vendored under 3rdparty rather than recorded as
# differences. The port's own configuration header comes first on the path so it wins over
# anything upstream with the same name.
set(RESONANCE_SCRIPT_SRCS
    apptunnelscript.cpp
    cmdpostscript.cpp
    configquery.cpp
    cxx/extensionmodulebase.cpp
    cxx/int.cpp
    cxx/object.cpp
    cxx/pythonextensionbase.cpp
    cxx/pythontype.cpp
    cxx/seqbase.cpp
    cxx/string.cpp
    cxx/tuple.cpp
    gamecheats.cpp
    hudscript.cpp
    hxcommands.cpp
    hxmodule.cpp
    input.cpp
    playermode.cpp
    powerup.cpp
    pyinterpreter.cpp
    pyshell.cpp
    recinfo.cpp
    sceneanim.cpp
    scripteval.cpp
    scriptcalltemplates.cpp
    scriptfunc.cpp
    scripthost.cpp
    scripttemplatemap.cpp
    testmap.cpp
    testregistry.cpp
    togglemode.cpp)

add_library(resonance_script STATIC ${RESONANCE_SCRIPT_SRCS})
target_link_libraries(resonance_script PRIVATE resonance_common)
target_include_directories(
  resonance_script PRIVATE "${CMAKE_SOURCE_DIR}/src/python/PC"
                           "${CMAKE_SOURCE_DIR}/3rdparty/Python-2.0/Include")
