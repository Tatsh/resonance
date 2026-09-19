#include "game/localplayer.h"

#include "app/msgsource.h"

// 0x00121ea0
int LocalPlayer::Slot2() {
    return mUnknown50;
}

// 0x00121e90
int LocalPlayer::Slot4() {
    return mUnknown58;
}

// 0x00121e98
int LocalPlayer::Slot5() {
    return mUnknown5c;
}

// 0x00122890
int LocalPlayer::Slot6() {
    return 0;
}

// 0x00121ea8
void LocalPlayer::Slot7() {
}

// 0x00122898
void LocalPlayer::Slot8(int first, int second) {
    mUnknown70 = first;
    mUnknown74 = second;
}

// 0x00121eb0
int LocalPlayer::Slot10() {
    return mUnknown60;
}

// 0x00121ec0
int LocalPlayer::Slot14() {
    return mUnknownb0 + 1;
}

// 0x00121ed0
int LocalPlayer::Slot15() {
    return mUnknownac + 1;
}

// 0x00122cc8
float LocalPlayer::Slot18() {
    if (mCount9c == 0) {
        return 0.0f;
    }

    return static_cast<float>(mCount9c) / static_cast<float>(mCount9c + mCounta0);
}

// 0x00121ee0
int LocalPlayer::Slot17() {
    return mUnknown88;
}

// 0x00121ee8
int LocalPlayer::Slot19() {
    return mUnknown6c;
}

// 0x001228f8
void LocalPlayer::AddSink(MsgSink *pSink) {
    MsgSource::AddSink(pSink);
    mSourceA8->AddSink(pSink);
    mSourceA4->AddSink(pSink);
}

// 0x00122968
void LocalPlayer::RemoveSink(MsgSink *pSink) {
    MsgSource::RemoveSink(pSink);
    mSourceA8->RemoveSink(pSink);
    mSourceA4->RemoveSink(pSink);
}
