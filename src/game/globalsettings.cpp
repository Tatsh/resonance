#include "game/globalsettings.h"

#include "met/metkeyboardscreen.h"
#include "os/formatstring.h"
#include "os/mem.h"

namespace {

// The instance Create() allocates. 0x0067ea38.
GlobalSettings *g_pGlobalSettings = nullptr;

// The record versions Save() writes and Load() branches on.
constexpr int kRecordVersion = 4;
constexpr int kMultipleControllersVersion = 1;
constexpr int kPortVersion = 2;
constexpr int kMacrosVersion = 3;
constexpr int kTutorialVersion = 4;

// The number of keyboard macros operator=() copies.
constexpr int kCopiedMacroCount = 12;

// The defaults the constructor installs.
static const char *const kDefaultNetAddress = "127.0.0.2";
static const char *const kPortFormat = "%d";
constexpr int kDefaultNetPort = 2000;
static const char *const kDefaultCardSlotName = "1";
constexpr int kDefaultCardSlotPort = 0;
#ifdef VIDEO_STANDARD_PAL
constexpr int kDefaultRequiredSaveSpace = 128;
constexpr int kDefaultMinimumFreeClusters = 60;
constexpr int kDefaultPersonaMinimumFreeClusters = 2;
#else
constexpr int kDefaultRequiredSaveSpace = 256;
constexpr int kDefaultMinimumFreeClusters = 60;
constexpr int kDefaultPersonaMinimumFreeClusters = 24;
#endif

// The labels Print() writes.
static const char *const kNetAddressLabel = "Net IP Address";
static const char *const kNetPortLabel = "Net Port      ";
static const char *const kControllerLabels[] = {
    " Controller Config 1",
    " Controller Config 2",
    " Controller Config 3",
    " Controller Config 4",
};
static const char *const kTutorialLabel = "Tutorial is complete? ";

// Reports the text of a string, or the shared empty string when it has no buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Writes a string as its length and then its bytes, and reports the stream Write() reports.
inline OBStream &WriteString(OBStream &stream, const HxStr &text) {
    int nLength = text.mLen;
    stream.WriteLE(&nLength, sizeof(nLength));
    return stream.Write(TextOf(text), nLength);
}

// Reads a string WriteString() wrote.
inline void ReadString(IBStream &stream, HxStr &text) {
    int nLength;
    stream.ReadLE(&nLength, sizeof(nLength));
    text.Alloc(nLength);
    stream.Read(text.mStr != nullptr ? text.mStr : const_cast<char *>(g_szEmptyString), nLength);
}

} // namespace

// NTSC-U/C: 0x0018b988, PAL: 0x00191418
void *GlobalSettings::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "GlobalSettings");
}

// NTSC-U/C: 0x0018b9a8, PAL: 0x00191438
void GlobalSettings::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, "GlobalSettings");
}

// NTSC-U/C: 0x00187d00, PAL: 0x0018d588
GlobalSettings::GlobalSettings()
    : mDefaultMacros(MetKeyboardScreen::GetDefaultMacros()), mTutorialComplete(0),
      mTeamFreqUnlocked(0) {
    mNetAddress = kDefaultNetAddress;
    mNetPort = Rnd::MakeString(kPortFormat, kDefaultNetPort);

    int nCount = mDefaultMacros->size();
    mMacros.resize(nCount);
    for (int i = 0; i < nCount; ++i) {
        mMacros[i] = (*mDefaultMacros)[i];
    }

    mCardSlots.clear();
    MemcardConnectState slot;
    slot.mSlotName = kDefaultCardSlotName;
    slot.mPortSlot = kDefaultCardSlotPort;
    slot.mFormatted = 0;
    slot.mFree = 0;
    mCardSlots.push_back(slot);

    mRequiredSaveSpace = kDefaultRequiredSaveSpace;
    mMinimumFreeClusters = kDefaultMinimumFreeClusters;
    mPersonaMinimumFreeClusters = kDefaultPersonaMinimumFreeClusters;
}

// NTSC-U/C: 0x00188370, PAL: 0x0018dc90
GlobalSettings::~GlobalSettings() {
}

