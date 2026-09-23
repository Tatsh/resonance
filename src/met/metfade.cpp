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

} // namespace

// 0x0016a1c8
MetFade::MetFade(MetRenderer *pRenderer) : renderer_(pRenderer) {
    state_ = kStateIdle;
    inStart_ = kIdleFrame;
    outStart_ = kIdleFrame;
    rect_ = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kRectName)));
    rect_->SetShowing(0);
}

// 0x0016a608
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

// 0x0016d750
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

// 0x0016d710
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

// 0x0016a2d0
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

// 0x0016a468
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
