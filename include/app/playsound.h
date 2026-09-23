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

#include "app/hudutil.h"

class HxStr;

/**
 * Look up the synthesiser notes, the velocity, and the auto-stop flag registered under one name.
 *
 * The four outputs start at -1, -1, 127, and 0, and a name in the table overwrites the ones it
 * sets. Most names set one note. `SND_MET_MUSIC1`, `SND_MET_MUSIC2`, and `SND_MET_SLIDE` set a
 * second note, the two music names also set the velocity to 75, and the in-game deploy, erase
 * section, caught powerup, win, and crippler-hit names set the auto-stop flag. A name outside the
 * table leaves all four at their starting values. PlaySoundByName() and StopSoundByName() are the
 * callers. The title is inferred.
 *
 * @param name The registered name.
 * @param pNote Receives the note, or is left at -1.
 * @param pNote2 Receives the second note, or is left at -1.
 * @param pVelocity Receives the velocity, or is left at 127.
 * @param pAutoStop Receives 1 for a sound that releases itself, or is left at 0.
 * @ghidraAddress 0x0012e570
 */
void LookupSound(const HxStr &name, int *pNote, int *pNote2, int *pVelocity, int *pAutoStop);

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

/**
 * Silence the sound registered under one name.
 *
 * Looks the name up through LookupSound() and sends a note-off for each of its notes to
 * Globals::GetSynth() on the last MIDI channel. The first note is skipped when it is 1, not when it
 * is -1 as for the second, which is what the binary compares against. MetRenderer::OnUnknownSlot5()
 * is the one caller, and it stops `SND_MET_MUSIC1`. The title is inferred from the note-off.
 *
 * @param pszName The registered name.
 * @ghidraAddress 0x0012eba0
 */
void StopSoundByName(const char *pszName);

/**
 * Play one sound of the hardware synthesiser.
 *
 * Starts the sound through slot 9 of Globals::GetSynth() with the velocity, and when bAutoStop is
 * set queues its release 480 ticks later on the song clock. Not reconstructed, because the queue
 * the release goes into at `0x00669538` is unrecovered. The title is inferred.
 *
 * @param nSound The sound number.
 * @param nUnknown The second argument. Every recovered caller passes -1.
 * @param nVelocity The velocity, from 0 to 127.
 * @param bAutoStop Non-zero to release the sound after 480 ticks.
 * @ghidraAddress 0x0012ea50
 */
void PlaySynthSound(int nSound, int nUnknown, int nVelocity, int bAutoStop);

/**
 * Play the sound of a captured powerup.
 *
 * The neutralizer, crippler, freestyler, autocatcher, bumper, and multiplier each have a sound.
 * The effect powerups and the guides have none. LocalPlayer::HandleMessage() and the routine at
 * `0x00122ab8` are the callers. The title is inferred.
 *
 * @param kind The captured powerup.
 * @ghidraAddress 0x0012f520
 */
void PlayPowerupSound(HudItemKind kind);
