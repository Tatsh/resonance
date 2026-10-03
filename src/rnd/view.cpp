#include "rnd/view.h"

#include "os/dbg.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/animatable.h"
#include "rnd/cam.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// The allocation tag every view block is billed to.
constexpr char kViewTag[] = "Rnd::View";

// The revision Save() writes, and the highest revision Load() accepts.
constexpr int kSerialVersion = 3;

// Below this revision a view record also names a camera, which the current code reads and drops.
constexpr int kCameraRefRevision = 3;

} // namespace

// NTSC-U/C: 0x00702b20, PAL: 0x007465c0
HxStr g_viewClassName("View");

// NTSC-U/C: 0x004e32a0, PAL: 0x00521b58
// The thunk registered against the "View" key. The null test is the conversion of a View pointer
// to its virtual Rnd::Object base rather than a check the source asks for.
static Object *NewViewObject(const HxStr &name) {
    return View::NewView(name);
}

// NTSC-U/C: 0x004e3458, PAL: 0x00521d10
static Object *NewAnimatableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mAnimatable = 1;
    return pView;
}

// NTSC-U/C: 0x004e3378, PAL: 0x00521c30
static Object *NewTransformableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mTransformable = 1;
    return pView;
}

// NTSC-U/C: 0x004e3538, PAL: 0x00521df0
static Object *NewDrawableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mDrawable = 1;
    return pView;
}

// NTSC-U/C: 0x004e3618, PAL: 0x00521ed0
static Object *NewCollideableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mCollideable = 1;
    return pView;
}

// NTSC-U/C: 0x004e2048, PAL: 0x00520900
void *View::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kViewTag);
}

// NTSC-U/C: 0x004e2068, PAL: 0x00520920
void View::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kViewTag);
}

// NTSC-U/C: 0x004e2740, PAL: 0x00520ff8
View::View(const HxStr &name)
    : Object(name), mAnimatable(0), mTransformable(0), mDrawable(0), mCollideable(0) {
}

// NTSC-U/C: 0x004e21b8, PAL: 0x00520a70
View::~View() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// NTSC-U/C: 0x004e2730, PAL: 0x00520fe8
void View::RemoveObjectRefs() {
}

// NTSC-U/C: 0x004e2720, PAL: 0x00520fd8
const HxStr &View::ClassName() const {
    return g_viewClassName;
}

// NTSC-U/C: 0x004e3768, PAL: 0x00522020
void View::DumpText(Dbg &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    Transformable::DumpText(sink);
    Drawable::DumpText(sink);
    Collideable::DumpText(sink);
}

// NTSC-U/C: 0x004e37d0, PAL: 0x00522088
void View::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.WriteLE(&nVersion, sizeof(nVersion));
    Animatable::Save(stream);
    Transformable::Save(stream);
    Drawable::Save(stream);
    Collideable::Save(stream);
}

// NTSC-U/C: 0x004e36f8, PAL: 0x00521fb0
void View::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);
    Transformable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);
}

// NTSC-U/C: 0x004e3850, PAL: 0x00522108
void View::Copy(const Object *pSource, unsigned nFlags) {
    (void)dynamic_cast<const View *>(pSource); // Yes, the binary discards the cast.
    Animatable::Copy(pSource, nFlags);
    Transformable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);
    Collideable::Copy(pSource, nFlags);
}

// NTSC-U/C: 0x004e0128, PAL: 0x0051e978
void View::Load(Stream &stream) {
    // A view created for a bare mix-in key reads only that mix-in's record.
    if (mAnimatable != 0) {
        Animatable::Load(stream);
        return;
    }
    if (mDrawable != 0) {
        Drawable::Load(stream);
        return;
    }
    if (mCollideable != 0) {
        Collideable::Load(stream);
        return;
    }
    if (mTransformable != 0) {
        Transformable::Load(stream);
        return;
    }

    int nVersion = 0;
    stream.ReadLE(&nVersion, sizeof(nVersion));
    if (nVersion > kSerialVersion) {
        Rnd::TheDbg.Notify("Can't load new View\n");
        return;
    }

    Animatable::Load(stream);
    Transformable::Load(stream);
    Drawable::Load(stream);
    Collideable::Load(stream);
    if (nVersion < kCameraRefRevision) {
        HxStr name(nullptr);
        stream.ReadString(name);
        (void)dynamic_cast<Cam *>(TheManager.Find(name)); // Yes, the binary discards the camera.
    }
}

// NTSC-U/C: 0x004e2088, PAL: 0x00520940
// The five registered creators above inline this body, handler and all, and set their flag after
// it even when it produced null.
View *View::NewView(const HxStr &name) {
    try {
        return new View(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// NTSC-U/C: 0x004dff48, PAL: 0x0051e720
void View::Init() {
    TheManager.RegisterClass(g_viewClassName, NewViewObject);
    TheManager.RegisterClass(HxStr("Animatable"), NewAnimatableView);
    TheManager.RegisterClass(HxStr("Collideable"), NewCollideableView);
    TheManager.RegisterClass(HxStr("Drawable"), NewDrawableView);
    TheManager.RegisterClass(HxStr("Transformable"), NewTransformableView);
}

} // namespace Rnd
