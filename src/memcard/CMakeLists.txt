set(RESONANCE_MEMCARD_SRCS
    checkinfoop.cpp
    closeop.cpp
    createdirop.cpp
    deletefileop.cpp
    deleteremixmct.cpp
    entspaceop.cpp
    formatcardmct.cpp
    formatop.cpp
    getallconnectstatesmct.cpp
    getconnectstatemct.cpp
    listdirop.cpp
    listremixesmct.cpp
    loadfilemct.cpp
    loadremixmct.cpp
    memcard.cpp
    memcardcbhandler.cpp
    memcardmanager.cpp
    memcardop.cpp
    memcardps2.cpp
    memcardsavepaths.cpp
    memcardtask.cpp
    memcarduser.cpp
    minimumsavespacemct.cpp
    openreadop.cpp
    openwriteop.cpp
    readop.cpp
    renamefileop.cpp
    savefilemct.cpp
    saveremixmct.cpp
    seekop.cpp
    unformatop.cpp
    writeop.cpp)

add_library(resonance_memcard STATIC ${RESONANCE_MEMCARD_SRCS})
target_link_libraries(resonance_memcard PRIVATE resonance_common)
