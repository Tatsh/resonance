#include "met/metkeyboardrequest.h"

#include <climits>

namespace {

// The values the constructor starts the limits and the unread word at.
constexpr int kDefaultUnknown1c = 1;
constexpr int kDefaultMaxWidth = 500;
constexpr int kNoMaxLength = INT_MAX;

} // namespace

// 0x0028cf18
MetKeyboardRequest::MetKeyboardRequest(
    const HxStr &returnScreen, const HxStr &prompt, const HxStr &text, int nPad, MetKBUser *pUser)
    : mReturnScreen(returnScreen), mPrompt(prompt), mText(text), mUser(pUser),
      mUnknown1c(kDefaultUnknown1c), mMaxWidth(kDefaultMaxWidth), mMaxLength(kNoMaxLength),
      mMacros(nullptr), mPad(nPad) {
}
