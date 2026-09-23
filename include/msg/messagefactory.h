#pragma once

class Message;

/** Produces one default-constructed message of a registered class. */
typedef Message *(*MessageFactoryProc)();

/**
 * Registrar that adds one message class to the factory list Message::NewMessage() searches.
 *
 * The translation unit at `0x003d9818` constructs 92 of these as file-scope objects, one for each
 * concrete message, passing the identity Message::Type() reports and the class's static New().
 * The class is not polymorphic, emits no RTTI descriptor, and writes no member, and the title is
 * inferred from the diagnostic `Cannot find ID %ld in Message Factory List`.
 */
class MessageFactory {
public:
    /**
     * Insert a factory into the list, ordered by identity.
     *
     * Unlike Sch::CommandFactory, the constructor has no zero test and inserts every pair.
     *
     * @param nType The identity the class streams itself under.
     * @param pfnCreate The factory for the class.
     * @ghidraAddress 0x00555948
     */
    MessageFactory(int nType, MessageFactoryProc pfnCreate);
};
