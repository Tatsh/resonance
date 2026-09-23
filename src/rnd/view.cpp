#include "rnd/view.h"

#include "os/failsink.h"
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

// 0x00702b20
HxStr g_viewClassName("View");

// 0x004e32a0
// The thunk registered against the "View" key. The null test is the conversion of a View pointer
// to its virtual Rnd::Object base rather than a check the source asks for.
static Object *NewViewObject(const HxStr &name) {
    return View::NewView(name);
}

// 0x004e3458
static Object *NewAnimatableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mAnimatable = 1;
    return pView;
}

// 0x004e3378
static Object *NewTransformableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mTransformable = 1;
    return pView;
}

// 0x004e3538
static Object *NewDrawableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mDrawable = 1;
    return pView;
}

// 0x004e3618
static Object *NewCollideableView(const HxStr &name) {
    View *pView = View::NewView(name);
    pView->mCollideable = 1;
    return pView;
}

// 0x004e2048
void *View::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kViewTag);
}

// 0x004e2068
void View::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kViewTag);
}

// 0x004e2740
View::View(const HxStr &name)
    : Object(name), mAnimatable(0), mTransformable(0), mDrawable(0), mCollideable(0) {
}

// 0x004e21b8
View::~View() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// 0x004e2730
void View::RemoveObjectRefs() {
}

// 0x004e2720
const HxStr &View::ClassName() const {
    return g_viewClassName;
}

// 0x004e3768
void View::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    Transformable::DumpText(sink);
    Drawable::DumpText(sink);
    Collideable::DumpText(sink);
}

// 0x004e37d0
void View::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));
    Animatable::Save(stream);
    Transformable::Save(stream);
    Drawable::Save(stream);
    Collideable::Save(stream);
}

// 0x004e36f8
void View::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);
    Transformable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);
}

// 0x004e3850
void View::Copy(const Object *pSource, unsigned nFlags) {
    (void)dynamic_cast<const View *>(pSource); // Yes, the binary discards the cast.
    Animatable::Copy(pSource, nFlags);
    Transformable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);
    Collideable::Copy(pSource, nFlags);
}

// 0x004e0128
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
    stream.Read(&nVersion, sizeof(nVersion));
    if (nVersion > kSerialVersion) {
        g_failSink.Report("Can't load new View\n");
        return;
    }

    Animatable::Load(stream);
    Transformable::Load(stream);
    Drawable::Load(stream);
    Collideable::Load(stream);
    if (nVersion < kCameraRefRevision) {
        HxStr name(nullptr);
        stream.ReadString(name);
        (void)dynamic_cast<Cam *>(g_manager.Find(name)); // Yes, the binary discards the camera.
    }
}

// 0x004e2088
// The five registered creators above inline this body, handler and all, and set their flag after
// it even when it produced null.
View *View::NewView(const HxStr &name) {
    try {
        return new View(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// 0x004dff48
void View::Init() {
    g_manager.RegisterClass(g_viewClassName, NewViewObject);
    g_manager.RegisterClass(HxStr("Animatable"), NewAnimatableView);
    g_manager.RegisterClass(HxStr("Collideable"), NewCollideableView);
    g_manager.RegisterClass(HxStr("Drawable"), NewDrawableView);
    g_manager.RegisterClass(HxStr("Transformable"), NewTransformableView);
}

} // namespace Rnd
