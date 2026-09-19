set(RESONANCE_MET_SRCS
    metarenasscreen.cpp
    metexpansionpakscreen.cpp
    metglobalsettingssaverscreen.cpp
    metjukeboxbasescreen.cpp
    metjukeboxcustomremixesscreen.cpp
    metjukeboxeditplaylistscreen.cpp
    metjukeboxeditplaylistscreendone.cpp
    metjukeboxeditplaylistscreenlowerleft.cpp
    metjukeboxfactoryremixesscreen.cpp
    metjukeboxtopbuttonsscreen.cpp
    metkeyboardscreen.cpp
    metloadfreqbasescreen.cpp
    metloadfreqscreen.cpp
    metloadgamescreen.cpp
    metloadnewfreqscreen.cpp
    metloadprefabscreen.cpp
    metmemcardloadscreen.cpp
    metmemcardtypescreen.cpp
    metmemdetectscreen.cpp
    metmemdetectstartup.cpp
    metmultisaveremixscreen.cpp
    metpersonasaverscreen.cpp
    metremixdatascreen.cpp
    metremixdelscreen.cpp
    metremixloadscreen.cpp
    metremixmanager.cpp
    metremixsaver.cpp
    metremixtypescreen.cpp
    metrenderer.cpp
    metsaveremix.cpp
    metsaveremixscreen.cpp
    metscreen.cpp
    metsoloendremixscreen.cpp
    metsolowinscreen.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature carries, which the call sites prove the routine takes. The warning is
# therefore reporting a property of the original rather than a defect here, and
# it is disabled for these sources only.
set_source_files_properties(
    metscreen.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_met STATIC ${RESONANCE_MET_SRCS})
target_link_libraries(resonance_met PRIVATE resonance_common)
