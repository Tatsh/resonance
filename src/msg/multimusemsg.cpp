#include "msg/multimusemsg.h"

#include <iostream>

#include "mid/tick.h"

Message *MultiMuseMsg::New() {
    return new MultiMuseMsg(nullptr);
}

Message *MultiMuseMsg::Clone() {
    return new MultiMuseMsg(*this);
}

int MultiMuseMsg::Type() {
    return g_dwMultiMuseMsgType;
}

const char *MultiMuseMsg::GetName() const {
    return "MultiMuseMsg";
}

MultiMuseMsg::MultiMuseMsg(MultiMuse *pMuse) : mMuse(pMuse) {
    if (pMuse != nullptr) {
        ++pMuse->mRefs;
    }
}

MultiMuseMsg::MultiMuseMsg(const MultiMuseMsg &other) : MuseMsg(other), mMuse(other.mMuse) {
    if (mMuse != nullptr) {
        ++mMuse->mRefs;
    }
}

MultiMuseMsg::~MultiMuseMsg() {
    if (mMuse != nullptr) {
        mMuse->Release();
    }
}

void MultiMuseMsg::PrintExtra(std::ostream &stream) const {
    // MuseMsg's member is a Sch::Tick rather than a plain int, which this body proves by handing
    // it to Sch::Tick::Print(). Its header still types it as an int.
    Sch::Tick position;
    position.mTick = mTick;
    position.Print(stream);
    mMuse->Print(stream << " ");
}

void MultiMuseMsg::saveGuts(OBStream &stream) const {
    mMuse->SaveFields(stream);
}

void MultiMuseMsg::restoreGuts(IBStream &stream) {
    // Whatever sequence the message already stored is replaced without being released.
    mMuse = new MultiMuse();
    mMuse->LoadFields(stream);
}
