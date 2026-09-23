set(RESONANCE_RNDARTT_SRCS
    abitmap.cpp
    acanvas.cpp
    acanvas15.cpp
    acanvas24.cpp
    acanvas32.cpp
    acanvas8.cpp
    acanvaslin15.cpp
    acanvaslin24.cpp
    acanvaslin32.cpp
    acanvaslin4.cpp
    acanvaslin8.cpp
    afixed.cpp
    apalette.cpp
    arect.cpp
    arlereader.cpp)

add_library(resonance_rndartt STATIC ${RESONANCE_RNDARTT_SRCS})
target_link_libraries(resonance_rndartt PRIVATE resonance_common)
