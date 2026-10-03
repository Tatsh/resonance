#include "msg/multimusemsg.h"

#include <iostream>

#include "mid/mbt.h"

// NTSC-U/C: 0x003d6e08, PAL: 0x0040ecf8
Message *MultiMuseMsg::New() {
    return new MultiMuseMsg(nullptr);
}

// NTSC-U/C: 0x003dc590, PAL: 0x004149c8
Message *MultiMuseMsg::Clone() {
    return new MultiMuseMsg(*this);
}

// NTSC-U/C: 0x003dc608, PAL: 0x00414a40
int MultiMuseMsg::Type() {
    return g_dwMultiMuseMsgType;
}

// NTSC-U/C: 0x003dc618, PAL: 0x00414a50
const char *MultiMuseMsg::Name() {
    return "MultiMuseMsg";
}

// NTSC-U/C: 0x003e38e8, PAL: 0x0041bc88
MultiMuseMsg::MultiMuseMsg(MultiMuse *pMuse) : mMuse(pMuse) {
    if (pMuse != nullptr) {
        ++pMuse->mRefs;
    }
}

// NTSC-U/C: 0x003e38a8, PAL: 0x0041bc48
MultiMuseMsg::MultiMuseMsg(const MultiMuseMsg &other) : MuseMsg(other), mMuse(other.mMuse) {
    if (mMuse != nullptr) {
        ++mMuse->mRefs;
    }
}

// NTSC-U/C: 0x003e3920, PAL: 0x0041bcc0
MultiMuseMsg::~MultiMuseMsg() {
    if (mMuse != nullptr) {
        mMuse->Release();
    }
}

// NTSC-U/C: 0x003e3990, PAL: 0x0041bd30
void MultiMuseMsg::Print(std::ostream &stream) {
    // MuseMsg's member is a Mid::MBT rather than a plain int, which this body proves by handing
    // it to Mid::MBT::Print(). Its header still types it as an int.
    Mid::MBT position;
    position.mTick = mTick;
    position.Print(stream);
    mMuse->Print(stream << " ");
}

// NTSC-U/C: 0x003e39f8, PAL: 0x0041bd98
void MultiMuseMsg::Save(OBStream &stream) {
    mMuse->SaveFields(stream);
}

// NTSC-U/C: 0x003d8180, PAL: 0x004102e8
void MultiMuseMsg::Load(IBStream &stream) {
    // Whatever sequence the message already stored is replaced without being released.
    mMuse = new MultiMuse();
    mMuse->LoadFields(stream);
}
