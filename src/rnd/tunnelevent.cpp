#include "rnd/tunnelevent.h"

#include "os/hxstr.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/tunnel.h"

namespace Rnd {

namespace {

// The first stream revision that stores mUser.
constexpr int kUserRevision = 32;

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    const char *pszName = pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
    stream.WriteBytes(pszName, pObject->mName.mLen + 1);
}

template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

} // namespace

// 0x0046dd40
void TunnelEvent::Save(Stream &stream) const {
    WriteObjectRef(stream, mObject);
    stream.Write(&mFrame, sizeof(mFrame)).Write(&mId, sizeof(mId)).Write(&mUser, sizeof(mUser));
}

// 0x0046de48
void TunnelEvent::Load(Stream &stream) {
    mUser = 0;
    ReadObjectRef(stream, mObject);
    stream.Read(&mFrame, sizeof(mFrame)).Read(&mId, sizeof(mId));
    if (g_nTunnelLoadVersion >= kUserRevision) {
        stream.Read(&mUser, sizeof(mUser));
    }
}

// 0x00477630
void TunnelEvent::Replace(Object *pFrom, Object *pTo, Object *pReferrer) {
    if (mObject == pFrom && mObject != nullptr) {
        pFrom->RemoveRef(pReferrer);
        mObject = dynamic_cast<Drawable *>(pTo);
        if (mObject != nullptr) {
            mObject->AddRef(pReferrer);
        }
    }
}

} // namespace Rnd
