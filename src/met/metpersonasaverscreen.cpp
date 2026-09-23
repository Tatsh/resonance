#include "met/metpersonasaverscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from. The capital S is what the image records.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "dialogue";

// The registry keys StartSave() resolves.
static const char *const kOwnScreenName = "MetPersonaSaverScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";

} // namespace

MetPersonaSaverScreen::MetPersonaSaverScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown98(0), mUnknown9c(0), mUnknownbc(0) {
}

MetPersonaSaverScreen::~MetPersonaSaverScreen() {
    ClearPersonas();
}

void MetPersonaSaverScreen::StartSave(const std::vector<HxStr> &screens,
                                      MetPersonaData *pPersona,
                                      const CardSlot &slot,
                                      int nUnknown9c,
                                      int nUnknown98) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetPersonaSaverScreen *pSaver =
        pScreen != nullptr ? dynamic_cast<MetPersonaSaverScreen *>(pScreen) : nullptr;
    // The binary does not test the result for null.
    pSaver->SetSaveRequest(screens, pPersona, slot);
    pSaver->mUnknown98 = nUnknown98;
    pSaver->mUnknown9c = nUnknown9c;
    pSaver->mUnknown94 = 0;

    MetScreen *pLoadGame = MetScreen::FindScreenByName(HxStr(kLoadGameScreen));
    pLoadGame->PushNamedScreen(HxStr(kOwnScreenName));
    pLoadGame->ActivateNamedPanel(HxStr(kOwnScreenName));
}

void MetPersonaSaverScreen::SetSaveRequest(const std::vector<HxStr> &screens,
                                           MetPersonaData *pPersona,
                                           const CardSlot &slot) {
    mUnknownac = screens;
    mUnknownb8 = pPersona;
    mUnknownc0 = slot;
}

void MetPersonaSaverScreen::ClearPersonas() {
    // Yes, the binary re-reads the size on every iteration rather than caching it.
    for (unsigned index = 0; index < mPersonas.size(); ++index) {
        delete mPersonas[index];
    }
    mPersonas.clear();
}
