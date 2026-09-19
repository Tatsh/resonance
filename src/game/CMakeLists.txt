set(RESONANCE_GAME_SRCS
    axingstg.cpp
    campaignstats.cpp
    catcher.cpp
    catchingstg.cpp
    freqappearance.cpp
    gamemanagerimpl.cpp
    gameparams.cpp
    gamepowerupplacer.cpp
    gamerconstructcmd.cpp
    gamestats.cpp
    genericcatcher.cpp
    idablebase.cpp
    jukeboxplaylist.cpp
    levelbuilder.cpp
    levelconverter.cpp
    levelstats.cpp
    localplayer.cpp
    multicatcher.cpp
    netplayer.cpp
    nullplayer.cpp
    pitchingstg.cpp
    player.cpp
    playerinfo.cpp
    playmap.cpp
    playmaplinear.cpp
    playmaprepeatring.cpp
    playmapring.cpp
    powerupcollection.cpp
    powerupcollectioni.cpp
    rawcontroller.cpp
    scoretrackgraph.cpp
    singlepowerupcollection.cpp
    skillstats.cpp
    trackselector.cpp
    voxingstg.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature carries, which the call sites prove the routine takes. The warning is
# therefore reporting a property of the original rather than a defect here, and
# it is disabled for these sources only.
set_source_files_properties(
    scoretrackgraph.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_game STATIC ${RESONANCE_GAME_SRCS})
target_link_libraries(resonance_game PRIVATE resonance_common)
