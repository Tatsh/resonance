#include "sch/command.h"

#include <algorithm>
#include <vector>

#include "os/log.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Sch {

namespace {

// Produces one command of a registered class.
typedef Command *(*CommandFactoryProc)();

// One entry of the factory list. The title is inferred from the diagnostic
// "Cannot find ID %ld in Command Factory List". The comparison at 0x0053a0b0 tests mCmdID alone,
// and that is what fixes the ordering below.
struct CommandFactoryEntry {
    int mCmdID;
    CommandFactoryProc mpfnCreate;
};

bool operator<(const CommandFactoryEntry &left, const CommandFactoryEntry &right) {
    return left.mCmdID < right.mCmdID;
}

typedef std::vector<CommandFactoryEntry> CommandFactoryTable;

// 0x005381b0
CommandFactoryTable &CommandFactoryList() {
    static CommandFactoryTable table;
    return table;
}

} // namespace

// 0x00539ef8
Command::~Command() {
}

// 0x0053a088
void Command::Print(ostream &stream) {
    stream << "{Command}";
}

// 0x00539f20
void Command::Save(OBStream &stream) {
}

// 0x00539f28
void Command::Load(IBStream &stream) {
}

// 0x00539fe8
Command *Command::NewCommand(int nCmdID) {
    if (nCmdID == 0) {
        cerr << " Attempted to call NewCommand(0); returning NULL" << endl;
        return nullptr;
    }
    CommandFactoryTable &table = CommandFactoryList();
    CommandFactoryEntry wanted;
    wanted.mCmdID = nCmdID; // Yes, the binary never writes the other field of the search key.
    CommandFactoryTable::iterator entry = std::lower_bound(table.begin(), table.end(), wanted);
    if (entry == table.end() || entry->mCmdID != nCmdID) {
        return nullptr;
    }
    return (*entry->mpfnCreate)();
}

// 0x005384d0
OBStream &operator<<(OBStream &stream, Command *pCommand) {
    if (pCommand != nullptr) {
        if (pCommand->CmdID() != 0) {
            char cPresent = '1';
            stream.WriteBytes(&cPresent, sizeof(cPresent));
            int nCmdID = pCommand->CmdID(); // Yes, the binary dispatches this slot twice.
            stream.Write(&nCmdID, sizeof(nCmdID));
            pCommand->Save(stream);
            return stream;
        }
        pCommand->Print(cout);
        Fatal(" Attempted to serialize a command that is non-serializable!");
    }
    char cAbsent = '0';
    stream.WriteBytes(&cAbsent, sizeof(cAbsent));
    return stream;
}

// 0x005385f0
IBStream &operator>>(IBStream &stream, Command *&pCommand) {
    char cPresent;
    stream.ReadBytes(&cPresent, sizeof(cPresent));
    if (stream.Eof() || cPresent == '0') {
        pCommand = nullptr;
        return stream;
    }
    if (cPresent != '1') {
        Fatal("Stream error while reading in a Command object.");
    }
    int nCmdID;
    stream.Read(&nCmdID, sizeof(nCmdID));
    Command *pNew = Command::NewCommand(nCmdID);
    if (pNew == nullptr) {
        Fatal("Cannot find ID %ld in Command Factory List", nCmdID);
    }
    (void)pNew->CmdID(); // Yes, the binary discards this call's result.
    pNew->Load(stream);
    pCommand = pNew;
    return stream;
}

} // namespace Sch
