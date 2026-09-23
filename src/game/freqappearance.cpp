#include "game/freqappearance.h"

#include <cstring>
#include <iostream>
#include <libgraph.h>
#include <list>
#include <vector>

#include "math/color.h"
#include "met/metfreqmakerassetmanager.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/manager.h"
#include "rnd/tex.h"
#include "rnd/view.h"
#include "rndartt/acanvas.h"

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

static const char *const kBurnTextureFormat = "persona_texburn_texture_%d.tex";

// Arguments to sceGsSyncPath() that wait for every path without a timeout.
constexpr int kGsSyncPathWait = 0;
constexpr unsigned short kGsSyncPathNoTimeout = 0;

// RenderBurnTextures() locks the top mip level of both textures, reading the render target back
// from GS memory, and passes SetPalette() the second word Rnd::Movie also passes.
constexpr int kTopMip = 0;
constexpr int kLockReadBack = 2;
constexpr int kPaletteUnknownWord = -1;

// The layout EncodeNonZeroBytes() writes.
constexpr int kEncodedDataLength = 7;
constexpr int kEncodedMaskByte = 7;
constexpr int kEncodedFillLength = 8;
constexpr int kEncodedMaskBase = 0x80;
constexpr unsigned char kEncodedZero = 0xff;

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

// 0x001716d0
void FreqAppearance::RenderBurnTextures() {
    if (g_nBurnSlotsReady == 0) {
        return;
    }

    for (int i = 0; i < kBurnSlotCount; ++i) {
        Rnd::Cam *pCam = g_burnCams[i];
        if (pCam->GetShowing() == 0) {
            continue;
        }

        pCam->UpdateWorldXfm(nullptr, 0);
        pCam->Draw();
        sceGsSyncPath(kGsSyncPathWait, kGsSyncPathNoTimeout);
        pCam->Draw();

        Rnd::Tex *pTarget = pCam->mpTargetTex;
        Rnd::Tex *pBurn = FindPersonaBurnTexture(i);
        ACanvas *pSource = pTarget->LockMipBitmap(kTopMip, 0, kLockReadBack);
        ACanvas *pDest = pBurn->LockMipBitmap(kTopMip, 0, 0);

        std::vector<const Color *> colors;
        std::list<FreqPart *> &parts = g_apBurnSlotDetails[i]->parts();
        for (std::list<FreqPart *>::iterator it = parts.begin(); it != parts.end(); ++it) {
            colors.push_back((*it)->GetColor());
        }
        pSource->QuantizeToRamps(*pDest, colors);

        pTarget->UnlockMipBitmap();
        pBurn->UnlockMipBitmap();
        pBurn->SetPalette(nullptr, kPaletteUnknownWord);
        pCam->SetShowing(0);
    }
}

// 0x001712c0
Rnd::Tex *FreqAppearance::FindPersonaBurnTexture(int nIndex) {
    return dynamic_cast<Rnd::Tex *>(
        Rnd::g_manager.Find(HxStr(FormatString(kBurnTextureFormat, nIndex + 1))));
}

// 0x00174458
void FreqAppearance::CopyFrom(const FreqAppearance &other) {
    *this = other;
}

// 0x001744f8
void FreqAppearance::EncodeNonZeroBytes(const unsigned char *pSource, unsigned char *pDest) {
    memset(pDest + kEncodedMaskByte, kEncodedMaskBase, kEncodedFillLength);
    for (int i = 0; i < kEncodedDataLength; ++i) {
        pDest[i] = pSource[i];
        if (pSource[i] == 0) {
            pDest[kEncodedMaskByte] |= 1 << i;
            pDest[i] = kEncodedZero;
        }
    }
}

// 0x00174930
void FreqAppearance::Pack(Record *pRecord) {
    pRecord->mValid = true;
    pRecord->mSkillStatus = mUnknown0c;

    const char *pszName = mUnknown00.mStr != nullptr ? mUnknown00.mStr : g_szEmptyString;
    int nLength = strlen(pszName);
    if (nLength > kRecordNameLength) {
        nLength = kRecordNameLength;
    }
    for (int i = 0; i < nLength; ++i) {
        pRecord->mName[i] = pszName[i];
    }
    pRecord->mName[nLength] = '\0';

    int nCount;
    mDetail->pack(pRecord->mParts, &nCount);
    pRecord->mPartCount = static_cast<unsigned char>(nCount); // The binary reads one byte back.
}

// 0x001749e8
void FreqAppearance::Unpack(const Record &record) {
    if (record.mValid) {
        mUnknown0c = record.mSkillStatus;
        mUnknown00 = record.mName;
        mDetail->unpack(record.mParts, record.mPartCount);
    }
}
