#pragma once

/**
 * @file
 *
 * One-shot sound playback by registered name.
 *
 * The PlayStation 2 body lives in `AppPlaySoundPS2.cpp`, which the mangled name of a file-scope
 * class in the same translation unit records at `0x0012f190`. The declaration sits here rather than
 * in a platform header, so a port supplies its own body for the same prototype.
 */

/**
 * Play the sound registered under one name.
 *
 * The routine wraps the name in an HxStr, builds a four-word request on the stack, and hands it to
 * the request builder at `0x0012ea50`. Every metagame screen sound arrives at the mixer through
 * this one entry point.
 *
 * @param pszName The registered name, such as `SND_MET_SLIDE`.
 * @ghidraAddress 0x0012f470
 */
void PlaySoundByName(const char *pszName);
