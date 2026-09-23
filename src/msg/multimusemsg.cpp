#include "msg/multimusemsg.h"

#include <iostream>

#include "mid/mbt.h"

// 0x003d6e08
MultiMuseMsg *MultiMuseMsg::New() {
    return new MultiMuseMsg(nullptr);
}

// 0x003dc590
Message *MultiMuseMsg::Clone() {
    return new MultiMuseMsg(*this);
}

// 0x003dc608
int MultiMuseMsg::Type() {
    return g_dwMultiMuseMsgType;
}

// 0x003dc618
const char *MultiMuseMsg::Name() {
    return "MultiMuseMsg";
}

// 0x003e38e8
MultiMuseMsg::MultiMuseMsg(MultiMuse *pMuse) : mMuse(pMuse) {
    if (pMuse != nullptr) {
        ++pMuse->mRefs;
    }
}

// 0x003e38a8
MultiMuseMsg::MultiMuseMsg(const MultiMuseMsg &other) : MuseMsg(other), mMuse(other.mMuse) {
    if (mMuse != nullptr) {
        ++mMuse->mRefs;
    }
}

// 0x003e3920
MultiMuseMsg::~MultiMuseMsg() {
    if (mMuse != nullptr) {
        mMuse->Release();
    }
}

// 0x003e3990
void MultiMuseMsg::Print(std::ostream &stream) {
    // MuseMsg's member is a Mid::MBT rather than a plain int, which this body proves by handing
    // it to Mid::MBT::Print(). Its header still types it as an int.
    Mid::MBT position;
    position.mTick = mTick;
    position.Print(stream);
    mMuse->Print(stream << " ");
}

// 0x003e39f8
void MultiMuseMsg::Save(OBStream &stream) {
    mMuse->SaveFields(stream);
}

// 0x003d8180
void MultiMuseMsg::Load(IBStream &stream) {
    // Whatever sequence the message already stored is replaced without being released.
    mMuse = new MultiMuse();
    mMuse->LoadFields(stream);
}
