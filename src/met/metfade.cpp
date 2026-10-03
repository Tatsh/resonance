#include "met/metfade.h"

#include "math/color.h"
#include "met/fadeuser.h"
#include "met/metrenderer.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/view.h"

namespace {

// The start frame of a fade that is not running.
constexpr float kIdleFrame = 1.0e9f;

constexpr char kRectName[] = "metfade.rect";
constexpr char kViewName[] = "met_fade.view";

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x006bab18
// Non-zero from the start of a fade until it finishes. The binary writes it and never reads it.
int g_nFadeRunning = 0;

Rnd::View *FindFadeView() {
    return dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kViewName)));
}
#endif

} // namespace

// NTSC-U/C: 0x0016a1c8, PAL: 0x0016c3c8
MetFade::MetFade(MetRenderer *pRenderer) : renderer_(pRenderer) {
    state_ = kStateIdle;
    inStart_ = kIdleFrame;
    outStart_ = kIdleFrame;
    rect_ = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kRectName)));
    rect_->SetShowing(0);
}

#ifdef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x0016a608, PAL: 0x0016ce48
void MetFade::FadeOut(float duration, float start, FadeUser *pUser, int nRetainView) {
    g_nFadeRunning = 1;
    retainView_ = nRetainView;
    user_ = pUser;
    rect_->SetShowing(1);
    float end = start + duration;
    outStart_ = start;
    state_ = kStateOut;
    outEnd_ = end;
    rate_ = 1.0f / (end - start);
    offset_ = 0.0f - rate_ * start;
    renderer_->RemoveScreenView(FindFadeView());
    renderer_->AddScreenView(FindFadeView());
}

// NTSC-U/C: 0x0016d750, PAL: 0x0016cc30
void MetFade::FadeIn(float duration, float start, FadeUser *pUser, int nRetainView) {
    g_nFadeRunning = 1;
    float end = start + duration;
    inStart_ = start;
    user_ = pUser;
    retainView_ = nRetainView;
    state_ = kStateIn;
    inEnd_ = end;
    rate_ = 1.0f / (end - start);
    offset_ = 0.0f - rate_ * start;
    renderer_->RemoveScreenView(FindFadeView());
    renderer_->AddScreenView(FindFadeView());
}
#else
// NTSC-U/C: 0x0016a608
void MetFade::FadeOut(float duration, float start, FadeUser *pUser, int nRetainView) {
    retainView_ = nRetainView;
    user_ = pUser;
    rect_->SetShowing(1);
    float end = start + duration;
    outStart_ = start;
    state_ = kStateOut;
    outEnd_ = end;
    rate_ = 1.0f / (end - start);
    offset_ = 0.0f - rate_ * start;
    renderer_->AddScreenView(dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kViewName))));
}

// NTSC-U/C: 0x0016d750
void MetFade::FadeIn(float duration, float start, FadeUser *pUser, int nRetainView) {
    float end = start + duration;
    inStart_ = start;
    user_ = pUser;
    retainView_ = nRetainView;
    state_ = kStateIn;
    inEnd_ = end;
    rate_ = 1.0f / (end - start);
    offset_ = 0.0f - rate_ * start;
    renderer_->AddScreenView(dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kViewName))));
}
#endif

// NTSC-U/C: 0x0016d710, PAL: 0x001700e8
void MetFade::Update(float frame) {
    if (state_ == kStateIdle) {
        return;
    }
    if (state_ == kStateOut) {
        UpdateOut(frame);
    } else {
        UpdateIn(frame);
    }
}

#ifdef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x0016a2d0, PAL: 0x0016c4f0
void MetFade::UpdateOut(float frame) {
    if (!(outEnd_ <= frame)) {
        Color color{0.0f, 0.0f, 0.0f, 1.0f - (rate_ * frame + offset_)};
        rect_->SetVertexColor(color);
        rect_->SetShowing(1);
        return;
    }
    Color color{0.0f, 0.0f, 0.0f, 1.0f - (rate_ * outEnd_ + offset_)};
    rect_->SetVertexColor(color);
    rect_->SetShowing(1);
    state_ = kStateIdle;
    outStart_ = kIdleFrame;
    renderer_->RemoveScreenView(FindFadeView());
    rect_->SetShowing(0);
    g_nFadeRunning = 0;
    if (user_ != nullptr) {
        user_->OnFadeOutDone();
    }
}

// NTSC-U/C: 0x0016a468, PAL: 0x0016c708
void MetFade::UpdateIn(float frame) {
    if (!(inEnd_ <= frame)) {
        Color color{0.0f, 0.0f, 0.0f, rate_ * frame + offset_};
        rect_->SetVertexColor(color);
        rect_->SetShowing(1);
        return;
    }
    Color color{0.0f, 0.0f, 0.0f, rate_ * inEnd_ + offset_};
    rect_->SetVertexColor(color);
    rect_->SetShowing(1);
    inStart_ = kIdleFrame;
    state_ = kStateIdle;
    if (retainView_ == 0) {
        renderer_->RemoveScreenView(FindFadeView());
        rect_->SetShowing(0);
    }
    g_nFadeRunning = 0;
    if (user_ != nullptr) {
        user_->OnFadeInDone();
    }
}

// PAL: 0x0016c910
void MetFade::Detach() {
    renderer_->RemoveScreenView(FindFadeView());
    rect_->SetShowing(0);
}

// PAL: 0x0016ca20
void MetFade::ShowOpaque() {
    renderer_->RemoveScreenView(FindFadeView());
    renderer_->AddScreenView(FindFadeView());
    Color color{0.0f, 0.0f, 0.0f, 1.0f};
    rect_->SetVertexColor(color);
    rect_->SetShowing(1);
}

// PAL: 0x00170128
float MetFade::GetCoverage() const {
    if (rect_->GetShowing() == 0) {
        return 0.0f;
    }
    return 1.0f - rect_->mVertsOwner->mVerts[0].mColor.a;
}
#else
// NTSC-U/C: 0x0016a2d0
void MetFade::UpdateOut(float frame) {
    if (outEnd_ <= frame) {
        state_ = kStateIdle;
        frame = outEnd_;
        outStart_ = kIdleFrame;
        renderer_->RemoveScreenView(
            dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kViewName))));
        rect_->SetShowing(0);
        if (user_ != nullptr) {
            user_->OnFadeOutDone();
        }
    }
    float alpha = rate_ * frame + offset_;
    Color color{0.0f, 0.0f, 0.0f, 1.0f - alpha};
    rect_->SetVertexColor(color);
    rect_->SetShowing(1); // Yes, the binary shows again a rectangle that a finished fade hid.
}

// NTSC-U/C: 0x0016a468
void MetFade::UpdateIn(float frame) {
    if (inEnd_ <= frame) {
        frame = inEnd_;
        inStart_ = kIdleFrame;
        state_ = kStateIdle;
        if (retainView_ == 0) {
            renderer_->RemoveScreenView(
                dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kViewName))));
            rect_->SetShowing(0);
        }
        if (user_ != nullptr) {
            user_->OnFadeInDone();
        }
    }
    Color color{0.0f, 0.0f, 0.0f, rate_ * frame + offset_};
    rect_->SetVertexColor(color);
    rect_->SetShowing(1); // Yes, the binary shows again a rectangle that a finished fade hid.
}
#endif
