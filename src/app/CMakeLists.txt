set(RESONANCE_APP_SRCS
    animrange.cpp
    application.cpp
    attachment.cpp
    globals.cpp
    hudbadge.cpp
    hudcountdown.cpp
    hudeffects.cpp
    hudenergy.cpp
    hudfreq.cpp
    hudloop.cpp
    hudpoints.cpp
    hudpowerup.cpp
    hudscore.cpp
    hudscorepulse.cpp
    hudtextmessage.cpp
    hudtrack.cpp
    hudtracklabel.cpp
    hudutil.cpp
    linearramp.cpp
    longop.cpp
    mainloop.cpp
    msgqueue.cpp
    msgsink.cpp
    msgsource.cpp
    overlay.cpp
    renderer.cpp
    rendererbase.cpp
    scriptsink.cpp
    task.cpp)

add_library(resonance_app STATIC ${RESONANCE_APP_SRCS})
target_link_libraries(resonance_app PRIVATE resonance_common)
