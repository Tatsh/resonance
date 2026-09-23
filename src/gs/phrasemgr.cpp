#include "gs/phrasemgr.h"

#include "game/nullplayer.h"
#include "game/phrase.h"
#include "game/phrasedatabase.h"

// 0x001c0268
Player *PhraseMgr::GetPhraseOwner(int nBar) {
    Phrase *pPhrase = mDatabase->GetPhraseAt(nBar);
    if (pPhrase == nullptr) {
        return &g_nullPlayer;
    }
    return pPhrase->mPlayer;
}
