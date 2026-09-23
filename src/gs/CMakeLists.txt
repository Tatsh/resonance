set(RESONANCE_GS_SRCS
    effector.cpp
    ghostnoteseffector.cpp
    midionoffeffector.cpp
    mixer.cpp
    multimuse.cpp
    multimuseplayer.cpp
    musesynth.cpp
    notepitcher.cpp
    noteplayer.cpp
    phrasemgr.cpp
    scratcher.cpp
    volumeeffector.cpp
    voxer.cpp)

add_library(resonance_gs STATIC ${RESONANCE_GS_SRCS})
target_link_libraries(resonance_gs PRIVATE resonance_common)
