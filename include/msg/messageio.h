#pragma once

#include "msg/message.h"
#include "stream/obstream.h"

/**
 * Write a message through a stream, with a flag for the null case.
 *
 * Writes the single byte `0` and returns for a null message. Otherwise it writes the byte `1`,
 * then the message's identity as four bytes and its payload through the message's own virtuals.
 * The two bytes are the ASCII digits rather than 0 and 1, which the immediates 0x30 and 0x31 fix.
 *
 * @param stream The stream to write to.
 * @param pMsg The message, which may be null.
 * @return The stream, allowing calls to be chained.
 * @ghidraAddress 0x00556508
 */
OBStream &WriteMessagePointerToStream(OBStream &stream, Message *pMsg);

/**
 * Write a message that is known to exist through the stream, preceded by the byte `1`.
 *
 * The identity goes out as two bytes here rather than the four of the pointer form, and Type() is
 * dispatched twice with the first result discarded. The shipped program does not call it.
 *
 * @param stream The stream to write to.
 * @param msg The message.
 * @return The stream, allowing calls to be chained.
 * @ghidraAddress 0x00556448
 */
OBStream &WriteMessageBodyToStream(OBStream &stream, Message &msg);

/**
 * Read a message's payload back into an existing message.
 *
 * The presence byte and a two-byte identity are read together. A presence byte other than `1`
 * and an identity other than the message's own Type() are each reported through Fatal(). The
 * shipped program does not call it.
 *
 * @param stream The stream to read from.
 * @param msg The message to fill.
 * @return The stream, allowing calls to be chained.
 * @ghidraAddress 0x00555a18
 */
IBStream &ReadMessageBodyFromStream(IBStream &stream, Message &msg);

/**
 * Read a message back through the stream and construct it from the factory list.
 *
 * The presence byte `0`, and an end of data after it, both produce a null result. Any byte other
 * than `0` or `1` is reported through Fatal() and returns without writing pMsg. An unregistered
 * identity is reported through Fatal() as well.
 *
 * @param stream The stream to read from.
 * @param pMsg Receives the new message, or null.
 * @return The stream, allowing calls to be chained.
 * @ghidraAddress 0x00555b10
 */
IBStream &ReadMessagePointerFromStream(IBStream &stream, Message *&pMsg);
