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
constexpr int kDefaultUnknown6c = 256;
constexpr int kDefaultUnknown70 = 60;
constexpr int kDefaultUnknown74 = 24;

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

// Writes a string as its length and then its bytes, and reports the stream WriteBytes() reports.
inline OBStream &WriteString(OBStream &stream, const HxStr &text) {
    int nLength = text.mLen;
    stream.Write(&nLength, sizeof(nLength));
    return stream.WriteBytes(TextOf(text), nLength);
}

// Reads a string WriteString() wrote.
inline void ReadString(IBStream &stream, HxStr &text) {
    int nLength;
    stream.Read(&nLength, sizeof(nLength));
    text.Alloc(nLength);
    stream.ReadBytes(text.mStr != nullptr ? text.mStr : const_cast<char *>(g_szEmptyString),
                     nLength);
}

} // namespace

// 0x0018b988
void *GlobalSettings::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "GlobalSettings");
}

// 0x0018b9a8
void GlobalSettings::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "GlobalSettings");
}

// 0x00187d00
GlobalSettings::GlobalSettings()
    : mDefaultMacros(MetKeyboardScreen::GetDefaultMacros()), mTutorialComplete(0), mUnknown78(0) {
    mNetAddress = kDefaultNetAddress;
    mNetPort = FormatString(kPortFormat, kDefaultNetPort);

    int nCount = mDefaultMacros->size();
    mMacros.resize(nCount);
    for (int i = 0; i < nCount; ++i) {
        mMacros[i] = (*mDefaultMacros)[i];
    }

    mCardSlots.clear();
    CardSlot slot;
    slot.mName = kDefaultCardSlotName;
    slot.mPortSlot = kDefaultCardSlotPort;
    slot.mUnknown14 = 0;
    slot.mUnknown0c = 0;
    mCardSlots.push_back(slot);

    mUnknown6c = kDefaultUnknown6c;
    mUnknown70 = kDefaultUnknown70;
    mUnknown74 = kDefaultUnknown74;
}

// 0x00188370
GlobalSettings::~GlobalSettings() {
}

// 0x001885d0
void GlobalSettings::Save(OBStream &stream) {
    int nVersion = kRecordVersion;
    OBStream &out =
        WriteString(WriteString(stream.Write(&nVersion, sizeof(nVersion)), mNetAddress), mNetPort);
    for (int i = 0; i < kControllerCount; ++i) {
        mControllers[i].Save(out);
    }
    mGameOptions.Save(out);

    int nCount = mMacros.size();
    stream.Write(&nCount, sizeof(nCount));
    for (std::vector<HxStr>::iterator it = mMacros.begin(); it != mMacros.end(); ++it) {
        WriteString(stream, *it);
    }
    stream << mTutorialComplete;
}

// 0x001887d0
void GlobalSettings::Load(IBStream &stream) {
    int nVersion;
    stream.Read(&nVersion, sizeof(nVersion));
    ReadString(stream, mNetAddress);
    if (nVersion < kPortVersion) {
        HxStr discarded;
        ReadString(stream, discarded);
        mNetPort = FormatString(kPortFormat, kDefaultNetPort);
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
        stream.Read(&nCount, sizeof(nCount));
        mMacros.resize(nCount);
        for (std::vector<HxStr>::iterator it = mMacros.begin(); it != mMacros.end(); ++it) {
            ReadString(stream, *it);
        }
    }

    if (nVersion >= kTutorialVersion) {
        stream >> mTutorialComplete;
    }
}

// 0x00188b90
void GlobalSettings::SkipLegacyMacros(IBStream &stream) {
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    HxStr macro;
    for (int i = 0; i < nCount; ++i) {
        int nUnused;
        stream.Read(&nUnused, sizeof(nUnused));
        ReadString(stream, macro);
    }
}

// 0x00188cc0
void GlobalSettings::SetMacros(std::vector<HxStr> macros) {
    int nCount = macros.size();
    for (int i = 0; i < nCount; ++i) {
        mMacros[i] = macros[i];
    }
}

// 0x0018b9c8
GlobalSettings *GlobalSettings::shared() {
    return g_pGlobalSettings;
}

// 0x0018b9d8
void GlobalSettings::Create() {
    g_pGlobalSettings = new GlobalSettings();
}

// 0x0018ba48
void GlobalSettings::Destroy() {
    delete g_pGlobalSettings;
    g_pGlobalSettings = nullptr;
}

// 0x0018ba90
void GlobalSettings::Print(std::ostream &stream) {
    stream << kNetAddressLabel << mNetAddress << kNetPortLabel << mNetPort;
    stream << kControllerLabels[0] << kControllerLabels[1] << kControllerLabels[2]
           << kControllerLabels[3];
    stream << kTutorialLabel << mTutorialComplete;
}

// 0x0018bb50
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
