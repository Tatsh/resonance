set(RESONANCE_APP_SRCS
    application.cpp
    attachment.cpp
    globals.cpp
    hudbadge.cpp
    hudfreq.cpp
    hudscore.cpp
    hudscorepulse.cpp
    hudutil.cpp
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
