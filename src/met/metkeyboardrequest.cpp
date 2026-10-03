#include "met/metkeyboardrequest.h"

#include <climits>

namespace {

// The values the constructor starts the limits and the unread word at.
constexpr int kDefaultUnusedFlag = 1;
constexpr int kDefaultMaxWidth = 500;
constexpr int kNoMaxLength = INT_MAX;

} // namespace

// NTSC-U/C: 0x0028cf18, PAL: 0x002a25f8
MetKeyboardRequest::MetKeyboardRequest(
    const HxStr &returnScreen, const HxStr &prompt, const HxStr &text, int nPad, MetKBUser *pUser)
    : mReturnScreen(returnScreen), mPrompt(prompt), mText(text), mUser(pUser),
      mUnusedFlag(kDefaultUnusedFlag), mMaxWidth(kDefaultMaxWidth), mMaxLength(kNoMaxLength),
      mMacros(nullptr), mPad(nPad) {
}
