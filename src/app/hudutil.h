#pragma once

#include "math/color.h"
#include "os/hxstr.h"

/**
 * Kinds of item the head-up display labels, in the order HudPowerupName() maps them.
 *
 * The first five are the powerups and the thirteenth is the multiplier, and HudPowerup selects a
 * view for exactly those six. The seven between are the effect names the track display prints.
 * The enumerator names follow the literals HudPowerupName() returns.
 */
enum HudItemKind {
    kHudItemNone = -1,       /*!< No item. HudPowerup shows nothing. */
    kHudItemNeutralizer = 0, /*!< `NEUTRALIZER`. */
    kHudItemCrippler = 1,    /*!< `CRIPPLER`. */
    kHudItemFreestyler = 2,  /*!< `FREESTYLER`. */
    kHudItemAutocatcher = 3, /*!< `AUTOCATCHER`. */
    kHudItemBumper = 4,      /*!< `BUMPER`. */
    kHudItemVolume = 5,      /*!< `Volume`. */
    kHudItemWah = 6,         /*!< `Wah`. */
    kHudItemStutter = 7,     /*!< `Stutter`. */
    kHudItemEcho = 8,        /*!< `Echo`. */
    kHudItemFlange = 9,      /*!< `Flange`. */
    kHudItemChorus = 10,     /*!< `Chorus`. */
    kHudItemGuides = 11,     /*!< `Guides`. */
    kHudItemMultiplier = 12, /*!< `MULTIPLIER`. */
};

/**
 * Report the display name of one item kind.
 *
 * Any value outside the thirteen kinds yields the empty string. The routine is a free function of
 * the head-up display's translation unit. Its callers are the track display's effect labels and
 * two of Overlay's message handlers. The title is inferred.
 *
 * @param nKind The item kind.
 * @return The name.
 * @ghidraAddress 0x00415ed8
 */
HxStr HudPowerupName(int nKind);

/**
 * Report the colour of one player colour name.
 *
 * `green`, `red`, `yellow`, `purple`, and `null` each select a fixed colour, and any other name
 * selects cyan. Every result is opaque. The title is inferred.
 *
 * Both callers copy the name into a temporary and pass its address, then release it after the
 * call. That is the by-value convention for a class with a copy constructor.
 *
 * @param name The colour name, as a player records it.
 * @return The colour.
 * @ghidraAddress 0x00416068
 */
Color HudColorFromName(HxStr name);

/**
 * Produce the next unique object name of the form `<hud%04d>`.
 *
 * Advances g_nHudNameCounter first, so the first name is `<hud0001>`. HudPosition's constructor
 * inlines the body twice, and this copy has no caller. The title is inferred.
 *
 * @return The name.
 * @ghidraAddress 0x004298e8
 */
HxStr NextHudName();

/**
 * Count of names NextHudName() has produced.
 *
 * @ghidraAddress 0x006dfde8
 */
extern int g_nHudNameCounter;
