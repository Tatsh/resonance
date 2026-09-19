#pragma once

#include "msg/message.h"
#include "stream/obstream.h"

/**
 * Write a message through a stream, with a flag for the null case.
 *
 * Writes the single byte `0` and returns for a null message. Otherwise it writes the byte `1`,
 * then the message's identity and its payload through the message's own virtuals.
 *
 * The body is not reconstructed. The two bytes are the ASCII digits rather than 0 and 1, which the
 * immediates 0x30 and 0x31 fix.
 *
 * @param stream The stream to write to.
 * @param pMsg The message, which may be null.
 * @return The stream, allowing calls to be chained.
 * @ghidraAddress 0x00556508
 */
OBStream &WriteMessagePointerToStream(OBStream &stream, Message *pMsg);
