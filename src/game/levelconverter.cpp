#include "game/levelconverter.h"

#include "game/levelbuilder.h"
#include "mid/mbt.h"

namespace {

// MIDI meta type 3 is the track name, which is the one text event this class acts on.
constexpr unsigned char kTrackNameMetaType = 3;

} // namespace

// 0x001e6278. The three HxStr members and the six collections are default-constructed by the
// expansions the compiler places ahead of and around these stores.
LevelConverter::LevelConverter()
    : mUnknown5c(kMBTInfinity), mUnknown60(kMBTInfinity), mUnknown88(kMBTInfinity), mUnknown90(0),
      mUnknown94(0) {
}

// 0x001e9ee0. Every statement in the body is the compiler expanding the destructor of a member,
// the three span collections and the name map first, then the pending-event collection, then the
// three strings in reverse declaration order.
LevelConverter::~LevelConverter() {
}

// 0x001ea570
void LevelConverter::Tempo(int nTick, int nMicrosecondsPerQuarter) {
    mBuilder->SetTempo(nTick, nMicrosecondsPerQuarter);
    mHasTempo = 1;
}

// 0x001ea5a0
void LevelConverter::TextEvent(int nTick, const char *pText, unsigned char nType) {
    if (nTick == Mid::MBT(0).mTick && nType == kTrackNameMetaType) {
        ParseTrackTypeString(pText);
    }
}
