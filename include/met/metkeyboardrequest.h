#pragma once

#include <vector>

#include "os/hxstr.h"

class MetKBUser;

/**
 * What a screen asks of the on-screen keyboard.
 *
 * A plain record of 0x38 bytes with no RTTI and no string in the image naming it, so the name is
 * inferred from its one consumer, MetKeyboardScreen::Open(). A screen builds one on its stack,
 * adjusts the limits and the ticker text, and passes it to Open(), which copies every field except
 * mUnknown1c into the keyboard screen. MetSaveRemixScreen, MetRemixDelScreen, and
 * MetLoadNewFreqScreen build one.
 */
struct MetKeyboardRequest {
    /**
     * Fill the request, with no macro list, an empty ticker, and no limits.
     *
     * The out-of-line routine returns its receiver.
     *
     * @param returnScreen The registry key of the screen the keyboard returns to.
     * @param prompt The prompt the keyboard shows.
     * @param text The text the entry starts from.
     * @param nPad The one controller the keyboard accepts, or -1 for any.
     * @param pUser The receiver of the committed text.
     * @ghidraAddress 0x0028cf18
     */
    MetKeyboardRequest(const HxStr &returnScreen,
                       const HxStr &prompt,
                       const HxStr &text,
                       int nPad,
                       MetKBUser *pUser);

    HxStr mReturnScreen; /*!< The screen the keyboard returns to. +0x00 */
    HxStr mPrompt;       /*!< The prompt the keyboard shows. +0x08 */
    HxStr mText;         /*!< The text the entry starts from. +0x10 */
    MetKBUser *mUser;    /*!< The receiver of the committed text. +0x18 */
    int mUnknown1c;      /*!< Starts at 1. MetKeyboardScreen::Open() does not read it. +0x1c */
    /**
     * The widest the entered text may measure, compared against Rnd::Text::MeasureText(). Starts
     * at 500. The name is inferred. +0x20
     */
    int mMaxWidth;
    /** The most characters the entry accepts. Starts at no limit. The name is inferred. +0x24 */
    int mMaxLength;
    /** The macro list the keyboard offers, or null for the default list. +0x28 */
    std::vector<HxStr> *mMacros;
    HxStr mTicker; /*!< The ticker text the keyboard posts, empty by default. +0x2c */
    int mPad;      /*!< The one controller the keyboard accepts, or -1 for any. +0x34 */
};
