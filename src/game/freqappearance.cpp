#include "game/freqappearance.h"

#include <iostream>

namespace {

// Written by Save() and read back by Load() into a local that nothing consults.
constexpr int kRecordVersion = 8;

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
