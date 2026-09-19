set(RESONANCE_APP_SRCS
    application.cpp
    attachment.cpp
    globals.cpp
    mainloop.cpp
    msgqueue.cpp
    msgsink.cpp
    msgsource.cpp
    rendererbase.cpp
    scriptsink.cpp
    task.cpp)

# Each of these reconstructs a routine whose body ignores a parameter its
# signature declares. The call sites prove the routine takes that parameter. The
# warning therefore reports a property of the original rather than a defect here,
# and it is disabled for these sources only.
set_source_files_properties(
    attachment.cpp
    PROPERTIES COMPILE_OPTIONS -Wno-unused-parameter)

add_library(resonance_app STATIC ${RESONANCE_APP_SRCS})
target_link_libraries(resonance_app PRIVATE resonance_common)
