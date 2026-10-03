#include "msg/messageio.h"

#include <algorithm>
#include <iostream>
#include <vector>

#include "msg/messagefactory.h"
#include "os/log.h"
#include "stream/ibstream.h"

namespace {

// One entry of the factory list. The comparison at 0x005565e8 tests mType alone, and that is what
// fixes the ordering below.
struct MessageFactoryEntry {
    int mType;
    MessageFactoryProc mpfnCreate;
};

bool operator<(const MessageFactoryEntry &left, const MessageFactoryEntry &right) {
    return left.mType < right.mType;
}

typedef std::vector<MessageFactoryEntry> MessageFactoryTable;

// NTSC-U/C: 0x005558f0, PAL: 0x00595f78
MessageFactoryTable &MessageFactoryList() {
    static MessageFactoryTable table;
    return table;
}

} // namespace

// NTSC-U/C: 0x00555948, PAL: 0x00595fd0
MessageFactory::MessageFactory(int nType, MessageFactoryProc pfnCreate) {
    MessageFactoryEntry wanted;
    wanted.mType = nType; // Yes, the binary never writes the other field of the search key.
    MessageFactoryTable::iterator position =
        std::lower_bound(MessageFactoryList().begin(), MessageFactoryList().end(), wanted);
    MessageFactoryEntry entry;
    entry.mType = nType;
    entry.mpfnCreate = pfnCreate;
    (void)MessageFactoryList(); // Yes, the binary discards this call's result.
    MessageFactoryList().insert(position, entry);
}

// NTSC-U/C: 0x005563d0, PAL: 0x00596a58
Message *Message::NewMessage(int nType) {
    MessageFactoryTable &table = MessageFactoryList();
    MessageFactoryEntry wanted;
    wanted.mType = nType; // Yes, the binary never writes the other field of the search key.
    MessageFactoryTable::iterator entry = std::lower_bound(table.begin(), table.end(), wanted);
    if (entry == table.end() || entry->mType != nType) {
        return nullptr;
    }
    return (*entry->mpfnCreate)();
}

// NTSC-U/C: 0x00556290, PAL: 0x00596918
std::ostream &Message::PrintBraced(std::ostream &stream) {
    stream << "{" << Name() << " ";
    Print(stream);
    return stream << "}";
}

// NTSC-U/C: 0x00555a18, PAL: 0x005960a0
IBStream &ReadMessageBodyFromStream(IBStream &stream, Message &msg) {
    char cPresent;
    unsigned short nType;
    stream.ReadBytes(&cPresent, sizeof(cPresent)).Read(&nType, sizeof(nType));
    if (cPresent != '1') {
        Fatal("Stream error while reading in a Message object.");
    }
    if (msg.Type() != nType) {
        Fatal("Streamed Message ID %ld does not match expected id %ld.", nType, msg.Type());
    }
    msg.Load(stream);
    return stream;
}

// NTSC-U/C: 0x00555b10, PAL: 0x00596198
IBStream &ReadMessagePointerFromStream(IBStream &stream, Message *&pMsg) {
    char cPresent;
    stream.ReadBytes(&cPresent, sizeof(cPresent));
    if (stream.Eof() || cPresent == '0') {
        pMsg = nullptr;
        return stream;
    }
    if (cPresent != '1') {
        Fatal("Stream error while reading in a Message object pointer.");
        return stream;
    }
    int nType;
    stream.Read(&nType, sizeof(nType));
    Message *pNew = Message::NewMessage(nType);
    if (pNew == nullptr) {
        Fatal("Cannot find ID %ld in Message Factory List", nType);
    }
    (void)pNew->Type(); // Yes, the binary discards this call's result.
    pNew->Load(stream);
    pMsg = pNew;
    return stream;
}

// NTSC-U/C: 0x00556448, PAL: 0x00596ad0
OBStream &WriteMessageBodyToStream(OBStream &stream, Message &msg) {
    (void)msg.Type(); // Yes, the binary discards this call's result.
    const unsigned short nType = msg.Type();
    char cPresent = '1';
    stream.WriteBytes(&cPresent, sizeof(cPresent)).Write(&nType, sizeof(nType));
    msg.Save(stream);
    return stream;
}

// NTSC-U/C: 0x00556508, PAL: 0x00596b90
OBStream &WriteMessagePointerToStream(OBStream &stream, Message *pMsg) {
    if (pMsg == nullptr) {
        char cAbsent = '0';
        stream.WriteBytes(&cAbsent, sizeof(cAbsent));
        return stream;
    }
    char cPresent = '1';
    OBStream &written = stream.WriteBytes(&cPresent, sizeof(cPresent));
    int nType = pMsg->Type();
    written.Write(&nType, sizeof(nType));
    pMsg->Save(stream);
    return stream;
}
