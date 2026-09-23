#include "game/freqappearance.h"

#include <iostream>
#include <vector>

#include "met/metfreqmakerassetmanager.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

// Written by Save() and read back by Load() into a local that nothing consults.
constexpr int kRecordVersion = 8;

// The number of persona burn slots.
constexpr int kBurnSlotCount = 4;
// The camera names count from one and the hangpoint names from zero.
static const char *const kBurnCamFormat = "persona_texburn_%i.cam";
static const char *const kHangpointFormat = "%i_freq_hangpoint.view";

// 0x0067af30. The camera of each burn slot.
std::vector<Rnd::Cam *> g_burnCams;
// 0x0067af3c. Non-zero once InitBurnSlots() has resolved both lists.
int g_nBurnSlotsReady = 0;
// 0x0067af40. The view each burn slot hangs its avatar from.
std::vector<Rnd::View *> g_hangpoints;
// 0x008efc80. The detail object AttachToBurnSlot() last hung in each slot.
FreqAppearanceDetail *g_apBurnSlotDetails[kBurnSlotCount];

} // namespace

// 0x001745b8
FreqAppearance::FreqAppearance()
    : mUnknown00("initial name"), mDetail(new FreqAppearanceDetail), mUnknown0c(0) {
}

// 0x00174668
FreqAppearance::FreqAppearance(const FreqAppearance &other)
    : mUnknown00("initial name"), mDetail(new FreqAppearanceDetail), mUnknown0c(0) {
    *this = other;
}

// 0x00174730
FreqAppearance::~FreqAppearance() {
    delete mDetail;
}

// 0x00171060
void FreqAppearance::Save(OBStream &stream) {
    int version = kRecordVersion;
    stream.Write(&version, sizeof(version));

    unsigned length = mUnknown00.mLen;
    stream.Write(&length, sizeof(length));
    // An empty username has no buffer, and the stream receives the shared empty string in place of
    // a null pointer.
    stream.WriteBytes(mUnknown00.mStr != nullptr ? mUnknown00.mStr : g_szEmptyString, length);

    mDetail->save(stream);

    int skillStatus = mUnknown0c;
    stream.Write(&skillStatus, sizeof(skillStatus));
}

// 0x001747a8
void FreqAppearance::Load(IBStream &stream) {
    int version;
    stream.Read(&version, sizeof(version));

    unsigned length;
    stream.Read(&length, sizeof(length));
    mUnknown00.Alloc(length);
    stream.ReadBytes(
        mUnknown00.mStr != nullptr ? mUnknown00.mStr : const_cast<char *>(g_szEmptyString), length);

    mDetail->load(stream);

    stream.Read(&mUnknown0c, sizeof(mUnknown0c));
}

// 0x00174878
void FreqAppearance::Print(std::ostream &stream) {
    // The detail object is never written, so the two literals below arrive back to back.
    stream << "username=" << mUnknown00 << " Freq=" << " SkillStatus=" << mUnknown0c;
}

// 0x001748e0
void FreqAppearance::operator=(const FreqAppearance &other) {
    if (&other != this) {
        mUnknown00 = other.mUnknown00;
        mDetail->clear();
        mDetail->copyFrom(*other.mDetail);
    }
}

// 0x00171138
void FreqAppearance::AttachToBurnSlot(int nSlot) {
    InitBurnSlots();
    g_apBurnSlotDetails[nSlot] = mDetail;

    Rnd::View *pHangpoint = g_hangpoints[nSlot];
    pHangpoint->ClearDraws();
    pHangpoint->ClearTransList();

    Rnd::View *pView = mDetail->mView;
    pHangpoint->AddDraw(pView, nullptr);
    pHangpoint->AddTrans(pView);
    pView->UpdateWorldXfm(pHangpoint, 1);

    pHangpoint->SetShowingRecursive(1);
    g_burnCams[nSlot]->SetShowing(1);
}

// 0x00171398
void FreqAppearance::InitBurnSlots() {
    if (g_nBurnSlotsReady != 0) {
        return;
    }

    MetFreqMakerAssetManager::shared()->PollLoad(); // Yes, the binary discards the result.
    g_burnCams.resize(kBurnSlotCount);
    g_hangpoints.resize(kBurnSlotCount);
    for (int i = 0; i < kBurnSlotCount; ++i) {
        g_burnCams[i] = dynamic_cast<Rnd::Cam *>(
            Rnd::g_manager.Find(HxStr(FormatString(kBurnCamFormat, i + 1))));
        g_burnCams[i]->SetShowing(0);

        g_hangpoints[i] = dynamic_cast<Rnd::View *>(
            Rnd::g_manager.Find(HxStr(FormatString(kHangpointFormat, i))));
        g_hangpoints[i]->SetShowing(1);
    }
    g_nBurnSlotsReady = 1;
}
