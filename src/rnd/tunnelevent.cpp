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
        stream.Write(&chTerminator, 1);
        return;
    }
    const char *pszName = pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
    stream.Write(pszName, pObject->mName.mLen + 1);
}

template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(TheManager.Find(name));
}

} // namespace

// NTSC-U/C: 0x0046dd40, PAL: 0x004ab880
void TunnelEvent::Save(Stream &stream) const {
    WriteObjectRef(stream, mObject);
    stream.WriteLE(&mFrame, sizeof(mFrame))
        .WriteLE(&mId, sizeof(mId))
        .WriteLE(&mUser, sizeof(mUser));
}

// NTSC-U/C: 0x0046de48, PAL: 0x004ab988
void TunnelEvent::Load(Stream &stream) {
    mUser = 0;
    ReadObjectRef(stream, mObject);
    stream.ReadLE(&mFrame, sizeof(mFrame)).ReadLE(&mId, sizeof(mId));
    if (g_nTunnelLoadVersion >= kUserRevision) {
        stream.ReadLE(&mUser, sizeof(mUser));
    }
}

// NTSC-U/C: 0x00477630, PAL: 0x004b52a8
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
