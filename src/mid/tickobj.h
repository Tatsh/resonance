#pragma once

#include "mid/mbt.h"

/**
 * One value at one song position.
 *
 * The template is attested by the mangled name `t9Sequencer1ZPCt7TickObj1ZP7MuseMsg` at
 * `0x008eec48`, which demangles to `Sequencer<TickObj<MuseMsg *> const *>`. That is the one
 * instantiation the image has, and MultiMuse's vector is an array of it.
 *
 * The layout comes from MultiMuse::SaveFields(), which advances eight bytes per element and writes
 * the first word through Mid::MBT::Save() and the second as a message pointer. The class emits no
 * descriptor of its own, which is consistent with it having no virtual member.
 *
 * Both members are public because MultiMuse::SaveFields() and MultiMusePlayer::Start() read them
 * directly and the image exposes no accessor.
 */
template <typename T>
struct TickObj {
    Mid::MBT mPosition; /*!< Song position, in MIDI ticks. +0x00 */
    T mValue;           /*!< The value at that position. +0x04 */
};
