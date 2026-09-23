#include "met/metloadprefabscreen.h"

#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfrontendstate.h"
#include "met/metpersonadata.h"
#include "rndartt/apalette.h"

namespace {

static const char *const kFreqMakerCanvasScreen = "MetFreqMakerCanvasScreen";
static const char *const kFreqMakerButtonsScreen = "MetFreqMakerButtonsScreen";
static const char *const kLoadPreFabScreen = "MetLoadPreFabScreen";

// The MetFreqMakerButtonsScreen::SetEditing() value, and the LoadPrefab() randomise flag.
constexpr int kFreqMakerEditing = 1;
constexpr int kNoRandomize = 0;

} // namespace

MetLoadPreFabScreen::MetLoadPreFabScreen(MetRenderer *pRenderer, int nPriority)
    : MetLoadFreqBaseScreen(pRenderer, nPriority) {
}

// 0x002a9330
void MetLoadPreFabScreen::PrepareFreqMakerForSelection() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    MetPersonaData *pPersona = (*mUnknown8c)[mUnknown94];
    if (static_cast<unsigned>(mUnknown94) < MetPersonaData::savedList()->size()) {
        pCanvas->LoadPersona(pPersona);
    } else {
        pCanvas->LoadPrefab(pPersona, kNoRandomize);
    }
    pButtons->SetEditing(kFreqMakerEditing);
    pButtons->mNewPersona = 0;
    MetFrontEndState::shared()->mUnknown24 = HxStr(kLoadPreFabScreen);
}
