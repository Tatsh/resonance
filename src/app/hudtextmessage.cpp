#include "app/hudtextmessage.h"

#include "os/hxstr.h"
#include "rnd/blur.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/transanim.h"

namespace {

// Start time that marks the message idle.
constexpr float kMessageIdle = 0.0f;

// Start time that marks a start not yet recorded.
constexpr float kMessageStartPending = 1.0e9f;

// Length of each fade, and the animation frame the fade-in ends on.
constexpr float kFadeLength = 250.0f;

} // namespace

HudTextMessage::HudTextMessage(const HxStr &name)
    : mText(nullptr), mStart(kMessageIdle), mActive(0) {
    mBlur = dynamic_cast<Rnd::Blur *>(Rnd::g_manager.Find(name + ".blur"));
    mText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(name + ".txt"));
    mAnim = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(name + ".tnm"));

    mFont = mText->GetFont();
    mFontSize = mFont->mSize;
    mText->SetShowing(0);
    if (mBlur != nullptr) {
        mBlur->SetShowing(1);
    }
}

// 0x00429b58
void HudTextMessage::Show(const HxStr &text, float flScale, float flHold) {
    if (mActive != 0) {
        return;
    }

    if (mBlur != nullptr) {
        mBlur->mXfms.clear();
    }
    mFont->SetSize(mFontSize * flScale);
    mText->SetText(text);
    mText->SetShowing(1);
    mHold = flHold;
    mStart = kMessageStartPending;
    mAnim->SetFrame(0.0f);
}

// 0x00429af8
void HudTextMessage::Hide() {
    mText->SetShowing(0);
    mActive = 0;
}

// 0x00429c20
void HudTextMessage::SetFrame(float flTime) {
    if (mStart == kMessageIdle) {
        return;
    }

    if (mStart == kMessageStartPending) {
        mStart = flTime;
    }

    const float flElapsed = flTime - mStart;
    if (flElapsed < kFadeLength) {
        mAnim->SetFrame(flElapsed);
    } else if (flElapsed < mHold) {
        mAnim->SetFrame(kFadeLength);
    } else {
        mAnim->SetFrame(flElapsed - mHold + kFadeLength);
    }

    if (flElapsed - mHold > kFadeLength) {
        mActive = 0;
        mStart = kMessageIdle;
        mText->SetShowing(0);
    }
}
