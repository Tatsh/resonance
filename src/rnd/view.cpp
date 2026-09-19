#include "rnd/view.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"

namespace Rnd {

// 0x00702b20
HxStr g_viewClassName("View");

// 0x004e32a0. The thunk registered against the "View" key. The null test is the conversion of a
// View pointer to its virtual Rnd::Object base rather than a check the source asks for.
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

View *View::NewView(const HxStr &name) {
    return new View(name);
}

void View::Init() {
    g_manager.RegisterClass(g_viewClassName, NewViewObject);
    g_manager.RegisterClass(HxStr("Animatable"), NewAnimatableView);
    g_manager.RegisterClass(HxStr("Collideable"), NewCollideableView);
    g_manager.RegisterClass(HxStr("Drawable"), NewDrawableView);
    g_manager.RegisterClass(HxStr("Transformable"), NewTransformableView);
}

} // namespace Rnd