// NTSC-U/C: 0x001885d0, PAL: 0x0018df28
void GlobalSettings::Save(OBStream &stream) {
    int nVersion = kRecordVersion;
    OBStream &out = WriteString(
        WriteString(stream.WriteLE(&nVersion, sizeof(nVersion)), mNetAddress), mNetPort);
    for (int i = 0; i < kControllerCount; ++i) {
        mControllers[i].Save(out);
    }
    mGameOptions.Save(out);

    int nCount = mMacros.size();
    stream.WriteLE(&nCount, sizeof(nCount));
    for (std::vector<HxStr>::iterator it = mMacros.begin(); it != mMacros.end(); ++it) {
        WriteString(stream, *it);
    }
    stream << mTutorialComplete;
}

// NTSC-U/C: 0x001887d0, PAL: 0x0018e128
void GlobalSettings::Load(IBStream &stream) {
    int nVersion;
    stream.ReadLE(&nVersion, sizeof(nVersion));
    ReadString(stream, mNetAddress);
    if (nVersion < kPortVersion) {
        HxStr discarded;
        ReadString(stream, discarded);
        mNetPort = Rnd::MakeString(kPortFormat, kDefaultNetPort);
    } else {
        ReadString(stream, mNetPort);
    }

    mControllers[0].Load(stream);
    if (nVersion >= kMultipleControllersVersion) {
        for (int i = 1; i < kControllerCount; ++i) {
            mControllers[i].Load(stream);
        }
    }
    mGameOptions.Load(stream);

    if (nVersion < kMacrosVersion) {
        SkipLegacyMacros(stream);
    } else {
        int nCount;
        stream.ReadLE(&nCount, sizeof(nCount));
        mMacros.resize(nCount);
        for (std::vector<HxStr>::iterator it = mMacros.begin(); it != mMacros.end(); ++it) {
            ReadString(stream, *it);
        }
    }

    if (nVersion >= kTutorialVersion) {
        stream >> mTutorialComplete;
    }
}

// NTSC-U/C: 0x00188b90, PAL: 0x0018e550
void GlobalSettings::SkipLegacyMacros(IBStream &stream) {
    int nCount;
    stream.ReadLE(&nCount, sizeof(nCount));
    HxStr macro;
    for (int i = 0; i < nCount; ++i) {
        int nUnused;
        stream.ReadLE(&nUnused, sizeof(nUnused));
        ReadString(stream, macro);
    }
}

// NTSC-U/C: 0x00188cc0, PAL: 0x0018e6a0
void GlobalSettings::SetMacros(std::vector<HxStr> macros) {
    int nCount = macros.size();
    for (int i = 0; i < nCount; ++i) {
        mMacros[i] = macros[i];
    }
}

// NTSC-U/C: 0x0018b9c8, PAL: 0x00191458
GlobalSettings *GlobalSettings::shared() {
    return g_pGlobalSettings;
}

// NTSC-U/C: 0x0018b9d8, PAL: 0x00191468
void GlobalSettings::Create() {
    g_pGlobalSettings = new GlobalSettings();
}

// NTSC-U/C: 0x0018ba48, PAL: 0x001914d8
void GlobalSettings::Destroy() {
    delete g_pGlobalSettings;
    g_pGlobalSettings = nullptr;
}

// NTSC-U/C: 0x0018ba90, PAL: 0x00191520
void GlobalSettings::Print(std::ostream &stream) {
    stream << kNetAddressLabel << mNetAddress << kNetPortLabel << mNetPort;
    stream << kControllerLabels[0] << kControllerLabels[1] << kControllerLabels[2]
           << kControllerLabels[3];
    stream << kTutorialLabel << mTutorialComplete;
}

// NTSC-U/C: 0x0018bb50, PAL: 0x001915e0
void GlobalSettings::operator=(const GlobalSettings &other) {
    mNetAddress = other.mNetAddress;
    mNetPort = other.mNetPort;
    for (int i = 0; i < kControllerCount; ++i) {
        mControllers[i] = other.mControllers[i];
    }
    for (int i = 0; i < kCopiedMacroCount; ++i) {
        mMacros[i] = other.mMacros[i];
    }
    mGameOptions = other.mGameOptions;
    mTutorialComplete = other.mTutorialComplete;
}
